#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#include "gpio_event_uapi.h"

/*
 * GPIO 按键和 LED 驱动学习流程：
 *
 * 1. 模块加载后申请字符设备号，并注册 platform_driver。
 * 2. 设备树的 compatible 匹配成功后进入 probe()。
 * 3. probe() 通过 key-gpios 获取按键，通过 led-gpios 获取 LED，
 *    再把按键 GPIO 转换为 IRQ。
 * 4. 中断处理函数只做很少的工作：记录“按键可能变化”，并安排延迟
 *    工作。延迟工作在进程上下文中读取 GPIO，从而完成消抖。
 * 5. 消抖后的事件进入环形队列，唤醒阻塞在 read()/poll() 的用户进程。
 * 6. 用户态通过 ioctl 控制 LED，通过 read() 获取按键事件。
 */

#define GPIO_EVENT_NAME        "gpio-event"
#define GPIO_EVENT_QUEUE_SIZE 16
#define GPIO_EVENT_DEBOUNCE_MS 20

/*
 * 用户态读取的按键事件结构体。
 * timestamp_ns：内核单调时钟时间戳。
 * value：按键 GPIO 的稳定电平。
 * sequence：事件序号，用来发现事件丢失。
 */
struct gpio_event_device {
	/* 设备模型对象，日志和设备资源管理都以它为上下文。 */
	struct device *dev;

	/* 设备树中 key-gpios 对应的按键 GPIO 描述符。 */
	struct gpio_desc *key;

	/* 设备树中 led-gpios 对应的 LED GPIO 描述符。 */
	struct gpio_desc *led;

	/* 按键 GPIO 对应的 Linux 中断号。 */
	int irq;

	/* 中断只安排 delayed_work，真正消抖在这里执行。 */
	struct delayed_work debounce_work;

	/* 保护环形队列、事件序号和 LED 状态。 */
	spinlock_t queue_lock;

	/* 有新事件时唤醒 read()/poll()。 */
	wait_queue_head_t read_wait;

	/* 按键事件环形队列。 */
	struct gpio_event_record queue[GPIO_EVENT_QUEUE_SIZE];
	unsigned int queue_head;
	unsigned int queue_tail;
	unsigned int sequence;

	/* 最近一次通过 ioctl 设置的 LED 逻辑状态。 */
	bool led_value;

	/* 字符设备三件套：cdev、设备号、/dev 对应 device。 */
	struct cdev cdev;
	dev_t devt;
	struct class *class;
	struct device *chardev;
};

static dev_t gpio_event_devt;
static struct class *gpio_event_class;

static bool gpio_event_queue_empty(struct gpio_event_device *event)
{
	return event->queue_head == event->queue_tail;
}

static bool gpio_event_has_data(struct gpio_event_device *event)
{
	unsigned long flags;
	bool has_data;

	spin_lock_irqsave(&event->queue_lock, flags);
	has_data = !gpio_event_queue_empty(event);
	spin_unlock_irqrestore(&event->queue_lock, flags);
	return has_data;
}

static bool gpio_event_queue_full(struct gpio_event_device *event)
{
	return ((event->queue_head + 1) % GPIO_EVENT_QUEUE_SIZE) ==
	       event->queue_tail;
}

static void gpio_event_push(struct gpio_event_device *event, u32 value)
{
	unsigned long flags;
	struct gpio_event_record *record;

	spin_lock_irqsave(&event->queue_lock, flags);

	/* 队列满时丢弃最旧事件，保证最新按键状态还能被读到。 */
	if (gpio_event_queue_full(event))
		event->queue_tail = (event->queue_tail + 1) % GPIO_EVENT_QUEUE_SIZE;

	record = &event->queue[event->queue_head];
	record->timestamp_ns = ktime_get_ns();
	record->value = value;
	record->sequence = ++event->sequence;
	event->queue_head = (event->queue_head + 1) % GPIO_EVENT_QUEUE_SIZE;

	spin_unlock_irqrestore(&event->queue_lock, flags);
	wake_up_interruptible(&event->read_wait);
}

static irqreturn_t gpio_event_irq(int irq, void *data)
{
	struct gpio_event_device *event = data;

	/* 中断上下文不做可能阻塞的 GPIO 访问，只安排消抖工作。 */
	mod_delayed_work(system_wq, &event->debounce_work,
			 msecs_to_jiffies(GPIO_EVENT_DEBOUNCE_MS));
	return IRQ_HANDLED;
}

static void gpio_event_debounce_work(struct work_struct *work)
{
	struct gpio_event_device *event = container_of(to_delayed_work(work),
							 struct gpio_event_device,
							 debounce_work);
	int value;

	value = gpiod_get_value_cansleep(event->key);
	if (value < 0) {
		dev_err(event->dev, "读取按键 GPIO 失败：%d\n", value);
		return;
	}

	gpio_event_push(event, value);
}

static int gpio_event_open(struct inode *inode, struct file *file)
{
	struct gpio_event_device *event = container_of(inode->i_cdev,
						       struct gpio_event_device, cdev);

	file->private_data = event;
	return 0;
}

static ssize_t gpio_event_read(struct file *file, char __user *buf,
				       size_t count, loff_t *ppos)
{
	struct gpio_event_device *event = file->private_data;
	struct gpio_event_record record;
	unsigned long flags;
	int ret;
	(void)ppos;

	if (count < sizeof(record))
		return -EINVAL;

	if (!gpio_event_has_data(event)) {
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;

		ret = wait_event_interruptible(event->read_wait,
					       gpio_event_has_data(event));
		if (ret)
			return -ERESTARTSYS;
	}

	spin_lock_irqsave(&event->queue_lock, flags);
	/* 重新检查，避免并发情况下错误取数。 */
	if (gpio_event_queue_empty(event)) {
		spin_unlock_irqrestore(&event->queue_lock, flags);
		return -EAGAIN;
	}
	record = event->queue[event->queue_tail];
	event->queue_tail = (event->queue_tail + 1) % GPIO_EVENT_QUEUE_SIZE;
	spin_unlock_irqrestore(&event->queue_lock, flags);

	if (copy_to_user(buf, &record, sizeof(record)))
		return -EFAULT;

	return sizeof(record);
}

static __poll_t gpio_event_poll(struct file *file, poll_table *wait)
{
	struct gpio_event_device *event = file->private_data;
	__poll_t mask = 0;

	poll_wait(file, &event->read_wait, wait);
	if (gpio_event_has_data(event))
		mask |= EPOLLIN | EPOLLRDNORM;

	return mask;
}

static long gpio_event_ioctl(struct file *file, unsigned int cmd,
				     unsigned long arg)
{
	struct gpio_event_device *event = file->private_data;
	u32 value;

	switch (cmd) {
	case GPIO_EVENT_IOC_SET_LED:
		if (copy_from_user(&value, (void __user *)arg, sizeof(value)))
			return -EFAULT;
		if (value > 1)
			return -EINVAL;
		gpiod_set_value_cansleep(event->led, value);
		event->led_value = value;
		return 0;

	case GPIO_EVENT_IOC_GET_LED:
		value = event->led_value;
		if (copy_to_user((void __user *)arg, &value, sizeof(value)))
			return -EFAULT;
		return 0;

	default:
		return -ENOTTY;
	}
}

static const struct file_operations gpio_event_fops = {
	.owner = THIS_MODULE,
	.open = gpio_event_open,
	.read = gpio_event_read,
	.poll = gpio_event_poll,
	.unlocked_ioctl = gpio_event_ioctl,
	.llseek = no_llseek,
};

static int gpio_event_probe(struct platform_device *pdev)
{
	struct gpio_event_device *event;
	int ret;

	event = devm_kzalloc(&pdev->dev, sizeof(*event), GFP_KERNEL);
	if (!event)
		return -ENOMEM;

	event->dev = &pdev->dev;
	event->devt = gpio_event_devt;
	event->class = gpio_event_class;
	spin_lock_init(&event->queue_lock);
	init_waitqueue_head(&event->read_wait);
	INIT_DELAYED_WORK(&event->debounce_work, gpio_event_debounce_work);

	event->key = devm_gpiod_get(&pdev->dev, "key", GPIOD_IN);
	if (IS_ERR(event->key))
		return dev_err_probe(&pdev->dev, PTR_ERR(event->key),
				     "获取 key GPIO 失败\n");

	event->led = devm_gpiod_get(&pdev->dev, "led", GPIOD_OUT_LOW);
	if (IS_ERR(event->led))
		return dev_err_probe(&pdev->dev, PTR_ERR(event->led),
				     "获取 led GPIO 失败\n");

	event->irq = gpiod_to_irq(event->key);
	if (event->irq < 0)
		return dev_err_probe(&pdev->dev, event->irq,
				     "按键 GPIO 没有关联有效 IRQ\n");

	ret = devm_request_irq(&pdev->dev, event->irq, gpio_event_irq,
				       IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				       GPIO_EVENT_NAME, event);
	if (ret)
		return dev_err_probe(&pdev->dev, ret, "申请按键 IRQ 失败\n");

	cdev_init(&event->cdev, &gpio_event_fops);
	event->cdev.owner = THIS_MODULE;
	ret = cdev_add(&event->cdev, event->devt, 1);
	if (ret)
		return dev_err_probe(&pdev->dev, ret, "添加 GPIO cdev 失败\n");

	event->chardev = device_create(event->class, &pdev->dev, event->devt,
				       event, GPIO_EVENT_NAME);
	if (IS_ERR(event->chardev)) {
		ret = PTR_ERR(event->chardev);
		cdev_del(&event->cdev);
		return dev_err_probe(&pdev->dev, ret,
				     "创建 /dev/gpio-event 失败\n");
	}

	platform_set_drvdata(pdev, event);
	dev_info(&pdev->dev, "GPIO 按键/LED 驱动探测成功\n");
	return 0;
}

static int gpio_event_remove(struct platform_device *pdev)
{
	struct gpio_event_device *event = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&event->debounce_work);
	device_destroy(event->class, event->devt);
	cdev_del(&event->cdev);
	return 0;
}

static const struct of_device_id gpio_event_of_match[] = {
	{ .compatible = "study-wg,gpio-event" },
	{ }
};
MODULE_DEVICE_TABLE(of, gpio_event_of_match);

static struct platform_driver gpio_event_driver = {
	.probe = gpio_event_probe,
	.remove = gpio_event_remove,
	.driver = {
		.name = GPIO_EVENT_NAME,
		.of_match_table = gpio_event_of_match,
	},
};

static int __init gpio_event_init(void)
{
	int ret;

	ret = alloc_chrdev_region(&gpio_event_devt, 0, 1, GPIO_EVENT_NAME);
	if (ret)
		return ret;

	gpio_event_class = class_create(THIS_MODULE, GPIO_EVENT_NAME);
	if (IS_ERR(gpio_event_class)) {
		ret = PTR_ERR(gpio_event_class);
		unregister_chrdev_region(gpio_event_devt, 1);
		return ret;
	}

	ret = platform_driver_register(&gpio_event_driver);
	if (ret) {
		class_destroy(gpio_event_class);
		unregister_chrdev_region(gpio_event_devt, 1);
	}
	return ret;
}

static void __exit gpio_event_exit(void)
{
	platform_driver_unregister(&gpio_event_driver);
	class_destroy(gpio_event_class);
	unregister_chrdev_region(gpio_event_devt, 1);
}

module_init(gpio_event_init);
module_exit(gpio_event_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("study-w-g");
MODULE_DESCRIPTION("GPIO 按键中断、消抖、事件队列与 LED 控制驱动");
