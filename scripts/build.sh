#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
buildroot_dir="${BUILDROOT_DIR:-${root_dir}/buildroot}"
output_dir="${OUTPUT_DIR:-${root_dir}/output}"
external_dir="${root_dir}/external"
jobs="${BUILD_JOBS:-${JOBS:-2}}"
log_dir="${root_dir}/logs"
log_file="${BUILD_LOG:-${root_dir}/build.log}"
archived_log="${log_dir}/build-$(date -u +%Y%m%dT%H%M%SZ).log"

[[ -f "${buildroot_dir}/Makefile" ]] || { echo "Buildroot is missing. Run scripts/setup.sh first." >&2; exit 1; }
mkdir -p "${output_dir}" "${log_dir}" "${root_dir}/dl"
[[ "${jobs}" =~ ^[1-9][0-9]*$ ]] || { echo "BUILD_JOBS/JOBS must be a positive integer, got: ${jobs}" >&2; exit 2; }

run_build() {
  echo "Configuring Linux-embarque in ${output_dir}"
  make -C "${buildroot_dir}" BR2_EXTERNAL="${external_dir}" O="${output_dir}" linux_embarque_defconfig || return $?
  echo "Building with ${jobs} job(s)"
  make -C "${buildroot_dir}" O="${output_dir}" BR2_DL_DIR="${root_dir}/dl" -j"${jobs}"
}

set +e
set -o pipefail
run_build 2>&1 | tee "${log_file}"
build_status=${PIPESTATUS[0]}
set -e

cp "${log_file}" "${archived_log}"

if ((build_status != 0)); then
  echo "::error::Buildroot build failed"
  echo "=== Last 300 lines ==="
  tail -n 300 "${log_file}"
  echo "=== Relevant error lines ==="
  grep -nEi \
    'fatal error|internal compiler error|killed|out of memory|no space left|cannot stat|error:|make(\[[0-9]+\])?: \*\*\*' \
    "${log_file}" | tail -n 100 || true
  echo "Full log: ${log_file}"
  exit "${build_status}"
fi

echo "Images: ${output_dir}/images"
echo "Log: ${log_file}"
echo "Archived log: ${archived_log}"
