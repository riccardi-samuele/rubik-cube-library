#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "${tmp_dir}"' EXIT

build_dir="${tmp_dir}/build"
mkdir -p "${build_dir}"

cat > "${build_dir}/rubik-bench" <<'SCRIPT'
#!/usr/bin/env bash
sleep 5
SCRIPT
chmod +x "${build_dir}/rubik-bench"

output_file="${tmp_dir}/run.out"
set +e
RUBIK_BENCH_COMMAND_TIMEOUT_MS=100 \
    "${repo_root}/scripts/run_benchmark_suite.sh" \
    --suite fast-100 \
    --build-dir "${build_dir}" \
    --cache-dir "${tmp_dir}/cache" \
    --cache-mode warm \
    --output-dir "${tmp_dir}/out" \
    --fast-timeout-ms 5000 \
    --threads 1 \
    > "${output_file}" 2>&1
status="$?"
set -e

if [[ "${status}" -eq 0 ]]; then
    cat "${output_file}" >&2
    echo "expected hard timeout failure" >&2
    exit 1
fi

if ! grep -q "command hard timeout" "${output_file}"; then
    cat "${output_file}" >&2
    echo "expected hard timeout message" >&2
    exit 1
fi
