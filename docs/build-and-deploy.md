# Build and Deploy

## Host preparation

Record these values before adding build scripts:

```text
BOARD=
KERNEL_DIR=
UBOOT_DIR=
CROSS_COMPILE=
TFTP_DIR=
NFS_ROOT=
SERIAL_DEVICE=
```

Do not commit machine-specific absolute paths.

## Planned build flow

```bash
# Build U-Boot for the actual board
make <board>_defconfig
make CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc)

# Build Linux kernel and device tree
make <board>_defconfig
make CROSS_COMPILE=${CROSS_COMPILE} zImage dtbs -j$(nproc)

# Build the root filesystem with BusyBox or Buildroot.
# Copy zImage, dtb and rootfs to the development server.
# Boot through TFTP and mount rootfs through NFS.
```

Exact board defconfig and deployment commands must be filled in after the board model is confirmed.

## Target-side checks

```bash
uname -a
cat /proc/device-tree/model
dmesg | tail -n 100
ip addr
ls /dev
```

## First boot evidence

Save sanitized U-Boot/kernel logs, driver probe logs, network results, and sensor test output under `docs/evidence/`.
