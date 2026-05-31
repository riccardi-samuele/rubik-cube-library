#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/run_v6_conservative_root_probe.sh}"
repo_root="$(cd "$(dirname "${script}")/.." && pwd)"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "${tmp_dir}"' EXIT

cat > "${tmp_dir}/bad-corpus.csv" <<'CSV'
suite,seed,start_index,depth,count,expected_reason
bad_suite,42,1,15,1,conservative_root
CSV

set +e
"${script}" \
    --build-dir "${repo_root}/out/release-native-lto" \
    --cache-dir "${tmp_dir}/cache" \
    --output-dir "${tmp_dir}/out" \
    --corpus-file "${tmp_dir}/bad-corpus.csv" \
    --cache-mode require-warm \
    > "${tmp_dir}/run.out" 2>&1
status="$?"
set -e

if [[ "${status}" -eq 0 ]]; then
    cat "${tmp_dir}/run.out" >&2
    echo "expected invalid corpus rejection" >&2
    exit 1
fi

grep -q "unsupported corpus row" "${tmp_dir}/run.out"

if [[ -d "${tmp_dir}/out/default" || -d "${tmp_dir}/out/candidate" ]]; then
    find "${tmp_dir}/out" -maxdepth 2 -type d >&2
    echo "benchmark directories should not be created after corpus rejection" >&2
    exit 1
fi
