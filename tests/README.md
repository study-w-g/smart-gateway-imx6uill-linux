# 测试

每个驱动都应配套一个用户态测试程序和硬件验收记录。

计划测试：

- `test_ds18b20`：正负温度、超时、CRC 和 `poll()`。
- `test_sht30`：I2C 发现、测量、CRC 和重试。
- `test_gpio`：按键中断、消抖、事件读取和 LED 状态。
- `test_servo`：UART 校验和、位置控制、超时和状态读取。
- `test_mqtt`：发布、订阅、重连和非法命令拒绝。

测试命令和脱敏输出统一记录在 `docs/evidence/`。
