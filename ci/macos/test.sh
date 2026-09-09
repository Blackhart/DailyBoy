#!/usr/bin/env bash
# Run unit / sanitize / perf / load tests on native macOS (no Docker).
#
# Usage:
#   ci/macos/test.sh 2026 tests
#   ci/macos/test.sh 2026 sanitize
#   ci/macos/test.sh 2026 perf|load
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$root"

year="${1:?usage: ci/macos/test.sh <2026> <tests|sanitize|perf|load>}"
kind="${2:?usage: ci/macos/test.sh <2026> <tests|sanitize|perf|load>}"
case "${year}" in
  2026) ;;
  *) echo "error: macOS CI currently supports year 2026 only" >&2; exit 1 ;;
esac

export DAILYBOY_BUILD_ROOT="${DAILYBOY_BUILD_ROOT:-macos/}"
export DAILYBOY_VFX_PLATFORM="${year}"

case "${kind}" in
  tests)
    binary_dir="build/${DAILYBOY_BUILD_ROOT}CY${year}/debug"
    ctest_preset="cy${year}-tests"
    ;;
  sanitize)
    binary_dir="build/${DAILYBOY_BUILD_ROOT}CY${year}/sanitize"
    ctest_preset="cy${year}-sanitize"
    ;;
  perf|load)
    binary_dir="build/${DAILYBOY_BUILD_ROOT}CY2026/release"
    ctest_preset="${kind}"
    ;;
  *)
    echo "unknown test kind: ${kind}" >&2
    exit 1
    ;;
esac

deps_libs="$(find "${binary_dir}/_deps" -type d -name lib 2>/dev/null | paste -sd: -)"
lib_path="${deps_libs}:${binary_dir}/dailyboy:${binary_dir}/api/cpp"
export DYLD_LIBRARY_PATH="${lib_path}${DYLD_LIBRARY_PATH:+:${DYLD_LIBRARY_PATH}}"

ctest --preset "${ctest_preset}"
