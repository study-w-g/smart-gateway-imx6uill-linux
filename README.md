# i.MX6ULL Embedded Linux Smart Gateway

基于 NXP i.MX6ULL 和嵌入式 Linux 的智能网关项目，目标是打通“外设驱动 - 用户态服务 - 本地监控 - 云端通信”的完整链路。

> 当前仓库处于工程初始化阶段。目录和设计文档已建立，驱动与应用功能将按 `docs/roadmap.md` 逐项实现。未完成的功能不会标记为已完成。

## 项目目标

- DS18B20 1-Wire 温度采集与字符设备接口
- SHT30 I2C 温湿度采集
- GPIO 按键中断、消抖与 LED 状态控制
- 设备树、U-Boot、Linux Kernel、Rootfs 构建与部署
- LX-16A 串口舵机控制
- QT 本地监控界面
- MQTT 网关与云端双向通信
- 断线重连、传感器异常和设备重启恢复

## 系统架构

```text
DS18B20 / SHT30 / GPIO / UART
            |
       Linux 驱动层
            |
      /dev、sysfs、poll
            |
     gateway-manager 服务
        /           \\
   QT 本地界面       MQTT 云端
            |
       LED / 舵机控制
```

## 目录说明

```text
.
├── app/                 用户态服务和测试程序
├── configs/             内核、BusyBox、MQTT 配置示例
├── device-tree/         设备树源文件和说明
├── docs/                硬件、架构、驱动、构建和测试文档
├── drivers/             Linux 外设驱动
│   ├── ds18b20/         DS18B20 1-Wire 驱动（待实现）
│   ├── gpio_event/      GPIO 按键/LED 驱动（待实现）
│   └── sht30/            SHT30 I2C 驱动（待实现）
├── qt/                  QT 本地监控界面（待实现）
├── scripts/             构建、部署和日志采集脚本
├── tests/               用户态测试和验收用例
└── Makefile
```

## 硬件平台

| 模块 | 计划用途 | 当前状态 |
|---|---|---|
| NXP i.MX6ULL | 主控平台 | 待补充具体开发板型号 |
| DS18B20 | 1-Wire 温度采集 | 待实现 |
| SHT30 | I2C 温湿度采集 | 待实现 |
| GPIO 按键/LED | 输入事件与状态指示 | 待实现 |
| LX-16A | UART 串口舵机 | 待实现 |
| USB Wi-Fi | 网关联网 | 待硬件型号确认 |

引脚、总线号、设备地址和串口参数统一记录在 [docs/hardware.md](docs/hardware.md)。

## 开发路线

1. 确认硬件连接和交叉编译环境
2. 完成 U-Boot、Kernel、设备树和 Rootfs 最小系统启动
3. 完成 DS18B20 字符设备驱动
4. 完成 SHT30 I2C 驱动
5. 完成 GPIO 按键/LED 驱动
6. 完成 UART 舵机控制和用户态网关服务
7. 接入 MQTT 和 QT 界面
8. 完成异常恢复、测试记录和演示材料

详细任务和验收标准见 [docs/roadmap.md](docs/roadmap.md)。

## 当前使用方式

```bash
make help
make check
```

当前顶层 Makefile 负责检查目录和显示开发入口；驱动和应用完成后，再逐步接入交叉编译目标。

## 安全提醒

- 不要提交阿里云 AccessKey、MQTT 密码、Wi-Fi 密码或私钥。
- 只提交 `configs/mqtt.conf.example` 等脱敏配置模板。
- 不要提交完整交叉编译工具链、商业 SDK 或大型 Rootfs 压缩包。

## 项目状态

详见 [docs/status.md](docs/status.md)。

## License

本项目代码建议使用 MIT License；加入第三方代码时请遵循其原始许可证。
