#!/usr/bin/env bash
# Run clang-tidy-check (expects CY2026 debug already built for compile_commands).
#
# Usage:
#   ./ci/linux/build.sh 2026 debug
#   ./ci/linux/tidy.sh
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$root"

export DAILYBOY_BUILD_ROOT="${DAILYBOY_BUILD_ROOT:-docker/ubuntu24/}"
export DAILYBOY_VFX_PLATFORM=2026

binary_dir="build/${DAILYBOY_BUILD_ROOT}CY2026/debug"
if [[ ! -f "${binary_dir}/compile_commands.json" ]]; then
  echo "error: ${binary_dir}/compile_commands.json missing — run ci/linux/build.sh 2026 debug first" >&2
  exit 1
fi

cmake --build --preset tidy-check
