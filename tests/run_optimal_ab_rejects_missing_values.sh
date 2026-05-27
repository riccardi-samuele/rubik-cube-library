#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

"${repo_root}/tests/expect_missing_value_error.sh" \
    "${repo_root}/scripts/run_optimal_ab.sh" \
    "Usage: scripts/run_optimal_ab.sh" \
    --build-dir \
    --cache-dir \
    --output-dir \
    --repetitions \
    --case-set \
    --max-case-depth \
    --max-depth \
    --timeout-ms \
    --random-count \
    --random-depth \
    --random-seed
