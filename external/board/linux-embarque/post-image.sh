#!/usr/bin/env bash
set -euo pipefail

board_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
tmp_dir="${BUILD_DIR}/genimage.tmp"
firmware_config="${BINARIES_DIR}/rpi-firmware/config.txt"

if [[ -f "${firmware_config}" ]]; then
  grep -q '^enable_uart=1' "${firmware_config}" || printf '\n# Linux-embarque serial console\nenable_uart=1\n' >> "${firmware_config}"
fi

rm -rf "${tmp_dir}"
genimage \
  --rootpath "${TARGET_DIR}" \
  --tmppath "${tmp_dir}" \
  --inputpath "${BINARIES_DIR}" \
  --outputpath "${BINARIES_DIR}" \
  --config "${board_dir}/genimage.cfg"
