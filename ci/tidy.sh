#!/usr/bin/env bash
# Build CY2026 debug (compile_commands + deps), then clang-tidy-check.
#
# Usage: ci/tidy.sh
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

export DAILYBOY_BUILD_ROOT="${DAILYBOY_BUILD_ROOT:-docker/}"
export DAILYBOY_VFX_PLATFORM=2026

./ci/build.sh 2026 debug
cmake --build --preset tidy-check
