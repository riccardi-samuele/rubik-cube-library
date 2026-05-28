#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"

build_dir="out/release-native-lto"
cache_dir="${RUBIK_TABLE_CACHE_DIR:-/tmp/rubik_cube_library_v6_tail_baseline_cache}"
output_dir="benchmark-results/v6-tail-baseline"
tail_seeds="987654321,424242,1009,666,555,99,888"
hardening_seeds="12345,20260525,42,314159,271828,987654321,7,99,123456789,424242,8675309,20240525"
timeout_ms="30000"
threads="0"
max_memory_mb="2048"
deep_opt14_count="2"
deep_opt15_count="1"
cache_mode="warm"

usage() {
    cat <<'USAGE'
Usage: scripts/run_v6_tail_baseline.sh [options]

Options:
  --build-dir DIR        CMake build directory, default: out/release-native-lto
  --cache-dir DIR        pruning table cache directory
  --output-dir DIR       output directory, default: benchmark-results/v6-tail-baseline
  --tail-seeds LIST      comma-separated depth-15 tail seeds
  --hardening-seeds LIST comma-separated hardening seeds
  --timeout-ms N         per-case timeout, default: 30000
  --threads N            solver threads, default: 0
  --max-memory-mb N      solver memory limit, default: 2048
  --deep-opt14-count N   hardening depth-14 cases per seed, default: 2
  --deep-opt15-count N   depth-15 cases per seed, default: 1
  --cache-mode MODE      warm|cold|reuse, default: warm
  -h, --help             show this help
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
        --tail-seeds)
            require_value "$@"
            tail_seeds="$2"
            shift 2
            ;;
        --hardening-seeds)
            require_value "$@"
            hardening_seeds="$2"
            shift 2
            ;;
        --timeout-ms)
            require_value "$@"
            timeout_ms="$2"
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
        --cache-mode)
            require_value "$@"
            cache_mode="$2"
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

for numeric in "${timeout_ms}" "${threads}" "${max_memory_mb}" "${deep_opt14_count}" "${deep_opt15_count}"; do
    if [[ ! "${numeric}" =~ ^[0-9]+$ ]]; then
        usage >&2
        exit 2
    fi
done

if (( timeout_ms < 1 || max_memory_mb < 1 || deep_opt14_count < 1 || deep_opt15_count < 1 )); then
    usage >&2
    exit 2
fi

if [[ "${cache_mode}" != "warm" && "${cache_mode}" != "cold" && "${cache_mode}" != "reuse" ]]; then
    usage >&2
    exit 2
fi

validate_seed_list() {
    local seeds="$1"
    local seed_list
    IFS=',' read -r -a seed_list <<< "${seeds}"
    for seed in "${seed_list[@]}"; do
        if [[ -z "${seed}" || ! "${seed}" =~ ^[0-9]+$ ]]; then
            usage >&2
            exit 2
        fi
    done
}

validate_seed_list "${tail_seeds}"
validate_seed_list "${hardening_seeds}"

cmake --build "${build_dir}" --target rubik-bench rubik-cache-setup

bench="${build_dir}/rubik-bench"
cache_setup="${build_dir}/rubik-cache-setup"
if [[ ! -x "${bench}" || ! -x "${cache_setup}" ]]; then
    echo "required benchmark binaries are missing in ${build_dir}" >&2
    exit 1
fi

mkdir -p "${cache_dir}" "${output_dir}"
if [[ "${cache_mode}" == "cold" ]]; then
    rm -rf "${cache_dir}"
    mkdir -p "${cache_dir}"
fi

manifest_file="${output_dir}/manifest.csv"
{
    echo "key,value"
    echo "git_revision,$(git -C "${repo_root}" rev-parse --short HEAD 2>/dev/null || echo unknown)"
    echo "build_dir,${build_dir}"
    echo "cache_dir,${cache_dir}"
    echo "output_dir,${output_dir}"
    echo "tail_seeds,${tail_seeds}"
    echo "hardening_seeds,${hardening_seeds}"
    echo "timeout_ms,${timeout_ms}"
    echo "threads,${threads}"
    echo "max_memory_mb,${max_memory_mb}"
    echo "deep_opt14_count,${deep_opt14_count}"
    echo "deep_opt15_count,${deep_opt15_count}"
    echo "cache_mode,${cache_mode}"
} > "${manifest_file}"

cache_setup_output="${output_dir}/cache_setup.csv"
if [[ "${cache_mode}" == "reuse" ]]; then
    {
        echo "cache_setup,status,Skipped"
        echo "cache_setup,message,cache setup skipped by cache-mode reuse"
    } > "${cache_setup_output}"
else
    "${cache_setup}" \
        --profile auto \
        --threads "${threads}" \
        --max-memory-mb "${max_memory_mb}" \
        --cache-dir "${cache_dir}" \
        --format csv \
        | tee "${cache_setup_output}"
fi

tail_output_dir="${output_dir}/optimal-auto-tail"
hardening_output_dir="${output_dir}/optimal-auto-hardening"

"${script_dir}/run_benchmark_suite.sh" \
    --suite optimal-auto-tail \
    --cache-mode warm \
    --build-dir "${build_dir}" \
    --cache-dir "${cache_dir}" \
    --output-dir "${tail_output_dir}" \
    --seeds "${tail_seeds}" \
    --optimal-timeout-ms "${timeout_ms}" \
    --deep-opt15-count "${deep_opt15_count}" \
    --threads "${threads}" \
    --max-memory-mb "${max_memory_mb}"

"${script_dir}/run_benchmark_suite.sh" \
    --suite optimal-auto-hardening \
    --cache-mode warm \
    --build-dir "${build_dir}" \
    --cache-dir "${cache_dir}" \
    --output-dir "${hardening_output_dir}" \
    --seeds "${hardening_seeds}" \
    --optimal-timeout-ms "${timeout_ms}" \
    --deep-opt14-count "${deep_opt14_count}" \
    --deep-opt15-count "${deep_opt15_count}" \
    --threads "${threads}" \
    --max-memory-mb "${max_memory_mb}"

echo "v6 tail baseline manifest: ${manifest_file}"
echo "v6 tail baseline auto-tail summary: ${tail_output_dir}/warm_optimal_auto_tail_summary.csv"
echo "v6 tail baseline hardening summary: ${hardening_output_dir}/warm_optimal_auto_hardening_summary.csv"
