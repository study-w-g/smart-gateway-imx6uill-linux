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
)

$missing = @($required | Where-Object { -not (Test-Path $_) })
if ($missing.Count -gt 0) {
    Write-Error ('Missing required files: ' + ($missing -join ', '))
}

Write-Host 'Project skeleton check passed.' -ForegroundColor Green
Write-Host ('Required documents: ' + $required.Count)
