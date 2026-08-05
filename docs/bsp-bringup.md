# I.MX6U ALPHA V2.2 BSP Bring-up

## Why the full BSP is not copied into this repository

The board-specific U-Boot and Linux sources are normally distributed in the vendor course/material package. A generic kernel may compile but still fail to boot because DDR initialization, pinctrl, storage, Ethernet, display and board device-tree details are vendor-specific.

The repository therefore stores the reproducible project layer:

- exact source version and local source path
- defconfig names after they are confirmed
- patches written by this project
- build/deploy scripts
- boot and test logs with secrets removed

## Bring-up order

1. Obtain the vendor BSP package for I.MX6U ALPHA V2.2.
2. Identify the U-Boot directory, board defconfig, kernel version and matching DTS.
3. Set `configs/board.env` from the example.
4. Build the unmodified vendor BSP first.
5. Boot the unmodified kernel and save the serial log.
6. Confirm Ethernet and USB Wi-Fi before adding project drivers.
7. Add project-specific device-tree nodes one at a time.
8. Build the external DS18B20 module against the running kernel source.

## NFS/TFTP roles

| Component | Typical role |
|---|---|
| TFTP | Transfer U-Boot-selected kernel image and DTB during development |
| NFS | Provide a writable development Rootfs to the board |
| SD/eMMC/NAND | Store the final bootloader, kernel, DTB and Rootfs/image |

NFS does not modify the device tree automatically. The DTB loaded by U-Boot must be rebuilt and selected at boot.

## Evidence to save

- U-Boot version and environment
- Kernel version from `uname -a`
- `/proc/device-tree/model`
- first boot serial log
- `dmesg` after network bring-up
- exact commands used for each build

Do not add the DS18B20 device node until the user has confirmed the actual GPIO and pull-up circuit.
