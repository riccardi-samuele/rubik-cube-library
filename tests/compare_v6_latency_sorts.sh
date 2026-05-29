#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/compare_v6_latency.py}"
baseline_dir="${2:-tests/fixtures/benchmark-results/v6-baseline}"
candidate_dir="${3:-tests/fixtures/benchmark-results/v6-candidate}"

output="$("${script}" \
    --baseline-dir "${baseline_dir}" \
    --candidate-dir "${candidate_dir}" \
    --sort-by elapsed_delta_ms)"

first_case="$(printf '%s\n' "${output}" | awk -F, 'NR == 2 { print $1 "," $5 }')"

if [[ "${first_case}" != "hardening:depth15:seed202:random_202_2,-100" ]]; then
    printf 'unexpected first sorted case: %s\n' "${first_case}" >&2
    exit 1
fi

output_desc="$("${script}" \
    --baseline-dir "${baseline_dir}" \
    --candidate-dir "${candidate_dir}" \
    --sort-by elapsed_delta_ms \
    --sort-desc)"

first_desc_case="$(printf '%s\n' "${output_desc}" | awk -F, 'NR == 2 { print $1 "," $5 }')"

if [[ "${first_desc_case}" != "hardening:depth15:seed202:random_202_1,100" ]]; then
    printf 'unexpected first descending sorted case: %s\n' "${first_desc_case}" >&2
    exit 1
fi
