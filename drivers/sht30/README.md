# SHT30 I2C 温湿度驱动

本目录提供 SHT30 的学习版 Linux I2C 字符设备驱动，目标设备节点为 `/dev/sht30`。

## 当前实现

- 使用 `struct i2c_driver`，通过设备树 `compatible` 自动匹配；
- 使用 `i2c_master_send()` 发送单次高重复性测量命令 `0x24 0x00`；
- 等待测量完成后读取 6 字节数据；
- 对温度和湿度分别执行 SHT30 CRC-8 校验；
- 对一次完整测量流程使用互斥锁保护；
- I2C 传输或 CRC 失败时最多重试 3 次；
- 通过 `/dev/sht30` 返回温度和湿度原始值。

## 用户态数据格式

一次 `read()` 返回 4 字节，布局为两个大端原始数据解析后的 `__u16`：

```c
struct sht30_measurement {
    __u16 raw_temperature;
    __u16 raw_humidity;
};
```

用户态换算：

```c
temperature = -45.0f + 175.0f * raw_temperature / 65535.0f;
humidity = 100.0f * raw_humidity / 65535.0f;
```

驱动不返回浮点数，也不在内核中执行浮点运算。

## 设备树接口

驱动匹配字符串为：

```dts
compatible = "study-wg,sht30";
reg = <0x44>;
```

`reg` 应根据硬件实际地址填写为 `0x44` 或 `0x45`。本仓库暂不添加具体 I2C 总线、地址和引脚的设备树节点，必须先根据开发板原理图确认。

## 编译

```bash
make -C /path/to/linux-kernel \
    ARCH=arm \
    CROSS_COMPILE=arm-linux-gnueabihf- \
    M=$PWD modules
```

也可以使用本目录 Makefile：

```bash
make KERNEL_DIR=/path/to/linux-kernel \
     ARCH=arm \
     CROSS_COMPILE=arm-linux-gnueabihf-
```

当前代码需要在你的实际厂家内核源码上编译，特别是 `class_create()`、`i2c_driver.probe/remove` 等接口可能随内核版本变化。
