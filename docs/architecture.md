# System Architecture

```text
Sensor/GPIO/UART
      |
Device tree + Linux driver
      |
/dev、sysfs、poll/event
      |
gateway-manager (user space)
      +--> QT local monitor
      +--> MQTT telemetry and command channel
      +--> LED / LX-16A control
```

## Responsibilities

| Layer | Responsibility |
|---|---|
| Device tree | Describe buses, GPIOs, addresses, pinctrl and hardware relationships |
| Kernel driver | Translate Linux APIs to bus transactions, interrupts and device state |
| User-space service | Combine sensor data, apply thresholds, handle events and recover failures |
| QT UI | Display data and issue local control commands |
| MQTT client | Upload telemetry and receive remote commands |

## Design rules

1. Hardware access stays below the user-space service boundary.
2. The UI does not access registers or GPIOs directly.
3. Driver interfaces return explicit error codes.
4. Blocking reads and `poll()` are used for event-driven data flow instead of busy loops.
5. Every externally controlled command is validated before execution.
