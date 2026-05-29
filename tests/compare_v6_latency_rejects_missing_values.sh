#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/compare_v6_latency.py}"

"${script}" --baseline > /tmp/compare_v6_latency_missing_value.out 2>&1 && {
    cat /tmp/compare_v6_latency_missing_value.out >&2
    exit 1
}

grep -q "Usage: scripts/compare_v6_latency.py" /tmp/compare_v6_latency_missing_value.out
