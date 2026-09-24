#!/usr/bin/env bash
# Regenerate the body under ## [Unreleased] from commits since the last tag.
# Leaves published version sections untouched.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CHANGELOG="${ROOT}/CHANGELOG.md"
TMP_BODY="$(mktemp)"
TMP_OUT="$(mktemp)"
trap 'rm -f "${TMP_BODY}" "${TMP_OUT}"' EXIT

cd "${ROOT}"

if ! command -v git-cliff >/dev/null 2>&1; then
  echo "error: git-cliff not found in PATH" >&2
  exit 1
fi

if [[ ! -f "${CHANGELOG}" ]]; then
  echo "error: ${CHANGELOG} not found" >&2
  exit 1
fi

# Body only: drop the ## [Unreleased] heading rendered by the template.
git cliff --unreleased --strip header >"${TMP_BODY}.raw"
awk '
  BEGIN { skip_first_heading = 1 }
  skip_first_heading && /^## \[Unreleased\]$/ {
    skip_first_heading = 0
    next
  }
  { print }
' "${TMP_BODY}.raw" >"${TMP_BODY}"
rm -f "${TMP_BODY}.raw"

python3 - "${CHANGELOG}" "${TMP_BODY}" "${TMP_OUT}" <<'PY'
import sys
from pathlib import Path

changelog_path = Path(sys.argv[1])
body_path = Path(sys.argv[2])
out_path = Path(sys.argv[3])

text = changelog_path.read_text(encoding="utf-8")
new_body = body_path.read_text(encoding="utf-8").rstrip("\n")

marker = "## [Unreleased]"
start = text.find(marker)
if start < 0:
    raise SystemExit("error: ## [Unreleased] section not found in CHANGELOG.md")

after_heading = start + len(marker)
# Skip a single newline after the heading if present.
if after_heading < len(text) and text[after_heading] == "\n":
    after_heading += 1

next_section = text.find("\n## [", after_heading)
if next_section < 0:
    prefix = text[:after_heading]
    suffix = ""
else:
    # next_section points at the newline before the next ## [
    prefix = text[:after_heading]
    suffix = text[next_section + 1 :]  # keep starting at ## [

if new_body:
    middle = new_body + "\n\n"
else:
    middle = "\n"

out_path.write_text(prefix + middle + suffix, encoding="utf-8")
PY

mv "${TMP_OUT}" "${CHANGELOG}"
trap - EXIT
rm -f "${TMP_BODY}"

echo "Updated Unreleased section in ${CHANGELOG}"
