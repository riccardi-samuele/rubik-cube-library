#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

build_dir="out/release-native-lto"
cache_dir="${RUBIK_TABLE_CACHE_DIR:-/tmp/rubik_cube_library_v4_deep_split_ab_cache}"
output_dir="benchmark-results/v4-deep-split-ab"
cases_file=""
timeout_ms="30000"
max_depth="15"
threads="0"
max_memory_mb="2048"
slowest_limit="50"
cache_mode="warm"

usage() {
    cat <<'USAGE'
Usage: scripts/run_v4_deep_split_ab.sh [options]

Options:
  --cases-file FILE    slowest CSV from run_v4_tail_discovery.sh
  --build-dir DIR      CMake build directory, default: out/release-native-lto
  --cache-dir DIR      pruning table cache directory
  --output-dir DIR     output directory, default: benchmark-results/v4-deep-split-ab
  --timeout-ms N       per-case timeout, default: 30000
  --max-depth N        solver max depth, default: 15
  --threads N          solver threads, default: 0
  --max-memory-mb N    solver memory limit, default: 2048
  --slowest-limit N    slowest rows to extract, default: 50
  --cache-mode MODE    warm|cold|reuse, default: warm
  -h, --help           show this help
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
        --cases-file)
            require_value "$@"
            cases_file="$2"
            shift 2
            ;;
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
        --timeout-ms)
            require_value "$@"
            timeout_ms="$2"
            shift 2
            ;;
        --max-depth)
            require_value "$@"
            max_depth="$2"
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

if [[ -z "${cases_file}" ]]; then
    usage >&2
    exit 2
fi

for numeric in "${timeout_ms}" "${max_depth}" "${threads}" "${max_memory_mb}" "${slowest_limit}"; do
    if [[ ! "${numeric}" =~ ^[0-9]+$ ]]; then
        usage >&2
        exit 2
    fi
done

if (( timeout_ms < 1 || max_depth < 1 || max_memory_mb < 1 || slowest_limit < 1 )); then
    usage >&2
    exit 2
fi

if [[ "${cache_mode}" != "warm" && "${cache_mode}" != "cold" && "${cache_mode}" != "reuse" ]]; then
    usage >&2
    exit 2
fi

mkdir -p "${output_dir}"

baseline_dir="${output_dir}/baseline"
candidate_dir="${output_dir}/deep-split"
comparison_file="${output_dir}/comparison.csv"

"${script_dir}/run_v4_tail_corpus.sh" \
    --cases-file "${cases_file}" \
    --build-dir "${build_dir}" \
    --cache-dir "${cache_dir}" \
    --output-dir "${baseline_dir}" \
    --timeout-ms "${timeout_ms}" \
    --max-depth "${max_depth}" \
    --threads "${threads}" \
    --max-memory-mb "${max_memory_mb}" \
    --slowest-limit "${slowest_limit}" \
    --cache-mode "${cache_mode}"

RUBIK_EXPERIMENTAL_DEEP_ROOT_SPLIT=1 "${script_dir}/run_v4_tail_corpus.sh" \
    --cases-file "${cases_file}" \
    --build-dir "${build_dir}" \
    --cache-dir "${cache_dir}" \
    --output-dir "${candidate_dir}" \
    --timeout-ms "${timeout_ms}" \
    --max-depth "${max_depth}" \
    --threads "${threads}" \
    --max-memory-mb "${max_memory_mb}" \
    --slowest-limit "${slowest_limit}" \
    --cache-mode reuse

"${script_dir}/compare_v4_tail_runs.py" \
    --baseline "${baseline_dir}/summary.csv" \
    --candidate "${candidate_dir}/summary.csv" \
    --output "${comparison_file}"

echo "v4 deep split baseline summary: ${baseline_dir}/summary.csv"
echo "v4 deep split candidate summary: ${candidate_dir}/summary.csv"
echo "v4 deep split comparison: ${comparison_file}"
