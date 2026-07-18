#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
buildroot_dir="${BUILDROOT_DIR:-${root_dir}/buildroot}"
output_dir="${OUTPUT_DIR:-${root_dir}/output}"
level="${1:-light}"

case "${level}" in
  light)
    echo "Will run Buildroot 'clean' in ${output_dir}; dl/ is retained."
    [[ -f "${buildroot_dir}/Makefile" ]] && make -C "${buildroot_dir}" O="${output_dir}" clean
    ;;
  buildroot)
    echo "Will run Buildroot 'distclean' in ${output_dir}; dl/ is retained."
    read -r -p "Continue? [y/N] " answer
    [[ "${answer}" =~ ^[Yy]$ ]] || exit 0
    [[ -f "${buildroot_dir}/Makefile" ]] && make -C "${buildroot_dir}" O="${output_dir}" distclean
    ;;
  output)
    echo "Will permanently remove ${output_dir}; dl/ is retained."
    read -r -p "Type REMOVE to continue: " answer
    [[ "${answer}" == "REMOVE" ]] || exit 0
    rm -rf -- "${output_dir}"
    ;;
  *) echo "Usage: $0 {light|buildroot|output}" >&2; exit 2 ;;
esac
