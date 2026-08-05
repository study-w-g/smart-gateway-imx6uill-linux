#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ENV_FILE="${1:-${ROOT_DIR}/configs/board.env}"

if [[ ! -f "${ENV_FILE}" ]]; then
    echo "找不到 ${ENV_FILE}；请先复制 configs/board.env.example。" >&2
    exit 1
fi

# shellcheck disable=SC1090
source "${ENV_FILE}"

: "${UBOOT_DIR:?请在 board.env 中设置 UBOOT_DIR}"
: "${KERNEL_DIR:?请在 board.env 中设置 KERNEL_DIR}"
: "${CROSS_COMPILE:?请在 board.env 中设置 CROSS_COMPILE}"

mkdir -p "${OUT_DIR}"

echo "开发板：${BOARD_NAME}"
echo "U-Boot 源码：${UBOOT_DIR}"
echo "内核源码：${KERNEL_DIR}"
echo
echo "本脚本在选择默认配置和设备树之前主动停止。"
echo "请先确认 I.MX6U ALPHA V2.2 对应的厂家 BSP 文件。"
echo "本仓库不会添加 DS18B20 设备树节点。"

# 确认厂家 BSP 后，在这里补充开发板专用命令：
# make -C "${UBOOT_DIR}" <vendor_defconfig> CROSS_COMPILE="${CROSS_COMPILE}"
# make -C "${UBOOT_DIR}" CROSS_COMPILE="${CROSS_COMPILE}" -j"$(nproc)"
# make -C "${KERNEL_DIR}" <vendor_defconfig> ARCH="${ARCH}" CROSS_COMPILE="${CROSS_COMPILE}"
# make -C "${KERNEL_DIR}" ARCH="${ARCH}" CROSS_COMPILE="${CROSS_COMPILE}" zImage dtbs -j"$(nproc)"
