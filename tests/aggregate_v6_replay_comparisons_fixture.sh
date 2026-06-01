#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/aggregate_v6_replay_comparisons.py}"
fixture_dir="${2:-tests/fixtures/benchmark-results/v6-replay-aggregate}"

output_file="$(mktemp)"
expected_file="$(mktemp)"
trap 'rm -f "${output_file}" "${expected_file}"' EXIT

"${script}" \
    --comparison "${fixture_dir}/replay_a.csv" \
    --comparison "${fixture_dir}/replay_b.csv" \
    --output "${output_file}"

cat > "${expected_file}" <<'CSV'
case_key,replays,baseline_elapsed_ms,candidate_elapsed_ms,elapsed_delta_ms,elapsed_delta_percent,baseline_wins,candidate_wins,neutral_replays,min_elapsed_delta_ms,max_elapsed_delta_ms,delta_spread_ms,baseline_nodes,candidate_nodes,nodes_delta,winner,stability
hardening:depth15:seed1:random_1_1,2,1900,2050,150,7.89,2,0,0,50,100,50,9800,10400,600,baseline,stable_baseline
hardening:depth15:seed1:random_1_2,2,3800,3650,-150,-3.95,0,2,0,-100,-50,50,19600,19300,-300,candidate,stable_candidate
__summary__,2,5700,5700,0,0.00,2,2,0,-100,100,200,29400,29700,300,baseline,mixed
CSV

diff -u "${expected_file}" "${output_file}"

missing_output="$(mktemp)"
trap 'rm -f "${output_file}" "${expected_file}" "${missing_output}"' EXIT
if "${script}" --comparison > "${missing_output}" 2>&1; then
    cat "${missing_output}" >&2
    exit 1
fi
grep -q "Usage: scripts/aggregate_v6_replay_comparisons.py" "${missing_output}"
