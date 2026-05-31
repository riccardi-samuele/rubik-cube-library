#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/summarize_v6_targeted_corpus.py}"
fixture_dir="${2:-tests/fixtures/benchmark-results/v6-targeted}"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "${tmp_dir}"' EXIT

"${script}" \
    --targeted-cases "${fixture_dir}/targeted_cases.csv" \
    --comparison "${fixture_dir}/comparison.csv" \
    --case-output "${tmp_dir}/case_summary.csv" \
    --profile-output "${tmp_dir}/profile_summary.csv"

grep -q "8:11:1,8,11,1,hardening:depth15:seed42:random_42_1,2000,1800,-200,-10.00,10000,9500,-500,candidate" "${tmp_dir}/case_summary.csv"
grep -q "8:7:1,8,7,1,1,0,1,1000,1100,100,10.00,5000,4900,-100,baseline" "${tmp_dir}/profile_summary.csv"
grep -q "9:14:0,9,14,0,1,1,0,1500,1450,-50,-3.33,8000,8100,100,candidate" "${tmp_dir}/profile_summary.csv"
