#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/extract_v6_transition_corpus.py}"
fixture_dir="${2:-tests/fixtures/benchmark-results/v6-transition-corpus}"

output_file="$(mktemp)"
expected_file="$(mktemp)"
trap 'rm -f "${output_file}" "${expected_file}"' EXIT

"${script}" \
    --comparison "${fixture_dir}/comparison.csv" \
    --baseline-ordering high_bound_first \
    --candidate-ordering default \
    --output "${output_file}"

cat > "${expected_file}" <<'CSV'
suite,seed,start_index,depth,count,expected_reason
hardening,12345,5,15,1,conservative_root
tail,202,3,14,1,conservative_root
CSV

diff -u "${expected_file}" "${output_file}"

missing_output="$(mktemp)"
trap 'rm -f "${output_file}" "${expected_file}" "${missing_output}"' EXIT
if "${script}" --comparison > "${missing_output}" 2>&1; then
    cat "${missing_output}" >&2
    exit 1
fi
grep -q "Usage: scripts/extract_v6_transition_corpus.py" "${missing_output}"
