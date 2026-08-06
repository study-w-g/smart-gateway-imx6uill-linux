# 构建与部署

## 主机准备

在补充构建脚本前记录以下参数：

```text
BOARD=
KERNEL_DIR=
UBOOT_DIR=
CROSS_COMPILE=
TFTP_DIR=
NFS_ROOT=
SERIAL_DEVICE=
```

不要提交与个人电脑相关的绝对路径。

### 编译环境要求

用户态代码包含 Linux/POSIX 头文件，SHT30 依赖 Linux 内核头文件，MQTT 代码依赖 Mosquitto 开发库。因此不要使用 Windows MinGW 直接验证完整工程；应使用 Linux 主机、虚拟机、WSL 或厂家提供的交叉编译环境，并准备：

- ARM 交叉编译器，例如 `arm-linux-gnueabihf-gcc`；
- 与开发板正在运行的内核完全对应的内核源码和 `.config`；
- Qt 交叉编译工具链；
- 目标 Rootfs 对应的 `libmosquitto` 头文件和库；
- 与目标 Rootfs 匹配的 C 运行库。

## 计划中的构建流程

```bash
# 为实际开发板构建 U-Boot
make <board>_defconfig
make CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc)

# 构建 Linux 内核和设备树
make <board>_defconfig
make CROSS_COMPILE=${CROSS_COMPILE} zImage dtbs -j$(nproc)

# 使用 BusyBox 或 Buildroot 构建根文件系统。
# 将 zImage、dtb 和根文件系统复制到开发服务器。
# 通过 TFTP 启动，并通过 NFS 挂载根文件系统。
```

确认开发板型号后，再补充准确的开发板默认配置和部署命令。

## 开发板端检查

```bash
uname -a
cat /proc/device-tree/model
dmesg | tail -n 100
ip addr
ls /dev
```

## 第一次启动的验证记录

将脱敏后的 U-Boot/内核日志、驱动探测日志、网络结果和传感器测试输出保存到 `docs/evidence/`。

## 本项目代码构建顺序

以下命令在 Linux 开发主机或交叉编译环境执行；Windows 主机可通过虚拟机、WSL 或厂家提供的 Linux 环境执行。

```bash
# 1. 使用正在运行的厂家内核源码分别构建内核模块
make -C drivers/ds18b20 KERNEL_DIR=/path/to/linux \
    ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
make -C drivers/sht30 KERNEL_DIR=/path/to/linux \
    ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
make -C drivers/gpio_event KERNEL_DIR=/path/to/linux \
    ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-

# 2. 构建用户态驱动测试程序
make -C app/tests CC=arm-linux-gnueabihf-gcc

# 3. 构建 C 网关服务，目标 Rootfs 需要 libmosquitto
make -C app/gateway_manager \
    CC=arm-linux-gnueabihf-gcc

# 4. 构建 Qt 界面
cd qt && qmake gateway-monitor.pro && make
```

## 启动顺序

```text
加载内核模块
    ↓
确认 /dev/ds18b20、/dev/sht30、/dev/gpio-event
    ↓
运行 app/tests 下的独立测试程序
    ↓
复制并填写 configs/mqtt.conf
    ↓
启动 gateway-manager
    ↓
启动 Qt 监控界面
```

驱动源码能编译不等于硬件已经正常工作；必须保存 `dmesg`、设备节点、测试输出和传感器断开测试记录。
