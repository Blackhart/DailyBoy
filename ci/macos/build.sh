#!/usr/bin/env bash
# Configure and build a CMake preset on native macOS (no Docker).
#
# Default tree: build/macos/CY<year>/<config>
#
# Usage: ci/macos/build.sh <2024|2025|2026> <debug|release|sanitize>
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$root"

year="${1:?usage: ci/macos/build.sh <2024|2025|2026> <debug|release|sanitize>}"
config="${2:?usage: ci/macos/build.sh <2024|2025|2026> <debug|release|sanitize>}"
case "${year}" in
  2024|2025) python_version="3.11" ;;
  2026) python_version="3.13" ;;
  *)
    echo "error: macOS CI supports year 2024, 2025, or 2026 (got '${year}')" >&2
    exit 1
    ;;
esac
case "${config}" in
  debug|release|sanitize) ;;
  *) echo "error: config must be debug|release|sanitize" >&2; exit 1 ;;
esac

export DAILYBOY_BUILD_ROOT="${DAILYBOY_BUILD_ROOT:-macos/}"
export DAILYBOY_VFX_PLATFORM="${year}"
preset="cy${year}-${config}"
binary_dir="build/${DAILYBOY_BUILD_ROOT}CY${year}/${config}"

if [[ -e "${binary_dir}" && ! -w "${binary_dir}" ]]; then
  echo "error: ${binary_dir} is not writable by uid $(id -u)" >&2
  exit 1
fi

export CMAKE_C_COMPILER_LAUNCHER="${CMAKE_C_COMPILER_LAUNCHER:-ccache}"
export CMAKE_CXX_COMPILER_LAUNCHER="${CMAKE_CXX_COMPILER_LAUNCHER:-ccache}"
# Homebrew ships CMake 4+; nested ExternalProject configs (e.g. OIIO deps) inherit this.
export CMAKE_POLICY_VERSION_MINIMUM="${CMAKE_POLICY_VERSION_MINIMUM:-3.5}"

jobs="$(sysctl -n hw.ncpu 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"

# Homebrew prefixes (Apple Silicon vs Intel).
if [[ -d /opt/homebrew ]]; then
  brew_prefix=/opt/homebrew
elif [[ -d /usr/local/opt ]]; then
  brew_prefix=/usr/local
else
  brew_prefix=""
fi
if [[ -n "${brew_prefix}" ]]; then
  export CMAKE_PREFIX_PATH="${brew_prefix}${CMAKE_PREFIX_PATH:+:${CMAKE_PREFIX_PATH}}"
  if [[ -d "${brew_prefix}/opt/openssl@3" ]]; then
    export PKG_CONFIG_PATH="${brew_prefix}/opt/openssl@3/lib/pkgconfig${PKG_CONFIG_PATH:+:${PKG_CONFIG_PATH}}"
    export CMAKE_PREFIX_PATH="${brew_prefix}/opt/openssl@3:${CMAKE_PREFIX_PATH}"
  fi
fi

extra=()
for py in \
  "${brew_prefix:+${brew_prefix}/bin/python${python_version}}" \
  "/usr/bin/python${python_version}"; do
  [[ -z "${py}" ]] && continue
  if [[ -x "${py}" ]]; then
    extra+=(-DPython3_EXECUTABLE="${py}")
    break
  fi
done

cmake --preset "${preset}" "${extra[@]}"
cmake --build --preset "${preset}" -j"${jobs}"
