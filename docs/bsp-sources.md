# BSP Source Links

本项目不把正点原子完整 BSP 压缩包复制到 GitHub。U-Boot、Linux Kernel、设备树、Rootfs 和工具链请从板卡资料包中获取；本仓库只保存版本记录、构建入口、补丁和项目自己的驱动/应用代码。

## Official entry points

- [正点原子官网](https://www.alientek.com/)
- [正点原子资料下载/技术资料入口](https://www.alientek.com/)

进入官网后搜索以下关键词：

```text
I.MX6U ALPHA V2.2
I.MX6U ALPHA
i.MX6ULL Linux BSP
```

下载时优先选择与开发板硬件版本一致的资料包，并记录下载日期、文件名和校验值。

## Local placement

```text
smart-gateway-imx6ull-linux/
└── external/                 # 本地使用，已被 .gitignore 忽略
    ├── u-boot-atk/
    ├── linux-atk/
    └── buildroot-or-busybox/
```

## Version record

下载资料后，把实际信息填写到下表；不要把整个压缩包提交到公开仓库。

| Component | Local source | Version/tag | SHA256 | Notes |
|---|---|---|---|---|
| U-Boot | `external/u-boot-atk` | TODO | TODO | board-specific |
| Linux Kernel | `external/linux-atk` | TODO | TODO | board-specific |
| Rootfs | `external/buildroot-or-busybox` | TODO | TODO | development/final image |
| Toolchain | local toolchain path | TODO | TODO | prefix TODO |

## Why only links are stored

- 避免重复镜像大型第三方源码包。
- 避免把厂商资料、工具链或受授权限制的内容公开分发。
- 保持仓库轻量，便于面试官查看项目自己的驱动代码和文档。
- 通过版本号和 SHA256 保证本地 BSP 可追溯。

## Current project boundary

你已经在开发板上完成内核、U-Boot 和 Rootfs 的移植；下一步只需把实际版本信息回填到本文件和 `configs/board.env`。本仓库暂不添加 DS18B20 设备树节点。
