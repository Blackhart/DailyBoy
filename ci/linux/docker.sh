#!/usr/bin/env bash
# Host entry point for a CI image.
#
# Creates `.ccache` on the host *before* `docker run`. If the path is missing,
# Docker would create the bind mount as root and break `--user`.
#
# Usage:
#   DAILYBOY_CI_IMAGE=dailyboy-ci:ubuntu24-cy2026 ./ci/linux/docker.sh ./ci/linux/build.sh 2026 debug
#   ./ci/linux/docker.sh -e ASAN_OPTIONS=detect_leaks=0 -- ./ci/linux/test.sh 2026 sanitize
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$root"

image="${DAILYBOY_CI_IMAGE:-dailyboy-ci:ubuntu24-cy2026}"
usage="usage: ci/linux/docker.sh [docker-run-args...] [--] <command> [args...]"

[[ $# -gt 0 ]] || { echo "$usage" >&2; exit 1; }

mkdir -p .ccache

# Collect docker-run flags. Options that take a separate value (-e VAR=val)
# must consume the next argv; otherwise Docker treats that value as the image.
extra=()
while [[ $# -gt 0 && "$1" == -* && "$1" != -- ]]; do
  flag=$1
  extra+=("${flag}")
  shift
  case "${flag}" in
    -e | --env | -v | --volume | -w | --workdir | -u | --user | -p | --publish | \
    --name | --network | --entrypoint)
      if [[ "${flag}" != *=* ]]; then
        [[ $# -gt 0 ]] || {
          echo "error: ${flag} requires a value" >&2
          exit 1
        }
        extra+=("$1")
        shift
      fi
      ;;
  esac
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
  -e DAILYBOY_BUILD_ROOT="${DAILYBOY_BUILD_ROOT:-docker/ubuntu24/}" \
  -e CMAKE_C_COMPILER_LAUNCHER="${CMAKE_C_COMPILER_LAUNCHER:-ccache}" \
  -e CMAKE_CXX_COMPILER_LAUNCHER="${CMAKE_CXX_COMPILER_LAUNCHER:-ccache}" \
  "${extra[@]}" \
  "${image}" \
  "$@"
