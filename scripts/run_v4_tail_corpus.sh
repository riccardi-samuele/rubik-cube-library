#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"

build_dir="out/release-native-lto"
cache_dir="${RUBIK_TABLE_CACHE_DIR:-/tmp/rubik_cube_library_v4_tail_corpus_cache}"
output_dir="benchmark-results/v4-tail-corpus"
cases_file=""
timeout_ms="30000"
max_depth="15"
threads="0"
max_memory_mb="2048"
slowest_limit="50"
cache_mode="warm"

usage() {
    cat <<'USAGE'
Usage: scripts/run_v4_tail_corpus.sh [options]

Options:
  --cases-file FILE    slowest CSV from run_v4_tail_discovery.sh
  --build-dir DIR      CMake build directory, default: out/release-native-lto
  --cache-dir DIR      pruning table cache directory
  --output-dir DIR     output directory, default: benchmark-results/v4-tail-corpus
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

if [[ ! -f "${cases_file}" ]]; then
    echo "v4 tail corpus failed: cases file not found: ${cases_file}" >&2
    exit 1
fi

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

cache_setup_output="${output_dir}/cache_setup.csv"
if [[ "${cache_mode}" == "warm" || "${cache_mode}" == "cold" ]]; then
    "${cache_setup}" \
        --profile auto \
        --threads "${threads}" \
        --max-memory-mb "${max_memory_mb}" \
        --cache-dir "${cache_dir}" \
        --format csv \
        | tee "${cache_setup_output}"
else
    {
        echo "cache_setup,status,Skipped"
        echo "cache_setup,message,cache setup skipped by cache-mode reuse"
    } > "${cache_setup_output}"
fi

manifest_file="${output_dir}/manifest.csv"
summary_file="${output_dir}/summary.csv"
slowest_file="${output_dir}/slowest.csv"

{
    echo "key,value"
    echo "git_revision,$(git -C "${repo_root}" rev-parse --short HEAD 2>/dev/null || echo unknown)"
    echo "build_dir,${build_dir}"
    echo "cache_dir,${cache_dir}"
    echo "cache_mode,${cache_mode}"
    echo "output_dir,${output_dir}"
    echo "cases_file,${cases_file}"
    echo "timeout_ms,${timeout_ms}"
    echo "max_depth,${max_depth}"
    echo "threads,${threads}"
    echo "max_memory_mb,${max_memory_mb}"
} > "${manifest_file}"

{
    echo "case_name,seed,start_index,case_depth,status,optimal,move_count,initial_lower_bound,elapsed_ms,nodes_expanded,warmup_elapsed_ms,wall_elapsed_ms,output_file"
} > "${summary_file}"

tmp_cases="$(mktemp)"
trap 'rm -f "${tmp_cases}"' EXIT

awk -F, '
    NR == 1 { next }
    NF >= 8 {
        print $7 "," $8
    }
' "${cases_file}" > "${tmp_cases}"

if [[ ! -s "${tmp_cases}" ]]; then
    echo "v4 tail corpus failed: cases file contains no replayable rows: ${cases_file}" >&2
    exit 1
fi

suite_status=0
while IFS=, read -r case_name case_depth; do
    if [[ ! "${case_name}" =~ ^random_([0-9]+)_([0-9]+)$ ]]; then
        echo "v4 tail corpus failed: unsupported case row: ${case_name},${case_depth}" >&2
        exit 1
    fi

    seed="${BASH_REMATCH[1]}"
    start_index="${BASH_REMATCH[2]}"
    if [[ ! "${case_depth}" =~ ^[0-9]+$ ]]; then
        echo "v4 tail corpus failed: unsupported case row: ${case_name},${case_depth}" >&2
        exit 1
    fi

    name="v4_tail_corpus_${case_name}_depth_${case_depth}"
    output_file="${output_dir}/${name}.csv"
    started_at="$(date +%s%3N)"
    {
        echo "benchmark,name,${name}"
        echo "benchmark,cache_dir,${cache_dir}"
        echo "benchmark,case_name,${case_name}"
        echo "benchmark,seed,${seed}"
        echo "benchmark,start_index,${start_index}"
    } > "${output_file}"

    set +e
    RUBIK_TABLE_CACHE_DIR="${cache_dir}" "${bench}" \
        --mode optimal \
        --profile auto \
        --threads "${threads}" \
        --max-memory-mb "${max_memory_mb}" \
        --timeout-ms "${timeout_ms}" \
        --max-depth "${max_depth}" \
        --case-set random \
        --random-count 1 \
        --random-depth "${case_depth}" \
        --random-seed "${seed}" \
        --random-start-index "${start_index}" \
        --slowest-count 1 \
        --diagnose-optimal \
        >> "${output_file}"
    command_status="$?"
    set -e

    ended_at="$(date +%s%3N)"
    echo "benchmark,wall_elapsed_ms,$((ended_at - started_at))" >> "${output_file}"

    status="$(awk -F, '$1 ~ /^random_/ { print $4; exit }' "${output_file}")"
    optimal="$(awk -F, '$1 ~ /^random_/ { print $5; exit }' "${output_file}")"
    move_count="$(awk -F, '$1 ~ /^random_/ { print $6; exit }' "${output_file}")"
    lower_bound="$(awk -F, '$1 ~ /^random_/ { print $7; exit }' "${output_file}")"
    elapsed="$(awk -F, '$1 ~ /^random_/ { print $8; exit }' "${output_file}")"
    nodes="$(awk -F, '$1 ~ /^random_/ { print $9; exit }' "${output_file}")"
    warmup="$(awk -F, '$1 == "benchmark" && $2 == "warmup_elapsed_ms" { print $3; exit }' "${output_file}")"
    wall="$(awk -F, '$1 == "benchmark" && $2 == "wall_elapsed_ms" { print $3; exit }' "${output_file}")"
    echo "${case_name},${seed},${start_index},${case_depth},${status:-Unknown},${optimal:-false},${move_count:-0},${lower_bound:-0},${elapsed:-0},${nodes:-0},${warmup:-0},${wall:-0},${output_file}" >> "${summary_file}"

    if (( command_status != 0 )); then
        suite_status=1
    fi
done < "${tmp_cases}"

"${repo_root}/scripts/extract_slowest_cases.sh" \
    --input-dir "${output_dir}" \
    --output "${slowest_file}" \
    --limit "${slowest_limit}"

echo "v4 tail corpus manifest: ${manifest_file}"
echo "v4 tail corpus summary: ${summary_file}"
echo "v4 tail corpus slowest: ${slowest_file}"

exit "${suite_status}"
