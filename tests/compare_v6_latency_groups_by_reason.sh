#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/compare_v6_latency.py}"
baseline_dir="${2:-tests/fixtures/benchmark-results/v6-baseline}"
candidate_dir="${3:-tests/fixtures/benchmark-results/v6-candidate}"

output="$("${script}" \
    --baseline-dir "${baseline_dir}" \
    --candidate-dir "${candidate_dir}" \
    --group-by-reason \
    --sort-by elapsed_delta_ms)"

conservative_row="$(printf '%s\n' "${output}" | awk -F, '$1 == "__reason__:conservative_root" { print $1 "," $2 "," $3 "," $4 "," $5 "," $22 "," $23 "," $24 }')"

if [[ "${conservative_row}" != "__reason__:conservative_root,2,1300,1100,-200,13000,12000,-1000" ]]; then
    printf 'unexpected conservative_root group: %s\n' "${conservative_row}" >&2
    exit 1
fi

summary_row="$(printf '%s\n' "${output}" | tail -n 1 | cut -d, -f1)"

if [[ "${summary_row}" != "__summary__" ]]; then
    printf 'summary row was not last: %s\n' "${summary_row}" >&2
    exit 1
fi
