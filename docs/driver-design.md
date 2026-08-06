# 驱动设计

## DS18B20 1-Wire 字符设备

计划接口：`/dev/ds18b20`

- 基于设备树获取 GPIO 资源
- 完成复位、ROM 命令、温度转换和暂存器读取
- Scratchpad CRC 校验以及温度转换超时处理
- 使用延迟工作或工作队列进行周期采样
- 使用互斥锁保护最新数据缓存
- 使用阻塞式 `read()` 和 `poll()` 通知新采样值
- 使用 `ioctl()` 配置采样周期和温度阈值

## SHT30 I2C 驱动

计划框架：`struct i2c_driver`

- 使用 `of_match_table` 和 `probe/remove`
- 完成 I2C 命令写入和测量数据读取
- 完成温度/湿度转换和 CRC 校验
- I2C 传输失败时进行有次数上限的重试
- 初始用户态接口采用 sysfs 或小型字符设备

## GPIO 按键和 LED

- 从设备树获取 GPIO 描述符
- 使用 IRQ 检测按键边沿
- 在进程上下文中使用工作队列消抖
- 使用事件队列和 `poll()` 向用户态通知
- 用 LED 表示启动、网络正常、运行正常和传感器故障状态

## 当前代码对应关系

| 功能 | 源码 | 接口 |
|---|---|---|
| DS18B20 | `drivers/ds18b20/ds18b20.c` | `/dev/ds18b20` |
| SHT30 | `drivers/sht30/sht30.c` | `/dev/sht30` |
| GPIO 按键/LED | `drivers/gpio_event/gpio_event.c` | `/dev/gpio-event` |
| C 后端 | `app/gateway_manager/` | Unix Socket、MQTT |
| Qt 前端 | `qt/` | 本地监控 |

## 并发模型

| 场景 | 使用的同步机制 |
|---|---|
| 保护缓存的传感器数据 | mutex |
| 等待新采样值或事件 | 等待队列 |
| 周期任务和按键消抖 | 延迟工作/工作队列 |
| 快速中断上下文记录 | 仅在上下文要求时使用自旋锁 |
| 用户态事件通知 | `poll()` 加事件缓冲区 |
