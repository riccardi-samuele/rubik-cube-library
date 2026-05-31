#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"

build_dir="out/release-native-lto"
cache_dir="${RUBIK_TABLE_CACHE_DIR:-/tmp/rubik_cube_library_v6_tail_baseline_cache}"
output_dir="benchmark-results/v6-conservative-root-targeted-corpus"
timeout_ms="30000"
threads="0"
max_memory_mb="2048"
depth="15"
seeds="42,99,424242,12345,20260525,314159,271828,987654321,7,123456789"
random_count="2"
random_start_index="1"
target_profiles="8:7:1,8:11:1,9:14:0"
min_target_cases="1"
sweep_script="${script_dir}/run_v6_conservative_root_ordering_sweep.sh"
summary_script="${script_dir}/summarize_v6_targeted_corpus.py"

usage() {
    cat <<'USAGE'
Usage: scripts/run_v6_conservative_root_targeted_corpus.sh [options]

Options:
  --build-dir DIR           CMake build directory, default: out/release-native-lto
  --cache-dir DIR           pruning table cache directory
  --output-dir DIR          output directory, default: benchmark-results/v6-conservative-root-targeted-corpus
  --timeout-ms N            per-case timeout, default: 30000
  --threads N               solver threads, default: 0
  --max-memory-mb N         solver memory limit, default: 2048
  --depth N                 random depth and optimal max depth, default: 15
  --seeds LIST              comma-separated random seeds
  --random-count N          generated cases per seed, default: 2
  --random-start-index N    first generated case index, default: 1
  --target-profiles LIST    comma-separated lb:strong_min:first_diff profiles
  --min-target-cases N      minimum matching cases required, default: 1
  --sweep-script FILE       ordering sweep runner
  --summary-script FILE     targeted corpus summary script
  -h, --help                show this help
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
        --depth)
            require_value "$@"
            depth="$2"
            shift 2
            ;;
        --seeds)
            require_value "$@"
            seeds="$2"
            shift 2
            ;;
        --random-count)
            require_value "$@"
            random_count="$2"
            shift 2
            ;;
        --random-start-index)
            require_value "$@"
            random_start_index="$2"
            shift 2
            ;;
        --target-profiles)
            require_value "$@"
            target_profiles="$2"
            shift 2
            ;;
        --min-target-cases)
            require_value "$@"
            min_target_cases="$2"
            shift 2
            ;;
        --sweep-script)
            require_value "$@"
            sweep_script="$2"
            shift 2
            ;;
        --summary-script)
            require_value "$@"
            summary_script="$2"
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

for numeric in "${timeout_ms}" "${threads}" "${max_memory_mb}" "${depth}" "${random_count}" "${random_start_index}" "${min_target_cases}"; do
    if [[ ! "${numeric}" =~ ^[0-9]+$ ]]; then
        usage >&2
        exit 2
    fi
done

if (( timeout_ms < 1 || max_memory_mb < 1 || depth < 1 || random_count < 1 || random_start_index < 1 || min_target_cases < 1 )); then
    usage >&2
    exit 2
fi

if [[ ! "${seeds}" =~ ^[0-9]+(,[0-9]+)*$ ]]; then
    usage >&2
    exit 2
fi

if [[ ! "${target_profiles}" =~ ^[0-9]+:[0-9]+:[01](,[0-9]+:[0-9]+:[01])*$ ]]; then
    usage >&2
    exit 2
fi

if [[ ! -x "${sweep_script}" ]]; then
    echo "v6 conservative root targeted corpus failed: sweep script is not executable: ${sweep_script}" >&2
    exit 1
fi

if [[ ! -x "${summary_script}" ]]; then
    echo "v6 conservative root targeted corpus failed: summary script is not executable: ${summary_script}" >&2
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
discovery_dir="${output_dir}/discovery"
mkdir -p "${discovery_dir}"
manifest_file="${output_dir}/manifest.csv"
cache_setup_output="${output_dir}/cache_setup.csv"
target_corpus="${output_dir}/targeted_corpus.csv"
target_summary="${output_dir}/targeted_cases.csv"

{
    echo "key,value"
    echo "git_revision,$(git -C "${repo_root}" rev-parse --short HEAD 2>/dev/null || echo unknown)"
    echo "build_dir,${build_dir}"
    echo "cache_dir,${cache_dir}"
    echo "output_dir,${output_dir}"
    echo "timeout_ms,${timeout_ms}"
    echo "threads,${threads}"
    echo "max_memory_mb,${max_memory_mb}"
    echo "depth,${depth}"
    echo "seeds,${seeds}"
    echo "random_count,${random_count}"
    echo "random_start_index,${random_start_index}"
    echo "target_profiles,${target_profiles}"
    echo "min_target_cases,${min_target_cases}"
    echo "sweep_script,${sweep_script}"
    echo "summary_script,${summary_script}"
    echo "cache_setup_output,${cache_setup_output}"
} > "${manifest_file}"

"${cache_setup}" \
    --profile auto \
    --threads "${threads}" \
    --max-memory-mb "${max_memory_mb}" \
    --cache-dir "${cache_dir}" \
    --dry-run \
    --format csv \
    | tee "${cache_setup_output}"
cache_warm="$(awk -F, '$1 == "cache_setup" && $2 == "cache_warm" { print $3; exit }' "${cache_setup_output}")"
bytes_missing="$(awk -F, '$1 == "cache_setup" && $2 == "bytes_missing" { print $3; exit }' "${cache_setup_output}")"
if [[ "${cache_warm}" != "true" ]]; then
    echo "cache is not warm for V6 conservative root targeted corpus; run rubik-cache-setup first (bytes_missing=${bytes_missing:-unknown})" >&2
    exit 1
fi

extract_profile_value() {
    local profile="$1"
    local key="$2"
    sed -n "s/.*${key}=\\([^;\\\"]*\\).*/\\1/p" <<< "${profile}"
}

profile_is_targeted() {
    local profile="$1"
    local lb strong_min first_diff candidate
    lb="$(extract_profile_value "${profile}" "adaptive_lb")"
    strong_min="$(extract_profile_value "${profile}" "adaptive_strong_min_count")"
    first_diff="$(extract_profile_value "${profile}" "adaptive_first_diff")"
    IFS=',' read -r -a candidate_profiles <<< "${target_profiles}"
    for candidate in "${candidate_profiles[@]}"; do
        if [[ "${candidate}" == "${lb}:${strong_min}:${first_diff}" ]]; then
            return 0
        fi
    done
    return 1
}

{
    echo "suite,seed,start_index,depth,count,expected_reason"
} > "${target_corpus}"
{
    echo "seed,start_index,case_name,adaptive_lb,adaptive_strong_min_count,adaptive_first_diff,elapsed_ms,nodes_expanded,profile"
} > "${target_summary}"

IFS=',' read -r -a seed_list <<< "${seeds}"
for seed in "${seed_list[@]}"; do
    output_file="${discovery_dir}/warm_v6_conservative_root_target_depth_${depth}_seed_${seed}_start_${random_start_index}_count_${random_count}.csv"
    {
        echo "benchmark,name,v6_conservative_root_target_depth_${depth}_seed_${seed}_start_${random_start_index}_count_${random_count}"
        echo "benchmark,variant,discovery"
        echo "benchmark,cache_dir,${cache_dir}"
    } > "${output_file}"

    env RUBIK_TABLE_CACHE_DIR="${cache_dir}" "${bench}" \
        --mode optimal \
        --profile auto \
        --threads "${threads}" \
        --max-memory-mb "${max_memory_mb}" \
        --timeout-ms "${timeout_ms}" \
        --max-depth "${depth}" \
        --case-set random \
        --random-count "${random_count}" \
        --random-depth "${depth}" \
        --random-seed "${seed}" \
        --random-start-index "${random_start_index}" \
        --slowest-count "${random_count}" \
        --diagnose-optimal \
        >> "${output_file}"

    while IFS=, read -r case_name case_depth scramble status optimal moves initial_lower_bound elapsed_ms nodes_expanded nodes_per_ms max_depth timeout nodes_by_depth solution ordering profile; do
        if [[ "${case_name}" == "case" || ! "${case_name}" =~ ^random_ ]]; then
            continue
        fi
        if [[ "${status}" != "Optimal" || "${optimal}" != "true" ]]; then
            continue
        fi
        adaptive_reason="$(extract_profile_value "${profile}" "adaptive_reason")"
        if [[ "${adaptive_reason}" != "conservative_root" ]]; then
            continue
        fi
        if ! profile_is_targeted "${profile}"; then
            continue
        fi
        case_index="$(sed -n 's/^random_[0-9][0-9]*_\([0-9][0-9]*\)$/\1/p' <<< "${case_name}")"
        if [[ -z "${case_index}" ]]; then
            continue
        fi
        lb="$(extract_profile_value "${profile}" "adaptive_lb")"
        strong_min="$(extract_profile_value "${profile}" "adaptive_strong_min_count")"
        first_diff="$(extract_profile_value "${profile}" "adaptive_first_diff")"
        echo "hardening,${seed},${case_index},${depth},1,conservative_root" >> "${target_corpus}"
        echo "${seed},${case_index},${case_name},${lb},${strong_min},${first_diff},${elapsed_ms},${nodes_expanded},${profile}" >> "${target_summary}"
    done < "${output_file}"
done

target_count="$(($(wc -l < "${target_corpus}") - 1))"
if (( target_count < min_target_cases )); then
    echo "v6 conservative root targeted corpus failed: only ${target_count} target cases found, expected at least ${min_target_cases}" >&2
    exit 1
fi

"${sweep_script}" \
    --build-dir "${build_dir}" \
    --cache-dir "${cache_dir}" \
    --output-dir "${output_dir}/ordering-sweep" \
    --corpus-file "${target_corpus}" \
    --timeout-ms "${timeout_ms}" \
    --threads "${threads}" \
    --max-memory-mb "${max_memory_mb}" \
    --candidates phase2_tiebreak

"${summary_script}" \
    --targeted-cases "${target_summary}" \
    --comparison "${output_dir}/ordering-sweep/phase2_tiebreak/comparison.csv" \
    --case-output "${output_dir}/case_summary.csv" \
    --profile-output "${output_dir}/profile_summary.csv"

echo "v6 conservative root targeted corpus manifest: ${manifest_file}"
echo "v6 conservative root targeted corpus: ${target_corpus}"
echo "v6 conservative root targeted cases: ${target_summary}"
echo "v6 conservative root targeted sweep: ${output_dir}/ordering-sweep/summary.csv"
echo "v6 conservative root targeted case summary: ${output_dir}/case_summary.csv"
echo "v6 conservative root targeted profile summary: ${output_dir}/profile_summary.csv"
