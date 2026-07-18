#!/usr/bin/env bash
set -euo pipefail

target_dir="${1:?Buildroot target directory is required}"

if [[ -f "${target_dir}/etc/inittab" ]] && ! grep -q '^tty1::' "${target_dir}/etc/inittab"; then
  sed -i '/GENERIC_SERIAL/a tty1::respawn:/sbin/getty -L tty1 0 vt100' "${target_dir}/etc/inittab"
fi
