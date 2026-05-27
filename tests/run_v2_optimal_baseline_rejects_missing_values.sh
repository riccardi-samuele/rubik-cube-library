#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

"${repo_root}/tests/expect_missing_value_error.sh" \
    "${repo_root}/scripts/run_v2_optimal_baseline.sh" \
    "Usage: scripts/run_v2_optimal_baseline.sh" \
    --build-dir \
    --cache-dir \
    --output-dir \
    --seeds \
    --timeout-ms \
    --opt13-count \
    --deep-opt14-count \
    --deep-opt15-count \
    --threads \
    --max-memory-mb \
    --slowest-limit
