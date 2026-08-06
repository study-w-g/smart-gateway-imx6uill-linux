# 用户态程序

## 目录

| 目录 | 内容 |
|---|---|
| `gateway_manager/` | 读取驱动、温湿度换算、阈值业务、Unix Socket 和 MQTT |
| `tests/` | DS18B20、SHT30、GPIO 驱动接口测试程序 |
| `servo_control/` | LX-16A 串口配置和协议帧控制库 |

用户态程序不直接访问硬件寄存器。传感器和按键通过 `/dev` 接口访问，串口舵机通过 Linux `termios` 配置 UART。

## 推荐顺序

```text
先运行 app/tests
       ↓
确认各个 /dev 接口
       ↓
运行 gateway-manager
       ↓
检查 Unix Socket 返回的 JSON
       ↓
再启用 MQTT 和 Qt
```
