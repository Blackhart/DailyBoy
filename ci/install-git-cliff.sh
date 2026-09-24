#!/usr/bin/env bash
# Install a pinned git-cliff binary into /usr/local/bin (or DEST_DIR).
set -euo pipefail

GIT_CLIFF_VERSION="${GIT_CLIFF_VERSION:-2.8.0}"
DEST_DIR="${DEST_DIR:-/usr/local/bin}"
ARCH="$(uname -m)"
case "${ARCH}" in
  x86_64|amd64) ASSET="git-cliff-${GIT_CLIFF_VERSION}-x86_64-unknown-linux-gnu.tar.gz" ;;
  aarch64|arm64) ASSET="git-cliff-${GIT_CLIFF_VERSION}-aarch64-unknown-linux-gnu.tar.gz" ;;
  *)
    echo "error: unsupported architecture ${ARCH}" >&2
    exit 1
    ;;
esac

URL="https://github.com/orhun/git-cliff/releases/download/v${GIT_CLIFF_VERSION}/${ASSET}"
TMP="$(mktemp -d)"
trap 'rm -rf "${TMP}"' EXIT

curl -fsSL "${URL}" | tar -xz -C "${TMP}"
BIN="${TMP}/git-cliff-${GIT_CLIFF_VERSION}/git-cliff"
if [[ -w "${DEST_DIR}" ]] || [[ "$(id -u)" -eq 0 ]]; then
  install -m 0755 "${BIN}" "${DEST_DIR}/git-cliff"
else
  sudo install -m 0755 "${BIN}" "${DEST_DIR}/git-cliff"
fi
git-cliff --version
