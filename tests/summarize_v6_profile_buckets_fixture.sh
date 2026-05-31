#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/summarize_v6_profile_buckets.py}"
fixture_dir="${2:-tests/fixtures/benchmark-results/v6-targeted}"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "${tmp_dir}"' EXIT

"${script}" \
    --targeted-cases "${fixture_dir}/targeted_cases.csv" \
    --output "${tmp_dir}/bucket_summary.csv"

grep -q "lb8_s5-8_fd1,8,5-8,1,1,1000,1000.00,5000,5000.00,8:7:1" "${tmp_dir}/bucket_summary.csv"
grep -q "lb8_s9-12_fd1,8,9-12,1,1,2000,2000.00,10000,10000.00,8:11:1" "${tmp_dir}/bucket_summary.csv"
grep -q "lb9_s13-16_fd0,9,13-16,0,1,1500,1500.00,8000,8000.00,9:14:0" "${tmp_dir}/bucket_summary.csv"
