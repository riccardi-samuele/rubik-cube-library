#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/analyze_v6_conservative_root_features.py}"
fixture_dir="${2:-tests/fixtures/benchmark-results/v6-feature-mining}"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "${tmp_dir}"' EXIT

"${script}" \
    --run-dir "${fixture_dir}" \
    --case-output "${tmp_dir}/case_features.csv" \
    --feature-output "${tmp_dir}/feature_summary.csv"

grep -q "hardening:depth15:seed42:random_42_1,8:7:1,lb8_s5-8_fd1,13,10+,0,0,-100,-10.00,-500,candidate" "${tmp_dir}/case_features.csv"
grep -q "hardening:depth15:seed42:random_42_2,8:11:1,lb8_s9-12_fd1,1,1-3,1,1,100,5.00,300,baseline" "${tmp_dir}/case_features.csv"
grep -q "bucket,lb8_s5-8_fd1,1,1,0,1000,900,-100,-10.00,5000,4500,-500,candidate" "${tmp_dir}/feature_summary.csv"
grep -q "solution_rank_bucket,1-3,1,0,1,2000,2100,100,5.00,10000,10300,300,baseline" "${tmp_dir}/feature_summary.csv"
grep -q "solution_matches_strong_first,0,1,1,0,1000,900,-100,-10.00,5000,4500,-500,candidate" "${tmp_dir}/feature_summary.csv"
