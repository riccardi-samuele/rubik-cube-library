#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

"${repo_root}/tests/expect_missing_value_error.sh" \
    "${repo_root}/scripts/run_auto_tail_ordering_ab.sh" \
    "Usage: scripts/run_auto_tail_ordering_ab.sh" \
    --build-dir \
    --cache-dir \
    --output-dir \
    --seeds \
    --timeout-ms \
    --max-depth \
    --threads \
    --max-memory-mb \
    --variants
