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

# Raspberry Pi Imager accepte les archives .xz et les decompresse a la volee.
# L'image brute fait ~320 Mo, compressee ~60 Mo : c'est ce qu'on distribue.
# On garde sdcard.img a cote pour dd / balenaEtcher.
if command -v xz >/dev/null 2>&1; then
  xz --keep --force --threads=0 -6 "${BINARIES_DIR}/sdcard.img"
  echo "Image compressee : ${BINARIES_DIR}/sdcard.img.xz"
else
  echo "xz introuvable, sdcard.img.xz non genere" >&2
fi
