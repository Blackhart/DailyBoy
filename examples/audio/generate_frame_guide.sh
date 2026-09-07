#!/usr/bin/env bash
# Build a 48 kHz stereo WAV that speaks each plate frame number (espeak).
set -euo pipefail

readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly OUT_WAV="${1:-${SCRIPT_DIR}/sh010_bg_guide.wav}"
readonly FRAME_START="${FRAME_START:-1001}"
readonly FRAME_END="${FRAME_END:-1048}"
readonly FPS="${FPS:-24}"

require_espeak() {
  command -v espeak >/dev/null || {
    echo "espeak required" >&2
    exit 1
  }
}

run_python_builder() {
  python3 "${SCRIPT_DIR}/generate_frame_guide.py" \
    "$OUT_WAV" "$FRAME_START" "$FRAME_END" "$FPS"
}

main() {
  require_espeak
  run_python_builder
}

main "$@"
