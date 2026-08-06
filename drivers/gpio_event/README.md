# GPIO 按键/LED 驱动

本目录实现一个 GPIO 按键和 LED 的 platform 驱动，目标设备节点为 `/dev/gpio-event`。

用户态和内核共用的接口定义在 `gpio_event_uapi.h`，它包含按键事件结构体和 LED 的 `ioctl` 编号。

## 调用流程

```text
按键电平变化
    ↓
GPIO IRQ
    ↓
延迟工作消抖
    ↓
读取稳定电平
    ↓
事件环形队列
    ↓
read()/poll() 通知用户态
```

LED 通过 `ioctl()` 控制：

```c
#define GPIO_EVENT_IOC_SET_LED _IOW('G', 0x01, __u32)
#define GPIO_EVENT_IOC_GET_LED _IOR('G', 0x02, __u32)
```

## 设备树属性

驱动匹配字符串：

```dts
compatible = "study-wg,gpio-event";
key-gpios = <...>;
led-gpios = <...>;
```

具体 GPIO、有效电平和 pinctrl 必须根据开发板原理图确认，本仓库暂不填写具体编号。
