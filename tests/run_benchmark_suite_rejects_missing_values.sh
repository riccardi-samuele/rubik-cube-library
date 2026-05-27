#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

"${repo_root}/tests/expect_missing_value_error.sh" \
    "${repo_root}/scripts/run_benchmark_suite.sh" \
    "Usage: scripts/run_benchmark_suite.sh" \
    --suite \
    --build-dir \
    --cache-dir \
    --cache-mode \
    --output-dir \
    --profile \
    --seed \
    --seeds \
    --fast-timeout-ms \
    --fast-max-depth \
    --optimal-timeout-ms \
    --realistic-fast-count \
    --realistic-opt12-count \
    --realistic-opt13-count \
    --deep-opt14-count \
    --deep-opt15-count \
    --threads \
    --max-memory-mb
