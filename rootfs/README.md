# 根文件系统

开发阶段建议先使用 BusyBox + NFS Rootfs，便于快速更新用户态测试程序和日志；功能稳定后再制作 SD/eMMC/NAND 可烧录镜像。

## 本地源码位置

```text
external/buildroot-or-busybox/
```

## 根文件系统的最小内容

- `/bin/sh`
- `mount`、`cp`、`cat`、`dmesg`、`insmod`、`rmmod`
- 带有 devtmpfs 的 `/dev`
- `/proc` 和 `/sys`
- 网络工具
- DS18B20 用户态测试程序

具体 BusyBox 配置、NFS 导出路径和启动脚本需要在板卡首次启动成功后补充。

启动脚本模板见 `rootfs/etc/init.d/S99smart-gateway.example`。不要在未确认模块路径、设备树节点和 Rootfs 库版本前直接启用脚本。
