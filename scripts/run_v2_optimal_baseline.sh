#!/usr/bin/env bash
set -euo pipefail

build_dir="out/release-native-lto"
cache_dir="${RUBIK_TABLE_CACHE_DIR:-/tmp/rubik_cube_library_v2_optimal_baseline_cache}"
output_dir="benchmark-results/v2-optimal-baseline"
seeds="12345,20260525,42"
optimal_timeout_ms="30000"
optimal_depth13_count="10"
deep_opt14_count="1"
deep_opt15_count="1"
threads="1"
max_memory_mb="1024"
slowest_limit="50"
include_deep_probe="0"

usage() {
    cat <<'USAGE'
Usage: scripts/run_v2_optimal_baseline.sh [options]

Options:
  --build-dir DIR          CMake build directory, default: out/release-native-lto
  --cache-dir DIR          pruning table cache directory
  --output-dir DIR         output directory, default: benchmark-results/v2-optimal-baseline
  --seeds LIST             comma-separated seeds, default: 12345,20260525,42
  --timeout-ms N           optimal per-case timeout, default: 30000
  --opt13-count N          random depth-13 cases per seed/profile, default: 10
  --include-deep-probe     also run depth-14/depth-15 non-gated probe
  --deep-opt14-count N     depth-14 cases per seed/profile when deep probe is enabled, default: 1
  --deep-opt15-count N     depth-15 cases per seed/profile when deep probe is enabled, default: 1
  --threads N              solver threads forwarded to deep probe, default: 1
  --max-memory-mb N        solver memory limit forwarded to deep probe, default: 1024
  --slowest-limit N        number of slowest rows to extract, default: 50
  -h, --help               show this help
USAGE
}

require_value() {
    if [[ $# -lt 2 || "$2" == -* ]]; then
        usage >&2
        exit 2
    fi
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir)
            require_value "$@"
            build_dir="$2"
            shift 2
            ;;
        --cache-dir)
            require_value "$@"
            cache_dir="$2"
            shift 2
            ;;
        --output-dir)
            require_value "$@"
            output_dir="$2"
            shift 2
            ;;
        --seeds)
            require_value "$@"
            seeds="$2"
            shift 2
            ;;
        --timeout-ms)
            require_value "$@"
            optimal_timeout_ms="$2"
            shift 2
            ;;
        --opt13-count)
            require_value "$@"
            optimal_depth13_count="$2"
            shift 2
            ;;
        --include-deep-probe)
            include_deep_probe="1"
            shift
            ;;
        --deep-opt14-count)
            require_value "$@"
            deep_opt14_count="$2"
            shift 2
            ;;
        --deep-opt15-count)
            require_value "$@"
            deep_opt15_count="$2"
            shift 2
            ;;
        --threads)
            require_value "$@"
            threads="$2"
            shift 2
            ;;
        --max-memory-mb)
            require_value "$@"
            max_memory_mb="$2"
            shift 2
            ;;
        --slowest-limit)
            require_value "$@"
            slowest_limit="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            usage >&2
            exit 2
            ;;
    esac
done

for numeric in \
    "${optimal_timeout_ms}" \
    "${optimal_depth13_count}" \
    "${deep_opt14_count}" \
    "${deep_opt15_count}" \
    "${threads}" \
    "${max_memory_mb}" \
    "${slowest_limit}"; do
    if [[ ! "${numeric}" =~ ^[0-9]+$ || "${numeric}" -lt 1 ]]; then
        usage >&2
        exit 2
    fi
done

bench="${build_dir}/rubik-bench"
if [[ ! -x "${bench}" ]]; then
    echo "rubik-bench not found or not executable: ${bench}" >&2
    echo "Build first with: cmake --build ${build_dir}" >&2
    exit 1
fi

mkdir -p "${output_dir}"
suite_status=0

manifest="${output_dir}/manifest.csv"
{
    echo "key,value"
    echo "git_revision,$(git rev-parse --short HEAD 2>/dev/null || echo unknown)"
    echo "build_dir,${build_dir}"
    echo "cache_dir,${cache_dir}"
    echo "seeds,${seeds}"
    echo "timeout_ms,${optimal_timeout_ms}"
    echo "opt13_count,${optimal_depth13_count}"
    echo "include_deep_probe,${include_deep_probe}"
    echo "deep_opt14_count,${deep_opt14_count}"
    echo "deep_opt15_count,${deep_opt15_count}"
    echo "threads,${threads}"
    echo "max_memory_mb,${max_memory_mb}"
} > "${manifest}"

run_suite() {
    set +e
    "$@"
    local status="$?"
    set -e
    if [[ "${status}" -ne 0 ]]; then
        suite_status=1
    fi
}

run_suite scripts/run_benchmark_suite.sh \
    --suite optimal-stress \
    --build-dir "${build_dir}" \
    --cache-dir "${cache_dir}" \
    --output-dir "${output_dir}/optimal-stress" \
    --seeds "${seeds}" \
    --optimal-timeout-ms "${optimal_timeout_ms}" \
    --realistic-opt13-count "${optimal_depth13_count}"

run_suite scripts/run_benchmark_suite.sh \
    --suite optimal-tail-cases \
    --build-dir "${build_dir}" \
    --cache-dir "${cache_dir}" \
    --output-dir "${output_dir}/optimal-tail-cases" \
    --optimal-timeout-ms "${optimal_timeout_ms}"

if [[ "${include_deep_probe}" == "1" ]]; then
    run_suite scripts/run_benchmark_suite.sh \
        --suite optimal-deep-probe \
        --build-dir "${build_dir}" \
        --cache-dir "${cache_dir}" \
        --output-dir "${output_dir}/optimal-deep-probe" \
        --seeds "${seeds}" \
        --optimal-timeout-ms "${optimal_timeout_ms}" \
        --deep-opt14-count "${deep_opt14_count}" \
        --deep-opt15-count "${deep_opt15_count}" \
        --threads "${threads}" \
        --max-memory-mb "${max_memory_mb}"
fi

scripts/extract_slowest_cases.sh \
    --input-dir "${output_dir}" \
    --output "${output_dir}/slowest-cases.csv" \
    --limit "${slowest_limit}"

echo "v2 optimal baseline manifest: ${manifest}"
echo "v2 optimal baseline slowest cases: ${output_dir}/slowest-cases.csv"
exit "${suite_status}"
