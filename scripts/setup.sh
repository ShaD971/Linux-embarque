#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
version_file="${root_dir}/buildroot.version"
version="$(<"${version_file}")"
buildroot_dir="${root_dir}/buildroot"
archive_dir="${root_dir}/dl"
force=0

usage() {
  echo "Usage: $0 [--version VERSION] [--force]"
}

while (($#)); do
  case "$1" in
    --version) version="${2:?Missing version}"; shift 2 ;;
    --force) force=1; shift ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown argument: $1" >&2; usage; exit 2 ;;
  esac
done

for command in bash make tar xz; do
  command -v "${command}" >/dev/null 2>&1 || { echo "Missing dependency: ${command}" >&2; exit 1; }
done
if command -v curl >/dev/null 2>&1; then
  downloader=(curl --fail --location --retry 3 --output)
elif command -v wget >/dev/null 2>&1; then
  downloader=(wget --tries=3 --output-document)
else
  echo "Missing dependency: curl or wget" >&2
  exit 1
fi

if [[ -f "${buildroot_dir}/Makefile" ]]; then
  installed="$(sed -n 's/^export BR2_VERSION := //p' "${buildroot_dir}/Makefile" | head -n1)"
  echo "Buildroot is already installed (${installed:-unknown version}) in ${buildroot_dir}."
  if ((force == 0)); then
    read -r -p "Replace this installation? [y/N] " answer
    [[ "${answer}" =~ ^[Yy]$ ]] || exit 0
  fi
  rm -rf "${buildroot_dir}"
fi

mkdir -p "${archive_dir}" "${root_dir}/output" "${root_dir}/logs"
archive="${archive_dir}/buildroot-${version}.tar.xz"
url="https://buildroot.org/downloads/buildroot-${version}.tar.xz"

if [[ ! -s "${archive}" ]]; then
  echo "Downloading ${url}"
  "${downloader[@]}" "${archive}" "${url}"
fi
[[ -s "${archive}" ]] || { echo "Download is missing or empty: ${archive}" >&2; exit 1; }

temporary="${root_dir}/.buildroot-extract-${version}"
rm -rf "${temporary}"
mkdir -p "${temporary}"
if ! tar -xJf "${archive}" -C "${temporary}"; then
  rm -rf "${temporary}"
  echo "Unable to extract ${archive}" >&2
  exit 1
fi
mv "${temporary}/buildroot-${version}" "${buildroot_dir}"
rm -rf "${temporary}"

installed="$(sed -n 's/^export BR2_VERSION := //p' "${buildroot_dir}/Makefile" | head -n1)"
echo "Installed Buildroot ${installed:-$version} in ${buildroot_dir}."
echo "Next: ${root_dir}/scripts/build.sh"
echo "Submodule alternative (display only):"
echo "  git submodule add -b ${version} https://gitlab.com/buildroot.org/buildroot.git buildroot"
