#!/usr/bin/env bash
# Run unit / sanitize / perf / load tests for a VFX platform year.
#
# Usage:
#   ci/linux/test.sh <2024|2025|2026> tests
#   ci/linux/test.sh <2024|2025|2026> sanitize
#   ci/linux/test.sh 2026 perf|load
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$root"

year="${1:?usage: ci/linux/test.sh <2024|2025|2026> <tests|sanitize|perf|load>}"
kind="${2:?usage: ci/linux/test.sh <2024|2025|2026> <tests|sanitize|perf|load>}"
case "${year}" in
  2024|2025|2026) ;;
  *) echo "error: year must be 2024, 2025, or 2026" >&2; exit 1 ;;
esac

export DAILYBOY_BUILD_ROOT="${DAILYBOY_BUILD_ROOT:-docker/ubuntu24/}"
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
    if [[ "${year}" != "2026" ]]; then
      echo "error: perf/load presets are CY2026-only" >&2
      exit 1
    fi
    binary_dir="build/${DAILYBOY_BUILD_ROOT}CY2026/release"
    ctest_preset="${kind}"
    ;;
  *)
    echo "unknown test kind: ${kind}" >&2
    exit 1
    ;;
esac

deps_libs="$(find "${binary_dir}/_deps" -type d -name lib 2>/dev/null | paste -sd:)"
export LD_LIBRARY_PATH="${deps_libs}:${binary_dir}/dailyboy:${binary_dir}/api/cpp${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"

ctest --preset "${ctest_preset}"
