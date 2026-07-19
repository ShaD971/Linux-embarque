#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
buildroot_dir="${BUILDROOT_DIR:-${root_dir}/buildroot}"
output_dir="${OUTPUT_DIR:-${root_dir}/output}"

[[ -f "${buildroot_dir}/Makefile" ]] || { echo "Buildroot is missing. Run scripts/setup.sh first." >&2; exit 1; }
echo "Cleaning build products in ${output_dir}; downloaded sources in dl/ are retained."
make -C "${buildroot_dir}" O="${output_dir}" clean
exec "${root_dir}/scripts/build.sh"
