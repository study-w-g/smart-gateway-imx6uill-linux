# Device Tree

本目录将在确认具体 i.MX6ULL 开发板和引脚映射后存放 DTS/DTSI 文件。

先填写 [../docs/hardware.md](../docs/hardware.md)，再编写设备树。不要在未核对原理图的情况下直接使用通用 GPIO 编号。

当前不添加任何 DS18B20 设备节点。等实际 GPIO、上拉电阻和 pinctrl 参数确认后，再在对应的板级 DTS/DTSI 中添加节点。
