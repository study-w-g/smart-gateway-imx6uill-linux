# Linux 外设驱动

本目录包含项目自己的 Linux 外设驱动源码。每个驱动都采用独立目录和 Makefile，使用目标板正在运行的厂家内核源码编译为外部模块。

| 驱动 | Linux 框架 | 用户态接口 |
|---|---|---|
| DS18B20 | `platform_driver`、设备树 GPIO | `/dev/ds18b20` |
| SHT30 | `i2c_driver`、设备树 I2C | `/dev/sht30` |
| GPIO 按键/LED | `platform_driver`、GPIO IRQ、工作队列 | `/dev/gpio-event` |

驱动源码中的 `compatible` 只负责匹配，具体总线编号、GPIO、I2C 地址和 pinctrl 必须由实际板卡设备树提供。
