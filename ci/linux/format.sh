#!/usr/bin/env bash
# clang-format on DailyBoy C++ sources (no CMake / build tree).
#
# Usage:
#   ci/linux/format.sh          # dry-run --Werror (CI)
#   ci/linux/format.sh --fix   # rewrite in place
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$root"

fix=0
case "${1:-}" in
  "") ;;
  --fix) fix=1 ;;
  *)
    echo "usage: ci/linux/format.sh [--fix]" >&2
    exit 2
    ;;
esac

mapfile -t sources < <(
  find dailyboy api/cpp api/python \
    \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.cc' \) \
    -type f | sort
)

if ((${#sources[@]} == 0)); then
  echo "no C++ sources found" >&2
  exit 1
fi

if ((fix)); then
  clang-format -i --style=file "${sources[@]}"
  echo "Formatted ${#sources[@]} files"
else
  clang-format --dry-run --Werror --style=file "${sources[@]}"
fi
