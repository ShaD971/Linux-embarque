#!/usr/bin/env bash
set -euo pipefail

board_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
tmp_dir="${BUILD_DIR}/genimage.tmp"

required_files=(
  "Image"
  "bcm2711-rpi-4-b.dtb"
  "rootfs.ext4"
  "rpi-firmware/config.txt"
  "rpi-firmware/cmdline.txt"
  "rpi-firmware/start4.elf"
  "rpi-firmware/fixup4.dat"
)

for file in "${required_files[@]}"; do
  [[ -e "${BINARIES_DIR}/${file}" ]] || {
    echo "Missing Raspberry Pi 4 boot artifact: ${BINARIES_DIR}/${file}" >&2
    exit 1
  }
done

[[ -d "${BINARIES_DIR}/rpi-firmware/overlays" ]] || {
  echo "Missing Raspberry Pi Device Tree overlays" >&2
  exit 1
}

rm -rf "${tmp_dir}"
genimage \
  --rootpath "${TARGET_DIR}" \
  --tmppath "${tmp_dir}" \
  --inputpath "${BINARIES_DIR}" \
  --outputpath "${BINARIES_DIR}" \
  --config "${board_dir}/genimage.cfg"
