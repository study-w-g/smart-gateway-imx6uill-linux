#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

/*
 * SHT30 驱动的整体流程：
 *
 * 1. 模块加载时申请字符设备号、创建 class，然后注册 i2c_driver。
 * 2. Linux I2C 核心根据设备树中的 compatible 和 reg 属性创建
 *    i2c_client，并与本驱动的 of_match_table 进行匹配。
 * 3. 匹配成功后调用 sht30_probe()。probe() 保存 I2C 设备对象，
 *    初始化互斥锁，注册 cdev，并创建 /dev/sht30。
 * 4. 用户态 C 服务调用 open()/read()，进入 sht30_read()。
 * 5. 驱动通过 Linux I2C 核心发送 SHT30 单次测量命令，等待测量完成，
 *    读取 6 字节数据并校验两个 CRC。
 * 6. 驱动只返回温度和湿度原始值；摄氏温度、相对湿度、阈值判断和
 *    Qt 显示由用户态 C 服务及 Qt 前端完成。
 *
 * 注意：本文件使用 Linux I2C 子系统，不自行实现 START、STOP、ACK、
 * SDA/SCL 时序，也不自行配置 UART 式的数据位和停止位。
 */

#define SHT30_NAME                 "sht30"
#define SHT30_FRAME_SIZE           6
#define SHT30_RETRY_COUNT          3
#define SHT30_MEASUREMENT_DELAY_MS 20
#define SHT30_RETRY_DELAY_US_MIN   20000
#define SHT30_RETRY_DELAY_US_MAX   25000

/* 单次测量、高重复性、不使用时钟拉伸。 */
#define SHT30_CMD_MEASURE_MSB      0x24
#define SHT30_CMD_MEASURE_LSB      0x00

/*
 * 用户态与驱动之间传递的数据格式。
 *
 * raw_temperature：SHT30 返回的 16 位温度原始值。
 * raw_humidity：SHT30 返回的 16 位湿度原始值。
 *
 * 用户态换算公式：
 * temperature = -45.0 + 175.0 * raw_temperature / 65535.0
 * humidity = 100.0 * raw_humidity / 65535.0
 */
struct sht30_measurement {
	__u16 raw_temperature;
	__u16 raw_humidity;
};

struct sht30_device {
	/* 当前 SHT30 对应的 I2C 设备对象，包含地址和 I2C 控制器信息。 */
	struct i2c_client *client;

	/*
	 * 保护一次完整的“发送命令-等待-读取-校验”流程。
	 * 多个进程不能同时操作同一个 SHT30。
	 */
	struct mutex lock;

	/* 将 open/read 等文件操作与设备号关联起来。 */
	struct cdev cdev;

	/* 当前字符设备实例使用的主设备号和次设备号。 */
	dev_t devt;

	/* 用于配合 device_create() 创建 /dev/sht30。 */
	struct class *class;

	/* device_create() 返回的 /dev/sht30 对应内核设备对象。 */
	struct device *chardev;
};

/* 本学习版只支持一个 SHT30 设备，因此只申请一个次设备号。 */
static dev_t sht30_devt;
static struct class *sht30_class;

/*
 * SHT30 使用 CRC-8：
 * 初始值 0xFF，多项式 0x31，输入和输出不反转。
 */
static u8 sht30_crc8(const u8 *data, int length)
{
	u8 crc = 0xFF;
	int i;
	int bit;

	for (i = 0; i < length; i++) {
		crc ^= data[i];
		for (bit = 0; bit < 8; bit++) {
			if (crc & 0x80)
				crc = (crc << 1) ^ 0x31;
			else
				crc <<= 1;
		}
	}

	return crc;
}

/*
 * 执行一次 SHT30 测量。
 *
 * 这里使用 i2c_master_send()/i2c_master_recv()，底层 START、STOP、ACK
 * 和 SCL/SDA 时序由 Linux I2C 控制器驱动完成。
 */
static int sht30_measure(struct sht30_device *sht30,
			 struct sht30_measurement *measurement)
{
	static const u8 command[] = {
		SHT30_CMD_MEASURE_MSB,
		SHT30_CMD_MEASURE_LSB,
	};
	u8 data[SHT30_FRAME_SIZE];
	int ret;
	int attempt;

	for (attempt = 0; attempt < SHT30_RETRY_COUNT; attempt++) {
		/* 发送 0x24 0x00，启动一次高重复性测量。 */
		ret = i2c_master_send(sht30->client, command, sizeof(command));
		if (ret != sizeof(command)) {
			if (ret >= 0)
				ret = -EIO;
			goto retry;
		}

		/* 高重复性测量需要等待一段时间，且等待时间有明确上限。 */
		msleep(SHT30_MEASUREMENT_DELAY_MS);

		/* 读取：温度 2 字节+CRC，湿度 2 字节+CRC。 */
		ret = i2c_master_recv(sht30->client, data, sizeof(data));
		if (ret != sizeof(data)) {
			if (ret >= 0)
				ret = -EIO;
			goto retry;
		}

		/* 分别校验温度和湿度数据，数据损坏时不向用户态返回。 */
		if (sht30_crc8(&data[0], 2) != data[2] ||
		    sht30_crc8(&data[3], 2) != data[5]) {
			ret = -EBADMSG;
			goto retry;
		}

		measurement->raw_temperature = ((__u16)data[0] << 8) | data[1];
		measurement->raw_humidity = ((__u16)data[3] << 8) | data[4];
		return 0;

	retry:
		if (attempt + 1 < SHT30_RETRY_COUNT)
			/* 等待上一次测量完全结束，再开始下一次重试。 */
			usleep_range(SHT30_RETRY_DELAY_US_MIN,
				     SHT30_RETRY_DELAY_US_MAX);
	}

	dev_err(&sht30->client->dev,
		"SHT30 测量失败，已重试 %d 次，错误码=%d\n",
		SHT30_RETRY_COUNT, ret);
	return ret;
}

static int sht30_open(struct inode *inode, struct file *file)
{
	/* 根据 cdev 地址反推出当前 SHT30 设备实例。 */
	struct sht30_device *sht30 = container_of(inode->i_cdev,
						  struct sht30_device, cdev);

	/* 后续 read() 通过 private_data 获取当前设备。 */
	file->private_data = sht30;
	return 0;
}

static ssize_t sht30_read(struct file *file, char __user *buf,
			  size_t count, loff_t *ppos)
{
	struct sht30_device *sht30 = file->private_data;
	struct sht30_measurement measurement;
	int ret;
	(void)ppos;

	if (count < sizeof(measurement))
		return -EINVAL;

	if (mutex_lock_interruptible(&sht30->lock))
		return -ERESTARTSYS;

	ret = sht30_measure(sht30, &measurement);
	mutex_unlock(&sht30->lock);
	if (ret)
		return ret;

	/* 浮点换算不放在内核驱动中，用户态读取两个原始 16 位值。 */
	if (copy_to_user(buf, &measurement, sizeof(measurement)))
		return -EFAULT;

	return sizeof(measurement);
}

static const struct file_operations sht30_fops = {
	.owner = THIS_MODULE,
	.open = sht30_open,
	.read = sht30_read,
	.llseek = no_llseek,
};

static int sht30_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct sht30_device *sht30;
	int ret;
	(void)id;

	/* 确认当前 I2C 控制器支持普通 I2C 读写。 */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		return -EOPNOTSUPP;

	sht30 = devm_kzalloc(&client->dev, sizeof(*sht30), GFP_KERNEL);
	if (!sht30)
		return -ENOMEM;

	sht30->client = client;
	sht30->devt = sht30_devt;
	sht30->class = sht30_class;
	mutex_init(&sht30->lock);

	/* 把设备私有结构体保存到 client，remove() 可以取回。 */
	i2c_set_clientdata(client, sht30);

	/* 绑定 open/read 文件操作。 */
	cdev_init(&sht30->cdev, &sht30_fops);
	sht30->cdev.owner = THIS_MODULE;

	ret = cdev_add(&sht30->cdev, sht30->devt, 1);
	if (ret) {
		dev_err(&client->dev, "添加 SHT30 cdev 失败：%d\n", ret);
		return ret;
	}

	/* 创建用户态设备节点 /dev/sht30。 */
	sht30->chardev = device_create(sht30->class, &client->dev,
				       sht30->devt, sht30, SHT30_NAME);
	if (IS_ERR(sht30->chardev)) {
		ret = PTR_ERR(sht30->chardev);
		cdev_del(&sht30->cdev);
		dev_err(&client->dev, "创建 /dev/sht30 失败：%d\n", ret);
		return ret;
	}

	dev_info(&client->dev, "SHT30 驱动探测成功，I2C 地址为 0x%02x\n",
		 client->addr);
	return 0;
}

static int sht30_remove(struct i2c_client *client)
{
	struct sht30_device *sht30 = i2c_get_clientdata(client);

	device_destroy(sht30->class, sht30->devt);
	cdev_del(&sht30->cdev);
	return 0;
}

static const struct of_device_id sht30_of_match[] = {
	{ .compatible = "study-wg,sht30" },
	{ }
};
MODULE_DEVICE_TABLE(of, sht30_of_match);

static const struct i2c_device_id sht30_id[] = {
	{ SHT30_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sht30_id);

static struct i2c_driver sht30_driver = {
	.driver = {
		.name = SHT30_NAME,
		.of_match_table = sht30_of_match,
	},
	.probe = sht30_probe,
	.remove = sht30_remove,
	.id_table = sht30_id,
};

static int __init sht30_init(void)
{
	int ret;

	/* 第一步：申请一个字符设备号。 */
	ret = alloc_chrdev_region(&sht30_devt, 0, 1, SHT30_NAME);
	if (ret)
		return ret;

	/* 第二步：创建 class，供 probe() 创建 /dev/sht30。 */
	sht30_class = class_create(THIS_MODULE, SHT30_NAME);
	if (IS_ERR(sht30_class)) {
		ret = PTR_ERR(sht30_class);
		unregister_chrdev_region(sht30_devt, 1);
		return ret;
	}

	/* 第三步：注册 I2C 驱动，随后由设备模型自动匹配并调用 probe()。 */
	ret = i2c_add_driver(&sht30_driver);
	if (ret) {
		class_destroy(sht30_class);
		unregister_chrdev_region(sht30_devt, 1);
	}

	return ret;
}

static void __exit sht30_exit(void)
{
	/* i2c_del_driver() 会先调用 remove()，再销毁 class 和设备号。 */
	i2c_del_driver(&sht30_driver);
	class_destroy(sht30_class);
	unregister_chrdev_region(sht30_devt, 1);
}

module_init(sht30_init);
module_exit(sht30_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("study-w-g");
MODULE_DESCRIPTION("SHT30 I2C 温湿度字符设备驱动");
