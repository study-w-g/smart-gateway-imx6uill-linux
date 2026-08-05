#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>

/*
 * 本驱动的整体调用流程（学习版）：
 *
 * 1. 模块加载时，ds18b20_init() 分配一个字符设备号、创建 class，
 *    然后注册 platform_driver。
 * 2. Linux 设备模型根据设备树中的 compatible 属性，把设备节点和
 *    本驱动的 of_match_table 进行匹配。匹配成功后调用 ds18b20_probe()。
 * 3. probe() 中获取设备树提供的 dq-gpios，初始化互斥锁，注册 cdev，
 *    最后通过 device_create() 创建 /dev/ds18b20。
 * 4. 用户态 C 服务调用 open()/read() 后，进入 ds18b20_open()/
 *    ds18b20_read()，驱动完成 1-Wire 复位、写命令和读 Scratchpad。
 * 5. 驱动向用户态返回 DS18B20 的原始 16 位温度值；原始值转换为摄氏度、
 *    阈值判断、超时策略和 QT 显示属于用户态 C 服务/ QT 前端的职责。
 * 6. 驱动卸载或设备移除时，platform 框架调用 ds18b20_remove()，
 *    模块退出路径再释放 class 和字符设备号。
 *
 * 注意：温度换算可以放在应用层，但底层总线响应超时仍应由驱动控制。
 * 例如等待 DS18B20 完成温度转换时必须有明确的上限，不能无限期等待硬件。
 */

#define DS18B20_NAME "ds18b20"

#define DS18B20_CMD_SKIP_ROM     0xCC
#define DS18B20_CMD_CONVERT_T    0x44
#define DS18B20_CMD_READ_SCRATCH 0xBE

struct ds18b20 {
	/*
	 * 指向设备模型中的 struct device 对象。
	 *
	 * 初始化位置：在 ds18b20_probe() 中执行
	 *     sensor->dev = &pdev->dev;
	 *
	 * pdev 由 platform 总线传入，pdev->dev 是当前设备在 Linux 设备模型
	 * 中的通用设备对象。后续可以使用 sensor->dev 进行 dev_info()、
	 * dev_err()、设备树资源访问和设备关联。
	 */
	struct device *dev;

	/*
	 * DS18B20 的 DQ 数据线描述符。
	 *
	 * 它不是 GPIO 整数编号，而是 Linux GPIO descriptor。初始化位置在
	 * probe()：
	 *     sensor->dq = devm_gpiod_get(&pdev->dev, "dq", GPIOD_IN);
	 *
	 * "dq" 对应设备树属性名 dq-gpios。devm_ 前缀表示资源由设备模型
	 * 自动管理，probe 失败或 remove 时会自动释放，避免手动 gpio_free()。
	 */
	struct gpio_desc *dq;

	/*
	 * 保护一次完整的传感器访问流程。
	 *
	 * 1-Wire 读写包含多个严格有序的时隙，不能让两个用户线程同时操作
	 * 同一根 DQ 线。因此 read() 在访问硬件前加锁，访问结束后解锁。
	 */
	struct mutex lock;

	/*
	 * Linux 字符设备对象。
	 *
	 * cdev 把本驱动的 file_operations（open/read 等）和设备号关联起来。
	 * 它不是设备号本身，也不是 /dev 下的目录项。初始化位置在 probe()：
	 *     cdev_init(&sensor->cdev, &ds18b20_fops);
	 */
	struct cdev cdev;

	/*
	 * 当前实例使用的设备号，包含主设备号和次设备号。
	 *
	 * 设备号的分配发生在模块初始化 ds18b20_init() 中：
	 *     alloc_chrdev_region(&ds18b20_devt, 0, 1, DS18B20_NAME);
	 *
	 * probe() 再把全局分配到的 ds18b20_devt 保存到每个设备实例的 devt，
	 * 供 cdev_add() 和 device_create() 使用。
	 */
	dev_t devt;

	/*
	 * 字符设备 class 指针。
	 *
	 * class 的创建发生在模块初始化 ds18b20_init()：
	 *     ds18b20_class = class_create(...);
	 *
	 * 它用于把设备归入 /sys/class/ds18b20，并配合 device_create() 完成
	 * 用户态设备节点的创建。class 由模块退出函数统一销毁。
	 */
	struct class *class;

	/*
	 * /dev/ds18b20 对应的 struct device 指针。
	 *
	 * 注意：chardev 不是“设备号的分配”，设备号保存在 devt 中；chardev
	 * 是 device_create() 返回的内核设备对象。初始化位置在 probe()：
	 *     sensor->chardev = device_create(..., sensor->devt, ...);
	 *
	 * device_create() 会使用 devt 生成 /dev/ds18b20 的设备节点（前提是
	 * 目标系统启用了 devtmpfs/udev 或对应的设备节点管理机制）。
	 */
	struct device *chardev;
};

/*
 * 全局字符设备号：模块只申请一次。
 * 本学习版按一个 DS18B20 设备设计，因此只申请 1 个 minor。
 * 如果以后要支持多个传感器，需要重新设计 minor 分配和实例管理。
 */
static dev_t ds18b20_devt;

/*
 * 全局 class：模块加载时创建，模块退出时销毁。每个设备实例的
 * sensor->class 都指向这个 class。
 */
static struct class *ds18b20_class;

static int ds18b20_reset(struct ds18b20 *sensor)
{
	int presence;

	gpiod_direction_output(sensor->dq, 0);
	udelay(480);
	gpiod_direction_input(sensor->dq);
	udelay(70);
	presence = !gpiod_get_value_cansleep(sensor->dq);
	udelay(410);

	return presence ? 0 : -ENODEV;
}

static void ds18b20_write_bit(struct ds18b20 *sensor, bool bit)
{
	gpiod_direction_output(sensor->dq, 0);
	if (bit) {
		udelay(6);
		gpiod_direction_input(sensor->dq);
		udelay(64);
	} else {
		udelay(60);
		gpiod_direction_input(sensor->dq);
		udelay(10);
	}
}

static bool ds18b20_read_bit(struct ds18b20 *sensor)
{
	int value;

	gpiod_direction_output(sensor->dq, 0);
	udelay(6);
	gpiod_direction_input(sensor->dq);
	udelay(9);
	value = gpiod_get_value_cansleep(sensor->dq);
	udelay(55);

	return value > 0;
}

static void ds18b20_write_byte(struct ds18b20 *sensor, u8 value)
{
	int i;

	for (i = 0; i < 8; i++) {
		ds18b20_write_bit(sensor, value & BIT(0));
		value >>= 1;
	}
}

static u8 ds18b20_read_byte(struct ds18b20 *sensor)
{
	u8 value = 0;
	int i;

	for (i = 0; i < 8; i++)
		if (ds18b20_read_bit(sensor))
			value |= BIT(i);

	return value;
}

static int ds18b20_read_temperature(struct ds18b20 *sensor, s16 *raw)
{
	u8 lsb, msb;
	int ret;

	ret = ds18b20_reset(sensor);
	if (ret)
		return ret;
	ds18b20_write_byte(sensor, DS18B20_CMD_SKIP_ROM);
	ds18b20_write_byte(sensor, DS18B20_CMD_CONVERT_T);
	/*
	 * 12-bit 分辨率下，DS18B20 最长转换时间约为 750ms。
	 * 这里的 msleep() 是一个固定等待；正式版本应进一步读取转换完成状态
	 * 或使用有上限的等待策略，并处理传感器断开等异常。
	 */
	msleep(750);

	ret = ds18b20_reset(sensor);
	if (ret)
		return ret;
	ds18b20_write_byte(sensor, DS18B20_CMD_SKIP_ROM);
	ds18b20_write_byte(sensor, DS18B20_CMD_READ_SCRATCH);

	lsb = ds18b20_read_byte(sensor);
	msb = ds18b20_read_byte(sensor);
	*raw = (s16)((msb << 8) | lsb);
	return 0;
}

static int ds18b20_open(struct inode *inode, struct file *file)
{
	/*
	 * inode->i_cdev 指向当前打开的 cdev。container_of() 根据成员 cdev
	 * 的地址反推出外层 struct ds18b20，从而获得本设备的 dq、lock、devt
	 * 等私有数据。
	 */
	struct ds18b20 *sensor = container_of(inode->i_cdev,
						     struct ds18b20, cdev);

	/*
	 * file->private_data 会在后续 read()/release() 中取出该设备实例，
	 * 避免使用全局变量，便于未来扩展多个设备。
	 */
	file->private_data = sensor;
	return 0;
}

static ssize_t ds18b20_read(struct file *file, char __user *buf,
				    size_t count, loff_t *ppos)
{
	/* open() 已经把设备实例保存到 private_data。 */
	struct ds18b20 *sensor = file->private_data;
	s16 raw;
	int ret;

	/* 用户缓冲区至少要能接收一个 s16 原始值。 */
	if (count < sizeof(raw))
		return -EINVAL;

	/*
	 * 锁住一次完整的硬件访问，防止多个进程同时驱动 DQ 时序。
	 * mutex_lock_interruptible() 允许进程在等待锁时响应信号。
	 */
	if (mutex_lock_interruptible(&sensor->lock))
		return -ERESTARTSYS;

	ret = ds18b20_read_temperature(sensor, &raw);
	mutex_unlock(&sensor->lock);
	if (ret)
		return ret;

	/*
	 * 驱动只向用户态返回原始 16 位值。
	 * 摄氏温度换算可以放到 C 用户态服务中，例如 raw / 16.0；QT 只负责
	 * 展示用户态服务已经处理好的结果。copy_to_user() 负责检查用户地址
	 * 并完成内核空间到用户空间的数据复制。
	 */
	if (copy_to_user(buf, &raw, sizeof(raw)))
		return -EFAULT;

	return sizeof(raw);
}

static const struct file_operations ds18b20_fops = {
	.owner = THIS_MODULE,
	.open = ds18b20_open,
	.read = ds18b20_read,
	.llseek = no_llseek,
};

static int ds18b20_probe(struct platform_device *pdev)
{
	struct ds18b20 *sensor;
	int ret;

	/*
	 * probe() 是标准设备模型的初始化入口：设备树节点的 compatible
	 * 与 ds18b20_of_match[] 匹配成功后，内核自动调用这里。
	 * devm_kzalloc() 分配并清零设备私有结构体，生命周期绑定到 pdev。
	 */
	sensor = devm_kzalloc(&pdev->dev, sizeof(*sensor), GFP_KERNEL);
	if (!sensor)
		return -ENOMEM;

	/* 保存设备模型对象，后续日志和资源管理都以它为上下文。 */
	sensor->dev = &pdev->dev;

	/* 初始化并发保护锁，必须在第一次 read() 前完成。 */
	mutex_init(&sensor->lock);

	/*
	 * 从设备树读取 dq-gpios，并申请 DQ 数据线。
	 * "dq" 是 GPIO descriptor 的 function 名称，对应 dq-gpios。
	 * GPIOD_IN 表示申请后先按输入方向配置；1-Wire 读写过程中会在
	 * 输出低电平和释放为输入之间切换。
	 */
	sensor->dq = devm_gpiod_get(&pdev->dev, "dq", GPIOD_IN);
	if (IS_ERR(sensor->dq))
		return dev_err_probe(&pdev->dev, PTR_ERR(sensor->dq),
				     "failed to get dq GPIO\n");

	/*
	 * 保存模块初始化阶段已经分配好的字符设备号。
	 * devt 只描述“哪个设备号”，真正把 file_operations 接入内核的是 cdev。
	 */
	sensor->devt = ds18b20_devt;

	/* 将 open/read 等文件操作绑定到 cdev 对象。 */
	cdev_init(&sensor->cdev, &ds18b20_fops);
	sensor->cdev.owner = THIS_MODULE;

	/* 把 cdev 和 devt 注册到内核字符设备框架。 */
	ret = cdev_add(&sensor->cdev, sensor->devt, 1);
	if (ret)
		return dev_err_probe(&pdev->dev, ret, "failed to add cdev\n");

	/* 使用模块级 class，供 device_create() 创建用户态设备节点。 */
	sensor->class = ds18b20_class;

	/*
	 * 创建 /dev/ds18b20 对应的 struct device。
	 * 第三个参数 sensor->devt 决定设备节点的主、次设备号；最后一个
	 * DS18B20_NAME 决定设备节点名称。返回值保存到 chardev，便于理解和
	 * 后续扩展；remove() 中用同一个 class + devt 销毁它。
	 */
	sensor->chardev = device_create(sensor->class, &pdev->dev,
					sensor->devt, sensor, DS18B20_NAME);
	if (IS_ERR(sensor->chardev)) {
		ret = PTR_ERR(sensor->chardev);
		cdev_del(&sensor->cdev);
		return dev_err_probe(&pdev->dev, ret,
				     "failed to create device node\n");
	}

	/*
	 * 把 sensor 保存到 platform_device 的驱动私有数据中。remove()、
	 * suspend/resume 等回调可以通过 platform_get_drvdata(pdev) 取回它。
	 */
	platform_set_drvdata(pdev, sensor);
	dev_info(&pdev->dev, "DS18B20 driver probed\n");
	return 0;
}

static int ds18b20_remove(struct platform_device *pdev)
{
	/* 取回 probe() 保存的设备私有结构体。 */
	struct ds18b20 *sensor = platform_get_drvdata(pdev);

	/* 按创建顺序的逆序撤销字符设备和 /dev 节点。 */
	device_destroy(sensor->class, sensor->devt);
	cdev_del(&sensor->cdev);
	return 0;
}

static const struct of_device_id ds18b20_of_match[] = {
	/*
	 * 设备树中的 compatible 必须写成同样的字符串：
	 * compatible = "study-wg,ds18b20";
	 */
	{ .compatible = "study-wg,ds18b20" },
	{ }
};
MODULE_DEVICE_TABLE(of, ds18b20_of_match);

static struct platform_driver ds18b20_driver = {
	.probe = ds18b20_probe,
	.remove = ds18b20_remove,
	.driver = {
		.name = DS18B20_NAME,
		.of_match_table = ds18b20_of_match,
	},
};

static int __init ds18b20_init(void)
{
	int ret;

	/*
	 * 模块初始化第一步：动态申请一个字符设备号。
	 * 这一步只负责获得主/次设备号，不会创建 /dev 节点，也不会绑定
	 * file_operations；真正的 cdev 绑定在 probe() 中完成。
	 */
	ret = alloc_chrdev_region(&ds18b20_devt, 0, 1, DS18B20_NAME);
	if (ret)
		return ret;

	/*
	 * 创建 class。class 是 sysfs/devtmpfs 设备管理的分类对象，配合
	 * probe() 中的 device_create() 才会出现 /dev/ds18b20。
	 */
	ds18b20_class = class_create(THIS_MODULE, DS18B20_NAME);
	if (IS_ERR(ds18b20_class)) {
		ret = PTR_ERR(ds18b20_class);
		unregister_chrdev_region(ds18b20_devt, 1);
		return ret;
	}

	/*
	 * 注册 platform_driver。注册后，内核会扫描已经存在的 platform_device，
	 * 并根据 compatible 自动匹配；匹配成功就进入 ds18b20_probe()。
	 */
	ret = platform_driver_register(&ds18b20_driver);
	if (ret) {
		class_destroy(ds18b20_class);
		unregister_chrdev_region(ds18b20_devt, 1);
	}
	return ret;
}

static void __exit ds18b20_exit(void)
{
	/*
	 * 先注销 platform_driver。内核会先触发 remove()，销毁 /dev 节点并
	 * 删除 cdev；然后这里再销毁 class 和释放字符设备号。
	 */
	platform_driver_unregister(&ds18b20_driver);
	class_destroy(ds18b20_class);
	unregister_chrdev_region(ds18b20_devt, 1);
}

module_init(ds18b20_init);
module_exit(ds18b20_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("study-w-g");
MODULE_DESCRIPTION("DS18B20 platform driver with 1-Wire character device");
