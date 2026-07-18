#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
buildroot_dir="${BUILDROOT_DIR:-${root_dir}/buildroot}"
output_dir="${OUTPUT_DIR:-${root_dir}/output}"
external_dir="${root_dir}/external"
jobs="${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)}"
log_dir="${root_dir}/logs"
log_file="${log_dir}/build-$(date -u +%Y%m%dT%H%M%SZ).log"

[[ -f "${buildroot_dir}/Makefile" ]] || { echo "Buildroot is missing. Run scripts/setup.sh first." >&2; exit 1; }
mkdir -p "${output_dir}" "${log_dir}" "${root_dir}/dl"

exec > >(tee -a "${log_file}") 2>&1
echo "Configuring Linux-embarque in ${output_dir}"
make -C "${buildroot_dir}" BR2_EXTERNAL="${external_dir}" O="${output_dir}" linux_embarque_defconfig
echo "Building with ${jobs} job(s)"
make -C "${buildroot_dir}" O="${output_dir}" BR2_DL_DIR="${root_dir}/dl" -j"${jobs}"
echo "Images: ${output_dir}/images"
echo "Log: ${log_file}"
