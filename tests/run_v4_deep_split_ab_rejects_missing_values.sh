#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/run_v4_deep_split_ab.sh}"

"${script}" --cases-file > /tmp/run_v4_deep_split_ab_missing_value.out 2>&1 && {
    cat /tmp/run_v4_deep_split_ab_missing_value.out >&2
    exit 1
}

grep -q "Usage: scripts/run_v4_deep_split_ab.sh" /tmp/run_v4_deep_split_ab_missing_value.out
