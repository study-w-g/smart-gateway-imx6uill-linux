# Driver Design

## DS18B20 1-Wire character device

Planned interface: `/dev/ds18b20`

- Device-tree based GPIO resource discovery
- Reset, ROM command, temperature conversion and scratchpad read
- CRC validation and timeout/retry handling
- Periodic sampling using delayed work or a workqueue
- Mutex-protected latest-value cache
- Blocking `read()` and `poll()` notification for new samples
- `ioctl()` for sampling period and threshold configuration

## SHT30 I2C driver

Planned framework: `struct i2c_driver`

- `of_match_table` and `probe/remove`
- I2C command write and measurement read
- Temperature/humidity conversion and CRC check
- Bounded retry on I2C transfer failure
- Initial user-space interface: sysfs or a small character device

## GPIO key and LED

- GPIO descriptor acquisition from device tree
- IRQ for key edge detection
- Workqueue-based debounce in process context
- Event queue plus `poll()` for user-space notification
- LED states for boot, network, normal, and sensor-fault conditions

## Concurrency model

| Situation | Primitive |
|---|---|
| Protect cached sensor data | mutex |
| Wait for new sample/event | wait queue |
| Periodic work and debounce | delayed work/workqueue |
| Fast interrupt bookkeeping | spinlock only when required by context |
| User-space event notification | `poll()` plus event buffer |
