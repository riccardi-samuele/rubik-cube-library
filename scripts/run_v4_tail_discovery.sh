#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"

build_dir="out/release-native-lto"
cache_dir="${RUBIK_TABLE_CACHE_DIR:-/tmp/rubik_cube_library_v4_tail_discovery_cache}"
output_dir="benchmark-results/v4-tail-discovery"
seeds="987654321,424242,1009,2016,666,555,99,888,12345,8675309,20260525"
timeout_ms="30000"
max_depth="15"
random_depth="15"
random_count="1"
threads="0"
max_memory_mb="2048"
slowest_limit="50"
cache_mode="warm"

usage() {
    cat <<'USAGE'
Usage: scripts/run_v4_tail_discovery.sh [options]

Options:
  --build-dir DIR       CMake build directory, default: out/release-native-lto
  --cache-dir DIR       pruning table cache directory
  --output-dir DIR      output directory, default: benchmark-results/v4-tail-discovery
  --seeds LIST          comma-separated random seeds
  --timeout-ms N        per-case timeout, default: 30000
  --max-depth N         solver max depth, default: 15
  --random-depth N      random scramble depth, default: 15
  --random-count N      random cases per seed, default: 1
  --threads N           solver threads, default: 0
  --max-memory-mb N     solver memory limit, default: 2048
  --slowest-limit N     slowest rows to extract, default: 50
  --cache-mode MODE     warm|cold|reuse, default: warm
  -h, --help            show this help
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
            timeout_ms="$2"
            shift 2
            ;;
        --max-depth)
            require_value "$@"
            max_depth="$2"
            shift 2
            ;;
        --random-depth)
            require_value "$@"
            random_depth="$2"
            shift 2
            ;;
        --random-count)
            require_value "$@"
            random_count="$2"
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

for numeric in "${timeout_ms}" "${max_depth}" "${random_depth}" "${random_count}" "${threads}" "${max_memory_mb}" "${slowest_limit}"; do
    if [[ ! "${numeric}" =~ ^[0-9]+$ ]]; then
        usage >&2
        exit 2
    fi
done

if (( timeout_ms < 1 || max_depth < 1 || random_depth < 1 || random_count < 1 || max_memory_mb < 1 || slowest_limit < 1 )); then
    usage >&2
    exit 2
fi

if [[ "${cache_mode}" != "warm" && "${cache_mode}" != "cold" && "${cache_mode}" != "reuse" ]]; then
    usage >&2
    exit 2
fi

IFS=',' read -r -a seed_list <<< "${seeds}"
for seed in "${seed_list[@]}"; do
    if [[ -z "${seed}" || ! "${seed}" =~ ^[0-9]+$ ]]; then
        usage >&2
        exit 2
    fi
done

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
    echo "seeds,${seeds}"
    echo "timeout_ms,${timeout_ms}"
    echo "max_depth,${max_depth}"
    echo "random_depth,${random_depth}"
    echo "random_count,${random_count}"
    echo "threads,${threads}"
    echo "max_memory_mb,${max_memory_mb}"
} > "${manifest_file}"

{
    echo "seed,status,optimal,move_count,initial_lower_bound,elapsed_ms,nodes_expanded,warmup_elapsed_ms,wall_elapsed_ms,output_file"
} > "${summary_file}"

suite_status=0
for seed in "${seed_list[@]}"; do
    name="v4_tail_discovery_auto_random_${random_count}_depth_${random_depth}_seed_${seed}"
    output_file="${output_dir}/${name}.csv"
    started_at="$(date +%s%3N)"
    {
        echo "benchmark,name,${name}"
        echo "benchmark,cache_dir,${cache_dir}"
        echo "benchmark,seed,${seed}"
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
        --random-count "${random_count}" \
        --random-depth "${random_depth}" \
        --random-seed "${seed}" \
        --slowest-count "${random_count}" \
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
    echo "${seed},${status:-Unknown},${optimal:-false},${move_count:-0},${lower_bound:-0},${elapsed:-0},${nodes:-0},${warmup:-0},${wall:-0},${output_file}" >> "${summary_file}"

    if (( command_status != 0 )); then
        suite_status=1
    fi
done

"${repo_root}/scripts/extract_slowest_cases.sh" \
    --input-dir "${output_dir}" \
    --output "${slowest_file}" \
    --limit "${slowest_limit}"

echo "v4 tail discovery manifest: ${manifest_file}"
echo "v4 tail discovery summary: ${summary_file}"
echo "v4 tail discovery slowest: ${slowest_file}"

exit "${suite_status}"
