#!/usr/bin/env bash
# Host entry point for a CI image.
#
# Creates `.ccache` on the host *before* `docker run`. If the path is missing,
# Docker would create the bind mount as root and break `--user`.
#
# Usage:
#   DAILYBOY_CI_IMAGE=dailyboy-ci:cy2026 ./ci/docker.sh ./ci/build.sh 2026 debug
#   ./ci/docker.sh -e ASAN_OPTIONS=detect_leaks=0 -- ./ci/test.sh 2026 sanitize
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

image="${DAILYBOY_CI_IMAGE:-dailyboy-ci:cy2026}"
usage="usage: ci/docker.sh [docker-run-args...] [--] <command> [args...]"

[[ $# -gt 0 ]] || { echo "$usage" >&2; exit 1; }

mkdir -p .ccache

extra=()
while [[ $# -gt 0 && "$1" == -* && "$1" != -- ]]; do
  extra+=("$1")
  shift
done
[[ "${1:-}" == -- ]] && shift
[[ $# -gt 0 ]] || { echo "$usage" >&2; exit 1; }

exec docker run \
  --rm \
  --user "$(id -u):$(id -g)" \
  -v "${root}:/src" \
  -w /src \
  -v "${root}/.ccache:/ccache" \
  -e CCACHE_DIR=/ccache \
  -e HOME=/tmp \
  -e DAILYBOY_BUILD_ROOT="${DAILYBOY_BUILD_ROOT:-docker/}" \
  -e CMAKE_C_COMPILER_LAUNCHER="${CMAKE_C_COMPILER_LAUNCHER:-ccache}" \
  -e CMAKE_CXX_COMPILER_LAUNCHER="${CMAKE_CXX_COMPILER_LAUNCHER:-ccache}" \
  "${extra[@]}" \
  "${image}" \
  "$@"
