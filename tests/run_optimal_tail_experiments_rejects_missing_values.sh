#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

"${repo_root}/tests/expect_missing_value_error.sh" \
    "${repo_root}/scripts/run_optimal_tail_experiments.sh" \
    "Usage: scripts/run_optimal_tail_experiments.sh" \
    --build-dir \
    --cache-dir \
    --output-dir \
    --profile \
    --timeout-ms \
    --max-depth \
    --threads \
    --max-memory-mb \
    --variants
