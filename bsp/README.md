# i.MX6U ALPHA V2.2 板级支持包

本目录保存与正点原子 I.MX6U ALPHA V2.2 开发板相关的构建入口、版本记录和补丁。

## 重要说明

不要把未经厂家资料验证的“通用 i.MX6ULL 内核”当作本板完整 BSP。板级 BSP 至少包含：

- 与开发板匹配的 U-Boot 配置和板级代码
- 与开发板匹配的 Linux 内核源码、设备树和补丁
- 内核默认配置
- Rootfs 构建配置
- 启动介质、DDR、引脚复用和网络配置

本仓库先保存可复现的版本和构建入口，不直接复制完整第三方源码。请从正点原子随板资料或官方授权下载页取得 BSP，并根据 `configs/board.env.example` 配置本地路径。

## 建议的本地目录

```text
external/
├── u-boot-atk/
├── linux-atk/
└── buildroot-or-busybox/
```

这些源码默认不提交到本仓库；`.gitignore` 会忽略 `external/`。

## 当前范围

- 目标板：正点原子 I.MX6U ALPHA V2.2
- 架构：ARM 32-bit
- 设备树：暂不添加 DS18B20 节点
- 内核版本：等待确认厂家 BSP 资料后固定
- Rootfs：先采用 BusyBox/NFS 开发方式，后续再制作可烧录镜像

## BSP 确认清单

- [ ] 找到正点原子 I.MX6U ALPHA V2.2 对应的资料包
- [ ] 确认 U-Boot 版本和 defconfig
- [ ] 确认 Linux 内核版本和板级 DTS 文件
- [ ] 确认交叉编译器前缀
- [ ] 确认启动介质：SD、EMMC 或 NAND
- [ ] 确认 TFTP/NFS 开发参数
- [ ] 完成第一次串口启动并保存日志
