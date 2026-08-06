# 用户态网关服务

`gateway-manager` 是智能网关的 C 后端，负责把内核驱动提供的原始数据组织成业务数据。

## 分层职责

```text
/dev/ds18b20       DS18B20 原始温度
/dev/sht30         SHT30 原始温湿度
/dev/gpio-event    按键事件和 LED 控制
       ↓
gateway-manager    换算、状态管理、阈值业务、MQTT、Unix Socket
       ↓                    ↓
MQTT 云端             Qt 本地界面
```

## 本地接口

服务创建 Unix Socket：

```text
/tmp/smart-gateway.sock
```

支持的命令：

```text
status       获取一行 JSON 状态
led 0        关闭 LED
led 1        打开 LED
```

MQTT 命令主题由 `configs/mqtt.conf` 的 `topic_command` 指定，当前支持：

```text
led 0
led 1
{"led":1}
```

服务会在 MQTT 断线后保留本地采集功能，并每隔约 5 秒尝试重连；重连策略仍需在真实 broker 和目标网络上验证。

启动方式可以选择 BusyBox 启动脚本模板 `rootfs/etc/init.d/S99smart-gateway.example`，或参考 `gateway-manager.service.example` 配置 systemd 服务。

## 构建依赖

目标 Rootfs 需要：

- C 运行库和 pthread；
- `libmosquitto` 头文件与运行库；
- DS18B20、SHT30、GPIO 驱动已经加载；
- `configs/mqtt.conf` 已从示例文件复制并填写。

当前代码是可学习的基础版本，传感器断开时保留上一状态并通过有效标志区分“无效数据”；真实板端还需要根据 Rootfs 的库版本调整交叉编译参数。
