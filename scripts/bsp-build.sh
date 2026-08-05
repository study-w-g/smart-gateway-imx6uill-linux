#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ENV_FILE="${1:-${ROOT_DIR}/configs/board.env}"

if [[ ! -f "${ENV_FILE}" ]]; then
    echo "Missing ${ENV_FILE}; copy configs/board.env.example first." >&2
    exit 1
fi

# shellcheck disable=SC1090
source "${ENV_FILE}"

: "${UBOOT_DIR:?Set UBOOT_DIR in board.env}"
: "${KERNEL_DIR:?Set KERNEL_DIR in board.env}"
: "${CROSS_COMPILE:?Set CROSS_COMPILE in board.env}"

mkdir -p "${OUT_DIR}"

echo "Board: ${BOARD_NAME}"
echo "U-Boot source: ${UBOOT_DIR}"
echo "Kernel source: ${KERNEL_DIR}"
echo
echo "This script intentionally stops before selecting a defconfig or DTS."
echo "Confirm the exact vendor BSP files for I.MX6U ALPHA V2.2 first."
echo "No DS18B20 device-tree node is added by this repository."

# After the vendor BSP is confirmed, add board-specific commands here:
# make -C "${UBOOT_DIR}" <vendor_defconfig> CROSS_COMPILE="${CROSS_COMPILE}"
# make -C "${UBOOT_DIR}" CROSS_COMPILE="${CROSS_COMPILE}" -j"$(nproc)"
# make -C "${KERNEL_DIR}" <vendor_defconfig> ARCH="${ARCH}" CROSS_COMPILE="${CROSS_COMPILE}"
# make -C "${KERNEL_DIR}" ARCH="${ARCH}" CROSS_COMPILE="${CROSS_COMPILE}" zImage dtbs -j"$(nproc)"
