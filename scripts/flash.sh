#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
image="${IMAGE:-${root_dir}/output/images/sdcard.img}"
device="${1:-}"

[[ -n "${device}" ]] || { echo "Usage: sudo $0 /dev/sdX" >&2; exit 2; }
[[ -b "${device}" ]] || { echo "Not a block device: ${device}" >&2; exit 1; }
[[ -f "${image}" ]] || { echo "Image not found: ${image}" >&2; exit 1; }

echo "WARNING: all data on ${device} will be overwritten with ${image}."
lsblk "${device}" || true
read -r -p "Type the full device path to confirm: " confirmation
[[ "${confirmation}" == "${device}" ]] || { echo "Cancelled."; exit 0; }
dd if="${image}" of="${device}" bs=4M status=progress conv=fsync
sync
echo "Flash completed."
