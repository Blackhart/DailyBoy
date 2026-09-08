#!/usr/bin/env bash
# Emit LCOV / Cobertura via coverage-report (CY2026 debug by default).
#
# Usage: ci/coverage_upload.sh [2025|2026]
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

year="${1:-2026}"
case "${year}" in
  2025|2026) ;;
  *) echo "error: year must be 2025 or 2026" >&2; exit 1 ;;
esac

export DAILYBOY_BUILD_ROOT="${DAILYBOY_BUILD_ROOT:-docker/ubuntu24/}"
export DAILYBOY_VFX_PLATFORM="${year}"
binary_dir="build/${DAILYBOY_BUILD_ROOT}CY${year}/debug"

if [[ ! -d "${binary_dir}" ]]; then
  echo "error: ${binary_dir} missing — run ci/build.sh ${year} debug and tests first" >&2
  exit 1
fi

if ! command -v gcovr >/dev/null 2>&1; then
  echo "error: gcovr not found (install in the CI image or on the host)" >&2
  exit 1
fi

if [[ "${year}" == "2025" ]]; then
  cmake --build --preset cy2025-coverage-report
else
  cmake --build --preset coverage-report
fi
ls -la "${binary_dir}/coverage/"
