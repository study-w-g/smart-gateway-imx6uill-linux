$ErrorActionPreference = 'Stop'

$required = @(
    'README.md',
    '.gitignore',
    'docs/hardware.md',
    'docs/architecture.md',
    'docs/driver-design.md',
    'docs/build-and-deploy.md',
    'docs/roadmap.md',
    'docs/status.md',
    'configs/mqtt.conf.example',
    'device-tree/README.md',
    'tests/README.md',
    'bsp/README.md',
    'kernel/README.md',
    'uboot/README.md',
    'rootfs/README.md',
    'docs/bsp-bringup.md',
    'configs/board.env.example',
    'docs/bsp-sources.md'
    'drivers/README.md',
    'drivers/ds18b20/ds18b20.c',
    'drivers/ds18b20/Makefile',
    'drivers/sht30/sht30.c',
    'drivers/sht30/Makefile',
    'drivers/gpio_event/gpio_event.c',
    'drivers/gpio_event/gpio_event_uapi.h',
    'app/README.md',
    'app/gateway_manager/gateway_manager.c',
    'app/gateway_manager/mqtt_client.c',
    'app/gateway_manager/Makefile',
    'app/tests/test_ds18b20.c',
    'app/tests/test_sht30.c',
    'app/tests/test_gpio.c',
    'app/servo_control/servo_control.c',
    'qt/main.cpp',
    'qt/mainwindow.cpp',
    'qt/gateway-monitor.pro',
    'docs/learning-flow.md',
    'device-tree/templates/smart-gateway-example.dtsi'
)

$missing = @($required | Where-Object { -not (Test-Path $_) })
if ($missing.Count -gt 0) {
    Write-Error ([string]::Concat(
        [char]0x7F3A, [char]0x5C11, [char]0x5FC5, [char]0x9700,
        [char]0x6587, [char]0x4EF6, [char]0xFF1A,
        ($missing -join [char]0x3001)))
}

Write-Host ([string]::Concat(
    [char]0x9879, [char]0x76EE, [char]0x76EE, [char]0x5F55,
    [char]0x68C0, [char]0x67E5, [char]0x901A, [char]0x8FC7, [char]0x3002)) -ForegroundColor Green
Write-Host ([string]::Concat(
    [char]0x5FC5, [char]0x9700, [char]0x6587, [char]0x6863,
    [char]0x6570, [char]0x91CF, [char]0xFF1A, $required.Count))
