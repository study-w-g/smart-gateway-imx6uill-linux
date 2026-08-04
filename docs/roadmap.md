# Development Roadmap

## Phase 0 - Hardware and toolchain

- [ ] Confirm board model, kernel version and cross compiler.
- [ ] Confirm every bus, GPIO, UART and sensor address.
- [ ] Record the first successful serial boot.

## Phase 1 - Minimal Linux system

- [ ] Build U-Boot, kernel, device tree and Rootfs.
- [ ] Boot through TFTP/NFS.
- [ ] Bring up USB Wi-Fi and verify network access.

## Phase 2 - Drivers

- [ ] DS18B20 protocol and character device.
- [ ] DS18B20 periodic work, blocking read and `poll()`.
- [ ] SHT30 I2C driver, conversion and CRC.
- [ ] GPIO key interrupt, debounce and event queue.
- [ ] LED state control.

## Phase 3 - User space

- [ ] Sensor service with explicit error handling.
- [ ] LX-16A UART control.
- [ ] MQTT telemetry, command parsing and reconnect.
- [ ] QT monitor.

## Phase 4 - Validation and presentation

- [ ] Sensor disconnect tests.
- [ ] I2C/UART/network failure recovery.
- [ ] Clean build from documented instructions.
- [ ] Add test logs and screenshots without credentials.
- [ ] Update README status and resume wording.

## Suggested commits

```text
Initialize project skeleton
Add hardware and architecture documentation
Add i.MX6ULL build notes
Add DS18B20 driver
Add SHT30 I2C driver
Add GPIO event and LED driver
Add gateway manager
Add MQTT communication
Add test report and evidence
```
