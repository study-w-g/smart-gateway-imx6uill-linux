# Root Filesystem

开发阶段建议先使用 BusyBox + NFS Rootfs，便于快速更新用户态测试程序和日志；功能稳定后再制作 SD/eMMC/NAND 可烧录镜像。

## Local source location

```text
external/buildroot-or-busybox/
```

## Rootfs minimum content

- `/bin/sh`
- `mount`、`cp`、`cat`、`dmesg`、`insmod`、`rmmod`
- `/dev` with devtmpfs
- `/proc` and `/sys`
- network tools
- DS18B20 user-space test program

具体 BusyBox 配置、NFS 导出路径和启动脚本需要在板卡首次启动成功后补充。
