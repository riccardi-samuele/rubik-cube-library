#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/analyze_root_search_profile.py}"
fixture_dir="${2:-tests/fixtures/benchmark-results}"

output="$("${script}" \
    --input-dir "${fixture_dir}" \
    --reason lb9_mid_strong_min \
    --sort-by root_elapsed_ms \
    --sort-desc \
    --limit 2)"

first_root="$(printf '%s\n' "${output}" | awk -F, 'NR == 2 { print $1 "," $29 "," $11 "," $12 "," $15 }')"
second_root="$(printf '%s\n' "${output}" | awk -F, 'NR == 3 { print $1 "," $29 "," $11 "," $12 "," $15 }')"

if [[ "${first_root}" != "root_search_sample.csv,lb9_mid_strong_min,3,D,701" ]]; then
    printf 'unexpected first sorted root: %s\n' "${first_root}" >&2
    exit 1
fi

if [[ "${second_root}" != "root_search_sample.csv,lb9_mid_strong_min,1,U,612" ]]; then
    printf 'unexpected second sorted root: %s\n' "${second_root}" >&2
    exit 1
fi
