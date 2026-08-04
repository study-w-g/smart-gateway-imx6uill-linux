# smart-gateway-imx6uill-linux
# i.MX6ULL Embedded Linux Smart Gateway

基于 NXP i.MX6ULL 和嵌入式 Linux 的智能网关项目。

## 项目功能

- DS18B20 1-Wire 温度传感器驱动
- GPIO 按键与 LED 控制
- SHT30 I2C 温湿度传感器驱动
- 设备树配置
- U-Boot、Linux Kernel、Rootfs 构建
- QT 本地监控界面
- MQTT 云端通信
- LX-16A 串口舵机控制

## 硬件平台

- NXP i.MX6ULL
- DS18B20
- SHT30
- GPIO 按键与 LED
- LX-16A 舵机
- USB Wi-Fi 模块

## 软件环境

- Embedded Linux
- Linux Kernel
- C/C++
- QT
- MQTT
- TFTP/NFS

## 项目架构

传感器/按键
→ Linux 驱动
→ /dev 设备节点
→ 用户态应用
→ QT / MQTT
→ 本地或云端控制

## 编译与运行

```bash
make
make install
