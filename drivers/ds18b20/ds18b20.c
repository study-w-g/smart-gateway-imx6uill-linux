#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define DS18B20_CNT      1
#define DS18B20_NAME     "ds18b20"

/* DS18B20 命令 */
#define DS18B20_CMD_SKIP_ROM       0xCC
#define DS18B20_CMD_CONVERT_T      0x44
#define DS18B20_CMD_READ_SCRATCH   0xBE
#define DS18B20_CMD_WRITE_SCRATCH  0x4E

/* 分辨率 12bit */
#define DS18B20_RESOLUTION 0x7F

/* 高温 75℃，低温 70℃ */
#define DS18B20_TH 75
#define DS18B20_TL 70

struct ds18b20_dev {
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    struct device_node *node;
    int ds_gpio;
    struct mutex lock;
};

static struct ds18b20_dev ds18b20;

/* 1-Wire 初始化 */
static int ds18b20_init_gpio(void)
{
    gpio_direction_output(ds18b20.ds_gpio, 0);
    udelay(500);
    gpio_set_value(ds18b20.ds_gpio, 1);
    udelay(60);
    return gpio_get_value(ds18b20.ds_gpio);
}

/* 写一个字节 */
static void ds18b20_write_byte(u8 val)
{
    int i;

    for (i = 0; i < 8; i++) {
        gpio_direction_output(ds18b20.ds_gpio, 0);
        udelay(2);
        gpio_set_value(ds18b20.ds_gpio, val & 1);
        udelay(60);
        gpio_set_value(ds18b20.ds_gpio, 1);
        val >>= 1;
    }
}

/* 读一个字节 */
static u8 ds18b20_read_byte(void)
{
    int i;
    u8 val = 0;

    for (i = 0; i < 8; i++) {
        gpio_direction_output(ds18b20.ds_gpio, 0);
        udelay(2);
        gpio_direction_input(ds18b20.ds_gpio);
        udelay(5);
        if (gpio_get_value(ds18b20.ds_gpio))
            val |= (1 << i);
        udelay(50);
    }

    return val;
}

/* 读取温度 */
static int ds18b20_get_temp(int *temp)
{
    u8 lsb, msb;

    ds18b20_init_gpio();
    ds18b20_write_byte(DS18B20_CMD_SKIP_ROM);
    ds18b20_write_byte(DS18B20_CMD_CONVERT_T);
    usleep_range(800000, 850000);

    ds18b20_init_gpio();
    ds18b20_write_byte(DS18B20_CMD_SKIP_ROM);
    ds18b20_write_byte(DS18B20_CMD_READ_SCRATCH);

    lsb = ds18b20_read_byte();
    msb = ds18b20_read_byte();

    *temp = (msb << 8) | lsb;
    return 0;
}

static int ds18b20_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &ds18b20;
    return 0;
}

static ssize_t ds18b20_read(struct file *filp, char __user *buf,
                             size_t cnt, loff_t *off)
{
    int temp;

    if (cnt < sizeof(temp))
        return -EINVAL;

    if (mutex_lock_interruptible(&ds18b20.lock))
        return -ERESTARTSYS;

    if (ds18b20_get_temp(&temp)) {
        mutex_unlock(&ds18b20.lock);
        return -EIO;
    }

    mutex_unlock(&ds18b20.lock);

    /* TODO: 将原始寄存器值转换为实际温度，并补充温度符号处理。 */
    if (copy_to_user(buf, &temp, sizeof(temp)))
        return -EFAULT;

    return sizeof(temp);
}

static int ds18b20_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static const struct file_operations ds18b20_fops = {
    .owner   = THIS_MODULE,
    .open    = ds18b20_open,
    .read    = ds18b20_read,
    .release = ds18b20_release,
};

static int __init ds_init(void)
{
    int ret = 0;

    mutex_init(&ds18b20.lock);

    /* 1. 获取设备树节点 */
    ds18b20.node = of_find_node_by_path("/ds18b20");
    if (ds18b20.node == NULL) {
        pr_err("ds18b20 node not found!\n");
        return -EINVAL;
    }

    /* 2. 获取 GPIO */
    ds18b20.ds_gpio = of_get_named_gpio(ds18b20.node, "ds_gpio", 0);
    if (ds18b20.ds_gpio < 0) {
        pr_err("can't get ds_gpio\n");
        return -EINVAL;
    }

    /* 3. 申请 GPIO */
    ret = gpio_request(ds18b20.ds_gpio, DS18B20_NAME);
    if (ret) {
        pr_err("gpio request failed\n");
        return ret;
    }

    /* 4. 设置输出 */
    gpio_direction_output(ds18b20.ds_gpio, 1);

    /* 5. 注册字符设备 */
    if (ds18b20.major) {
        ds18b20.devid = MKDEV(ds18b20.major, 0);
        ret = register_chrdev_region(ds18b20.devid, DS18B20_CNT,
                                     DS18B20_NAME);
    } else {
        ret = alloc_chrdev_region(&ds18b20.devid, 0, DS18B20_CNT,
                                  DS18B20_NAME);
        ds18b20.major = MAJOR(ds18b20.devid);
        ds18b20.minor = MINOR(ds18b20.devid);
    }
    if (ret)
        goto err_gpio;

    /* 6. 初始化 cdev */
    cdev_init(&ds18b20.cdev, &ds18b20_fops);
    ret = cdev_add(&ds18b20.cdev, ds18b20.devid, DS18B20_CNT);
    if (ret)
        goto err_unregister;

    /* 7. 创建类与设备 */
    ds18b20.class = class_create(THIS_MODULE, DS18B20_NAME);
    if (IS_ERR(ds18b20.class)) {
        ret = PTR_ERR(ds18b20.class);
        goto err_cdev;
    }

    ds18b20.device = device_create(ds18b20.class, NULL, ds18b20.devid,
                                   NULL, DS18B20_NAME);
    if (IS_ERR(ds18b20.device)) {
        ret = PTR_ERR(ds18b20.device);
        goto err_class;
    }

    pr_info("ds18b20 driver init ok\n");
    return 0;

err_class:
    class_destroy(ds18b20.class);
err_cdev:
    cdev_del(&ds18b20.cdev);
err_unregister:
    unregister_chrdev_region(ds18b20.devid, DS18B20_CNT);
err_gpio:
    gpio_free(ds18b20.ds_gpio);
    return ret;
}

static void __exit ds_exit(void)
{
    device_destroy(ds18b20.class, ds18b20.devid);
    class_destroy(ds18b20.class);
    cdev_del(&ds18b20.cdev);
    unregister_chrdev_region(ds18b20.devid, DS18B20_CNT);
    gpio_free(ds18b20.ds_gpio);
    of_node_put(ds18b20.node);
}

module_init(ds_init);
module_exit(ds_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("study-w-g");
MODULE_DESCRIPTION("DS18B20 1-Wire character device driver");
