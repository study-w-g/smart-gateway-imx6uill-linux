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
