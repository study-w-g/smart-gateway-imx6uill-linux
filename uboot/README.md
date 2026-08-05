# U-Boot 启动程序

将正点原子 I.MX6U ALPHA V2.2 配套 U-Boot 源码放在本地 `external/u-boot-atk/`，不要直接复制整个 U-Boot 源码到本项目。

确认 BSP 后记录：

- U-Boot 版本
- 板级 defconfig
- 启动介质
- 内核、DTB 和根文件系统的加载地址
- TFTP/NFS 启动环境变量

U-Boot 负责在启动阶段加载内核、DTB，并把设备树地址传递给 Linux；NFS 不会自动修改 DTB。
