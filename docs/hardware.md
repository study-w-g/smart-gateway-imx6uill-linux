# Hardware Definition

这份文档是设备树和驱动开发的硬件参数入口。完成实物确认后再填写具体值。

## Board

| Item | Value |
|---|---|
| SoC | NXP i.MX6ULL |
| Board model | TODO: fill actual board model |
| Kernel version | TODO |
| Cross compiler | TODO |
| Rootfs method | NFS during development / TODO for final image |

## Peripheral map

| Device | Bus/resource | Address or GPIO | Direction | Status |
|---|---|---|---|---|
| DS18B20 | 1-Wire GPIO | TODO | input/output | to be verified |
| SHT30 | I2C bus | TODO, normally 0x44 or 0x45 | input | to be verified |
| User key | GPIO interrupt | TODO | input | to be verified |
| Status LED | GPIO | TODO | output | to be verified |
| LX-16A | UART | TODO | TX/RX | to be verified |
| USB Wi-Fi | USB host | TODO chipset | network | to be verified |

## Bring-up checklist

- [ ] Confirm schematic and connector pinout.
- [ ] Confirm GPIO numbering and active level.
- [ ] Confirm I2C bus number and SHT30 address with `i2cdetect`.
- [ ] Confirm UART level, baud rate and wiring.
- [ ] Record logic-analyzer evidence for uncertain timings.
- [ ] Update the device tree only after values are confirmed.
