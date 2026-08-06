# Qt 本地监控界面

Qt 界面通过 Unix Socket 与 `gateway-manager` 通信，不直接打开内核设备节点。

## 数据流

```text
Qt 定时发送 status
        ↓
/tmp/smart-gateway.sock
        ↓
gateway-manager 返回 JSON
        ↓
Qt 解析并更新标签
```

## 构建

目标板安装 Qt 开发环境后执行：

```bash
qmake gateway-monitor.pro
make
```

如果目标板没有图形显示环境，可以先只运行 C 后端和本地 Socket，Qt 在开发主机或带显示的 Rootfs 中运行。
