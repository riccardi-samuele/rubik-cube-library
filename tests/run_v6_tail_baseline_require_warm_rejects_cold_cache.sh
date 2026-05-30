#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/run_v6_tail_baseline.sh}"
repo_root="$(cd "$(dirname "${script}")/.." && pwd)"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "${tmp_dir}"' EXIT

output_file="${tmp_dir}/run.out"
set +e
"${script}" \
    --build-dir "${repo_root}/out/release-native-lto" \
    --cache-dir "${tmp_dir}/cache" \
    --output-dir "${tmp_dir}/out" \
    --cache-mode require-warm \
    --tail-seeds 99 \
    --hardening-seeds 42 \
    --deep-opt14-count 1 \
    --deep-opt15-count 1 \
    --timeout-ms 1000 \
    > "${output_file}" 2>&1
status="$?"
set -e

if [[ "${status}" -eq 0 ]]; then
    cat "${output_file}" >&2
    echo "expected cold cache rejection" >&2
    exit 1
fi

if ! grep -q "cache is not warm" "${output_file}"; then
    cat "${output_file}" >&2
    echo "expected cache warm rejection message" >&2
    exit 1
fi

manifest_file="${tmp_dir}/out/manifest.csv"
if [[ ! -f "${manifest_file}" ]]; then
    find "${tmp_dir}/out" -maxdepth 2 -type f >&2 || true
    echo "expected manifest before cold cache rejection" >&2
    exit 1
fi

grep -q "cache_setup_output,${tmp_dir}/out/cache_setup.csv" "${manifest_file}"
grep -Eq "cache_setup_elapsed_ms,[0-9]+" "${manifest_file}"

if [[ -d "${tmp_dir}/out/optimal-auto-tail" || -d "${tmp_dir}/out/optimal-auto-hardening" ]]; then
    find "${tmp_dir}/out" -maxdepth 2 -type d >&2
    echo "benchmark directories should not be created after cold cache rejection" >&2
    exit 1
fi
