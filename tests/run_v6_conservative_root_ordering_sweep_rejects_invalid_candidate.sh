#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/run_v6_conservative_root_ordering_sweep.sh}"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "${tmp_dir}"' EXIT

set +e
"${script}" \
    --build-dir "${tmp_dir}/build" \
    --cache-dir "${tmp_dir}/cache" \
    --output-dir "${tmp_dir}/out" \
    --corpus-file benchmarks/v6_conservative_root_corpus.csv \
    --candidates reverse_tie,not_a_candidate \
    > "${tmp_dir}/run.out" 2>&1
status="$?"
set -e

if [[ "${status}" -eq 0 ]]; then
    cat "${tmp_dir}/run.out" >&2
    echo "expected invalid candidate rejection" >&2
    exit 1
fi

grep -q "unsupported root ordering candidate: not_a_candidate" "${tmp_dir}/run.out"

set +e
"${script}" \
    --build-dir "${tmp_dir}/build" \
    --cache-dir "${tmp_dir}/cache" \
    --output-dir "${tmp_dir}/positive-out" \
    --corpus-file benchmarks/v6_conservative_root_corpus.csv \
    --candidates positive_high_bound \
    --probe-script "${tmp_dir}/missing_probe.sh" \
    > "${tmp_dir}/positive.out" 2>&1
status="$?"
set -e

if [[ "${status}" -eq 0 ]]; then
    cat "${tmp_dir}/positive.out" >&2
    echo "expected positive_high_bound to pass candidate validation and fail on missing probe" >&2
    exit 1
fi
grep -q "probe script is not executable" "${tmp_dir}/positive.out"

if [[ -d "${tmp_dir}/out" ]]; then
    find "${tmp_dir}/out" -maxdepth 3 -type f >&2
    echo "output directory should not be created after candidate validation failure" >&2
    exit 1
fi
