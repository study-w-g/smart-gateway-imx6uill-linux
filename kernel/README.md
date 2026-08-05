# Linux Kernel

将正点原子 I.MX6U ALPHA V2.2 配套 Linux 内核源码放在本地 `external/linux-atk/`，不要直接复制整个内核到本项目。

在确认厂家 BSP 后，补充以下信息：

- 内核版本
- 对应 commit/tag
- 默认 defconfig
- 匹配的板级 DTS 文件名
- 工具链前缀

项目自己的驱动源码放在 `drivers/`，通过 `drivers/ds18b20/Makefile` 作为外部模块编译。

当前不添加 DS18B20 设备树节点。
