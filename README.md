# i.MX6ULL 嵌入式 Linux 智能网关

基于 NXP i.MX6ULL 和嵌入式 Linux 的智能网关项目，目标是打通“外设驱动 - 用户态服务 - 本地监控 - 云端通信”的完整链路。

> 当前仓库已经补充驱动、用户态、Qt、MQTT 和测试代码，但仍需结合你的厂家内核、设备树和真实硬件完成编译与板端验证。未经板端验证的功能不会标记为最终完成。

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
│   ├── gateway_manager/ 网关 C 后端、MQTT 和本地 Socket
│   ├── servo_control/   LX-16A 串口舵机控制库
│   └── tests/           驱动接口测试程序
├── configs/             内核、BusyBox、MQTT 配置示例
├── device-tree/         设备树源文件和说明
├── docs/                硬件、架构、驱动、构建和测试文档
├── drivers/             Linux 外设驱动
│   ├── ds18b20/         DS18B20 1-Wire 驱动
│   ├── gpio_event/      GPIO 按键/LED 驱动
│   └── sht30/            SHT30 I2C 驱动
├── qt/                  Qt 本地监控界面
├── scripts/             构建、部署和日志采集脚本
├── tests/               测试说明和验收用例
└── Makefile
```

## 硬件平台

| 模块 | 计划用途 | 当前状态 |
|---|---|---|
| NXP i.MX6ULL | 主控平台 | 正点原子 I.MX6U ALPHA V2.2 |
| DS18B20 | 1-Wire 温度采集 | 源码已加入，待板端验证 |
| SHT30 | I2C 温湿度采集 | 源码已加入，待板端验证 |
| GPIO 按键/LED | 输入事件与状态指示 | 源码已加入，待板端验证 |
| LX-16A | UART 串口舵机 | 基础控制库已加入，待板端验证 |
| USB Wi-Fi | 网关联网 | 待硬件型号确认 |

引脚、总线号、设备地址和串口参数统一记录在 [docs/hardware.md](docs/hardware.md)。

## 开发路线

1. 确认硬件连接和交叉编译环境
2. 完成 U-Boot、Kernel、设备树和 Rootfs 最小系统启动
3. 编译并验证 DS18B20 字符设备驱动
4. 编译并验证 SHT30 I2C 驱动
5. 编译并验证 GPIO 按键/LED 驱动
6. 验证 UART 舵机控制和用户态网关服务
7. 验证 MQTT 和 Qt 界面
8. 完成异常恢复、测试记录和演示材料

详细任务和验收标准见 [docs/roadmap.md](docs/roadmap.md)。

完整学习流程见 [docs/learning-flow.md](docs/learning-flow.md)。

## 当前使用方式

```bash
make help
make check
```

当前顶层 Makefile 负责检查目录和显示开发入口。各驱动、用户态程序和 Qt 界面分别在对应目录构建，具体见 [docs/build-and-deploy.md](docs/build-and-deploy.md)。

## 安全提醒

- 不要提交阿里云 AccessKey、MQTT 密码、Wi-Fi 密码或私钥。
- 只提交 `configs/mqtt.conf.example` 等脱敏配置模板。
- 不要提交完整交叉编译工具链、商业 SDK 或大型 Rootfs 压缩包。

## 项目状态

详见 [docs/status.md](docs/status.md)。

目标板 BSP 入口见 [bsp/README.md](bsp/README.md) 和 [docs/bsp-bringup.md](docs/bsp-bringup.md)。当前不包含 DS18B20 设备树节点。

BSP 官方资料入口和版本记录见 [docs/bsp-sources.md](docs/bsp-sources.md)。学习流程图见 [docs/learning-flow.md](docs/learning-flow.md)。

## 许可证

本项目代码建议使用 MIT 许可证；加入第三方代码时请遵循其原始许可证。
