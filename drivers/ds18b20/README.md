# DS18B20 Driver

这是 DS18B20 1-Wire 字符设备驱动的初始版本，目标设备节点为 `/dev/ds18b20`。

## 当前内容

- [x] GPIO 申请与释放
- [x] 1-Wire 基本复位、写字节、读字节流程
- [x] 字符设备注册
- [x] `read()` 接口
- [x] 互斥锁保护读取流程
- [ ] 设备树 DTS 节点
- [ ] 原始温度值转换为摄氏温度
- [ ] Scratchpad CRC 校验
- [ ] 传感器不存在和超时检测
- [ ] 阻塞读取和 `poll()` 通知
- [ ] 温度阈值 `ioctl`
- [ ] 周期采集工作队列

## 重要说明

当前驱动源码是开发中的初始版本，必须结合目标内核版本和实际硬件测试后再标记为完成。特别是 1-Wire 时序、GPIO 释放时机、温度转换、CRC 和设备树路径需要在开发板上验证。

## 设备树接口

驱动当前查找：

```text
/ds18b20
```

并读取节点属性：

```dts
ds_gpio = <&gpioX Y GPIO_ACTIVE_HIGH>;
```

实际 GPIO 编号和设备树写法需要根据开发板内核版本确认。

## 后续编译

需要在仓库补充适配目标内核的 Makefile 后再编译，例如：

```bash
make -C /path/to/linux-kernel \
    ARCH=arm \
    CROSS_COMPILE=arm-linux-gnueabihf- \
    M=$PWD modules
```

不要把生成的 `.o`、`.ko`、`.mod.*` 等文件提交到 GitHub。
