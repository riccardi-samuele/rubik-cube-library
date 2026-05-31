#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/analyze_v6_ordering_candidate_features.py}"
fixture_dir="${2:-tests/fixtures/benchmark-results/v6-ordering-feature-analysis}"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "${tmp_dir}"' EXIT

"${script}" \
    --comparison "${fixture_dir}/comparison.csv" \
    --features "${fixture_dir}/discovery_case_features.csv" \
    --case-output "${tmp_dir}/case_features.csv" \
    --feature-output "${tmp_dir}/feature_summary.csv"

grep -q "hardening:depth15:seed42:random_42_1,8:7:1,lb8_s5-8_fd1,13,10+,0,0,3,1-3,found,70,high,D,-100,-10.00,-500,candidate,1000,900,5000,4500" "${tmp_dir}/case_features.csv"
grep -q "hardening:depth15:seed42:random_42_2,8:11:1,lb8_s9-12_fd1,1,1-3,1,1,1,1-3,found,40,medium,U,100,5.00,300,baseline,2000,2100,10000,10300" "${tmp_dir}/case_features.csv"
grep -q "bucket,lb8_s5-8_fd1,1,1,0,0,1000,900,-100,-10.00,5000,4500,-500,candidate,hardening:depth15:seed42:random_42_1,-100,hardening:depth15:seed42:random_42_1,-100" "${tmp_dir}/feature_summary.csv"
grep -q "solution_rank_bucket,1-3,1,0,1,0,2000,2100,100,5.00,10000,10300,300,baseline,hardening:depth15:seed42:random_42_2,100,hardening:depth15:seed42:random_42_2,100" "${tmp_dir}/feature_summary.csv"
grep -q "solution_root_status,found,2,1,1,0,3000,3000,0,0.00,15000,14800,-200,tie,hardening:depth15:seed42:random_42_1,-100,hardening:depth15:seed42:random_42_2,100" "${tmp_dir}/feature_summary.csv"

"${script}" \
    --comparison "${fixture_dir}/comparison_missing_feature.csv" \
    --features "${fixture_dir}/discovery_case_features.csv" \
    --case-output "${tmp_dir}/missing_case_features.csv" \
    --feature-output "${tmp_dir}/missing_feature_summary.csv" >"${tmp_dir}/missing.out" 2>&1 && {
        cat "${tmp_dir}/missing.out" >&2
        exit 1
    }
grep -q "missing feature rows" "${tmp_dir}/missing.out"
