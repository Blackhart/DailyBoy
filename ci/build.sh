#!/usr/bin/env bash
# Configure and build a CMake preset for a VFX platform year.
#
# CI default tree: build/docker/CY<year>/<config>
# Host (unset DAILYBOY_BUILD_ROOT): build/CY<year>/<config>
#
# Usage: ci/build.sh <2024|2025|2026> <debug|release|sanitize>
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

year="${1:?usage: ci/build.sh <2024|2025|2026> <debug|release|sanitize>}"
config="${2:?usage: ci/build.sh <2024|2025|2026> <debug|release|sanitize>}"
case "${year}" in
  2024|2025|2026) ;;
  *) echo "error: year must be 2024, 2025, or 2026" >&2; exit 1 ;;
esac
case "${config}" in
  debug|release|sanitize) ;;
  *) echo "error: config must be debug|release|sanitize" >&2; exit 1 ;;
esac

export DAILYBOY_BUILD_ROOT="${DAILYBOY_BUILD_ROOT:-docker/}"
export DAILYBOY_VFX_PLATFORM="${year}"
preset="cy${year}-${config}"
binary_dir="build/${DAILYBOY_BUILD_ROOT}CY${year}/${config}"

if [[ -e "${binary_dir}" && ! -w "${binary_dir}" ]]; then
  echo "error: ${binary_dir} is not writable by uid $(id -u)" >&2
  echo "  sudo chown -R \"\$(id -u):\$(id -g)\" build" >&2
  exit 1
fi

export CMAKE_C_COMPILER_LAUNCHER="${CMAKE_C_COMPILER_LAUNCHER:-ccache}"
export CMAKE_CXX_COMPILER_LAUNCHER="${CMAKE_CXX_COMPILER_LAUNCHER:-ccache}"

extra=()
py=""
case "${year}" in
  2026) py=/usr/bin/python3.13 ;;
  2024|2025) py=/usr/bin/python3.11 ;;
esac
if [[ -n "${py}" && -x "${py}" ]]; then
  extra+=(-DPython3_EXECUTABLE="${py}")
fi

cmake --preset "${preset}" "${extra[@]}"
cmake --build --preset "${preset}" -j"$(nproc)"
