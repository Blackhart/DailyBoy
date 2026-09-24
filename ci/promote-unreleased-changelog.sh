#!/usr/bin/env bash
# Promote ## [Unreleased] to ## [VERSION] - DATE and insert an empty Unreleased section.
# Usage: ci/promote-unreleased-changelog.sh <semver>
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <semver>" >&2
  exit 1
fi

VERSION="$1"
if [[ ! "${VERSION}" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  echo "error: invalid SemVer '${VERSION}' (expected X.Y.Z)" >&2
  exit 1
fi

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CHANGELOG="${ROOT}/CHANGELOG.md"
DATE="$(date -u +%Y-%m-%d)"

if [[ ! -f "${CHANGELOG}" ]]; then
  echo "error: ${CHANGELOG} not found" >&2
  exit 1
fi

python3 - "${CHANGELOG}" "${VERSION}" "${DATE}" <<'PY'
import re
import sys
from pathlib import Path

changelog_path = Path(sys.argv[1])
version = sys.argv[2]
date = sys.argv[3]

text = changelog_path.read_text(encoding="utf-8")
marker = "## [Unreleased]"
start = text.find(marker)
if start < 0:
    raise SystemExit("error: ## [Unreleased] section not found")

after_heading = start + len(marker)
if after_heading < len(text) and text[after_heading] == "\n":
    after_heading += 1

next_section = text.find("\n## [", after_heading)
if next_section < 0:
    body = text[after_heading:].strip("\n")
    rest = ""
else:
    body = text[after_heading:next_section].strip("\n")
    rest = text[next_section + 1 :]

if not body.strip():
    raise SystemExit("error: ## [Unreleased] is empty; nothing to release")

# Reject if body is only whitespace / empty headings leftovers
if not re.search(r"(?m)^### ", body) and not re.search(r"(?m)^- ", body):
    raise SystemExit("error: ## [Unreleased] has no changelog entries")

prefix = text[:start]
promoted = f"## [{version}] - {date}\n\n{body}\n\n"
new_unreleased = "## [Unreleased]\n\n"
changelog_path.write_text(prefix + new_unreleased + promoted + rest, encoding="utf-8")
print(f"Promoted Unreleased to [{version}] - {date}")
PY
