#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

"${repo_root}/tests/expect_missing_value_error.sh" \
    "${repo_root}/scripts/analyze_root_search_profile.py" \
    "Usage: scripts/analyze_root_search_profile.py" \
    --input-dir \
    --output \
    --limit
