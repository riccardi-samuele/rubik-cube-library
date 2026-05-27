#!/usr/bin/env bash
set -euo pipefail

build_dir="out/release-native-lto"
cache_dir="${RUBIK_TABLE_CACHE_DIR:-/tmp/rubik_cube_library_root_ordering_cache}"
output_dir="benchmark-results/root-ordering-experiments"
seeds="987654321,424242,1009,2016,666,555,99,888"
timeout_ms="30000"
max_depth="15"
random_depth="15"
threads="0"
max_memory_mb="2048"
variants="default,reverse_tie,phase2_tiebreak"
smoke=0

usage() {
    cat <<'USAGE'
Usage: scripts/benchmark_root_ordering_experiments.sh [options]

Options:
  --build-dir DIR       CMake build directory, default: out/release-native-lto
  --cache-dir DIR       pruning table cache directory
  --output-dir DIR      output directory, default: benchmark-results/root-ordering-experiments
  --seeds LIST          comma-separated random seeds, default: 987654321,424242,1009,2016,666,555,99,888
  --timeout-ms N        per-case timeout, default: 30000
  --max-depth N         solver max depth, default: 15
  --random-depth N      random scramble depth, default: 15
  --threads N           solver threads, default: 0
  --max-memory-mb N     solver memory limit, default: 2048
  --variants LIST       comma-separated variants. Variants: default,reverse_tie,phase2_tiebreak
  --smoke               run one short validation case
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
        --variants)
            require_value "$@"
            variants="$2"
            shift 2
            ;;
        --smoke)
            smoke=1
            shift
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

if (( smoke == 1 )); then
    seeds="99"
    variants="default,reverse_tie"
    timeout_ms="5000"
    max_depth="10"
    random_depth="8"
fi

for numeric in "${timeout_ms}" "${max_depth}" "${random_depth}" "${threads}" "${max_memory_mb}"; do
    if [[ ! "${numeric}" =~ ^[0-9]+$ ]]; then
        usage >&2
        exit 2
    fi
done

if (( timeout_ms < 1 || max_depth < 1 || random_depth < 1 || max_memory_mb < 1 )); then
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

IFS=',' read -r -a variant_list <<< "${variants}"
for variant in "${variant_list[@]}"; do
    case "${variant}" in
        default|reverse_tie|phase2_tiebreak)
            ;;
        *)
            usage >&2
            exit 2
            ;;
    esac
done

cmake --build "${build_dir}" --target rubik-bench

bench="${build_dir}/rubik-bench"
if [[ ! -x "${bench}" ]]; then
    echo "rubik-bench not found or not executable: ${bench}" >&2
    exit 1
fi

mkdir -p "${cache_dir}" "${output_dir}"

summary_file="${output_dir}/summary.csv"
comparison_file="${output_dir}/comparison.csv"
manifest_file="${output_dir}/manifest.csv"

{
    echo "key,value"
    echo "git_revision,$(git rev-parse --short HEAD 2>/dev/null || echo unknown)"
    echo "build_dir,${build_dir}"
    echo "cache_dir,${cache_dir}"
    echo "output_dir,${output_dir}"
    echo "seeds,${seeds}"
    echo "timeout_ms,${timeout_ms}"
    echo "max_depth,${max_depth}"
    echo "random_depth,${random_depth}"
    echo "threads,${threads}"
    echo "max_memory_mb,${max_memory_mb}"
    echo "variants,${variants}"
} > "${manifest_file}"

{
    echo "variant,seed,status,optimal,move_count,initial_lower_bound,elapsed_ms,nodes_expanded,solution_rank,root_ordering_mode,warmup_elapsed_ms,output_file"
} > "${summary_file}"

variant_env() {
    case "$1" in
        default)
            return 0
            ;;
        reverse_tie)
            echo "RUBIK_EXPERIMENTAL_ROOT_ORDERING=reverse_tie"
            ;;
        phase2_tiebreak)
            echo "RUBIK_EXPERIMENTAL_ROOT_ORDERING=phase2_tiebreak"
            ;;
    esac
}

extract_profile_value() {
    local profile="$1"
    local key="$2"
    local value=""
    if [[ "${profile}" =~ (^|;)${key}=([^;|]+) ]]; then
        value="${BASH_REMATCH[2]}"
    fi
    echo "${value}"
}

run_case() {
    local variant="$1"
    local seed="$2"
    local output_file="${output_dir}/${variant}_seed_${seed}.csv"

    echo "==> ${variant} seed ${seed}"
    echo "    output: ${output_file}"

    {
        echo "benchmark,name,root_ordering_${variant}_seed_${seed}"
        echo "benchmark,variant,${variant}"
        echo "benchmark,cache_dir,${cache_dir}"
    } > "${output_file}"

    mapfile -t env_vars < <(variant_env "${variant}")

    set +e
    env RUBIK_TABLE_CACHE_DIR="${cache_dir}" "${env_vars[@]}" \
        "${bench}" \
        --mode optimal \
        --profile auto \
        --threads "${threads}" \
        --max-memory-mb "${max_memory_mb}" \
        --timeout-ms "${timeout_ms}" \
        --max-depth "${max_depth}" \
        --case-set random \
        --random-count 1 \
        --random-depth "${random_depth}" \
        --random-seed "${seed}" \
        --slowest-count 1 \
        --diagnose-optimal \
        >> "${output_file}"
    local command_status="$?"
    set -e

    local status optimal move_count initial_lower_bound elapsed nodes warmup profile rank mode
    status="$(awk -F, '$1 ~ /^random_/ { print $4; exit }' "${output_file}")"
    optimal="$(awk -F, '$1 ~ /^random_/ { print $5; exit }' "${output_file}")"
    move_count="$(awk -F, '$1 ~ /^random_/ { print $6; exit }' "${output_file}")"
    initial_lower_bound="$(awk -F, '$1 ~ /^random_/ { print $7; exit }' "${output_file}")"
    elapsed="$(awk -F, '$1 ~ /^random_/ { print $8; exit }' "${output_file}")"
    nodes="$(awk -F, '$1 ~ /^random_/ { print $9; exit }' "${output_file}")"
    warmup="$(awk -F, '$1 == "benchmark" && $2 == "warmup_elapsed_ms" { print $3; exit }' "${output_file}")"
    profile="$(awk -F, '$1 ~ /^random_/ { print $16; exit }' "${output_file}" | tr -d '"')"
    rank="$(extract_profile_value "${profile}" "solution_rank")"
    mode="$(extract_profile_value "${profile}" "root_ordering_mode")"

    echo "${variant},${seed},${status:-Unknown},${optimal:-false},${move_count:-0},${initial_lower_bound:-0},${elapsed:-0},${nodes:-0},${rank:-0},${mode:-unknown},${warmup:-0},${output_file}" \
        >> "${summary_file}"

    return "${command_status}"
}

suite_status=0
for seed in "${seed_list[@]}"; do
    for variant in "${variant_list[@]}"; do
        run_case "${variant}" "${seed}" || suite_status=1
    done
done

awk -F, '
    NR == 1 { next }
    {
        variant = $1
        seed = $2
        elapsed[seed, variant] = $7 + 0
        nodes[seed, variant] = $8 + 0
        rank[seed, variant] = $9 + 0
        seen[seed] = 1
    }
    END {
        print "seed,default_elapsed_ms,reverse_tie_elapsed_ms,phase2_tiebreak_elapsed_ms,default_nodes,reverse_tie_nodes,phase2_tiebreak_nodes,default_rank,reverse_tie_rank,phase2_tiebreak_rank,winner"
        for (seed in seen) {
            default_elapsed = elapsed[seed, "default"] + 0
            reverse_elapsed = elapsed[seed, "reverse_tie"] + 0
            phase2_elapsed = elapsed[seed, "phase2_tiebreak"] + 0
            winner = "default"
            best = default_elapsed
            if (reverse_elapsed > 0 && (best == 0 || reverse_elapsed < best)) {
                best = reverse_elapsed
                winner = "reverse_tie"
            }
            if (phase2_elapsed > 0 && (best == 0 || phase2_elapsed < best)) {
                winner = "phase2_tiebreak"
            }
            printf "%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s\n",
                seed,
                default_elapsed,
                reverse_elapsed,
                phase2_elapsed,
                nodes[seed, "default"] + 0,
                nodes[seed, "reverse_tie"] + 0,
                nodes[seed, "phase2_tiebreak"] + 0,
                rank[seed, "default"] + 0,
                rank[seed, "reverse_tie"] + 0,
                rank[seed, "phase2_tiebreak"] + 0,
                winner
        }
    }
' "${summary_file}" | sort -t, -k1,1n > "${comparison_file}"

echo "root ordering experiment manifest: ${manifest_file}"
echo "root ordering experiment summary: ${summary_file}"
echo "root ordering experiment comparison: ${comparison_file}"

exit "${suite_status}"
