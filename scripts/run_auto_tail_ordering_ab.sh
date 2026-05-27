#!/usr/bin/env bash
set -euo pipefail

build_dir="out/release-native-lto"
cache_dir="${RUBIK_TABLE_CACHE_DIR:-/tmp/rubik_cube_library_optimal_auto_tail_cache}"
output_dir="benchmark-results/optimal-auto-tail-ordering-ab"
seeds="987654321,424242,1009,2016,666,555,99,888"
timeout_ms="30000"
max_depth="15"
threads="0"
max_memory_mb="2048"
variants="base,strong"

usage() {
    cat <<'USAGE'
Usage: scripts/run_auto_tail_ordering_ab.sh [options]

Options:
  --build-dir DIR       CMake build directory, default: out/release-native-lto
  --cache-dir DIR       pruning table cache directory
  --output-dir DIR      output directory, default: benchmark-results/optimal-auto-tail-ordering-ab
  --seeds LIST          comma-separated random seeds, default: 987654321,424242,1009,2016,666,555,99,888
  --timeout-ms N        per-case timeout, default: 30000
  --max-depth N         solver max depth, default: 15
  --threads N           solver threads, default: 0
  --max-memory-mb N     solver memory limit, default: 2048
  --variants LIST       comma-separated variants. Variants: base,strong
  -h, --help            show this help
USAGE
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir)
            build_dir="$2"
            shift 2
            ;;
        --cache-dir)
            cache_dir="$2"
            shift 2
            ;;
        --output-dir)
            output_dir="$2"
            shift 2
            ;;
        --seeds)
            seeds="$2"
            shift 2
            ;;
        --timeout-ms)
            timeout_ms="$2"
            shift 2
            ;;
        --max-depth)
            max_depth="$2"
            shift 2
            ;;
        --threads)
            threads="$2"
            shift 2
            ;;
        --max-memory-mb)
            max_memory_mb="$2"
            shift 2
            ;;
        --variants)
            variants="$2"
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

for numeric in "${timeout_ms}" "${max_depth}" "${threads}" "${max_memory_mb}"; do
    if [[ ! "${numeric}" =~ ^[0-9]+$ ]]; then
        usage >&2
        exit 2
    fi
done

if (( timeout_ms < 1 || max_depth < 1 || max_memory_mb < 1 )); then
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
    if [[ "${variant}" != "base" && "${variant}" != "strong" ]]; then
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
    echo "threads,${threads}"
    echo "max_memory_mb,${max_memory_mb}"
    echo "variants,${variants}"
} > "${manifest_file}"

{
    echo "variant,seed,status,optimal,move_count,initial_lower_bound,elapsed_ms,nodes_expanded,warmup_elapsed_ms,output_file"
} > "${summary_file}"

variant_env() {
    case "$1" in
        base)
            echo "RUBIK_DISABLE_AUTO_STRONG_OPTIMAL_ORDERING=1"
            ;;
        strong)
            echo "RUBIK_EXPERIMENTAL_STRONG_OPTIMAL_ORDERING=1"
            ;;
        *)
            echo "unknown variant: $1" >&2
            exit 2
            ;;
    esac
}

run_case() {
    local variant="$1"
    local seed="$2"
    local output_file="${output_dir}/${variant}_seed_${seed}.csv"

    echo "==> ${variant} seed ${seed}"
    echo "    output: ${output_file}"

    {
        echo "benchmark,name,auto_tail_ordering_ab_${variant}_seed_${seed}"
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
        --random-depth 15 \
        --random-seed "${seed}" \
        --slowest-count 1 \
        --diagnose-optimal \
        >> "${output_file}"
    local command_status="$?"
    set -e

    local status optimal move_count initial_lower_bound elapsed nodes warmup
    status="$(awk -F, '$1 ~ /^random_/ { print $4; exit }' "${output_file}")"
    optimal="$(awk -F, '$1 ~ /^random_/ { print $5; exit }' "${output_file}")"
    move_count="$(awk -F, '$1 ~ /^random_/ { print $6; exit }' "${output_file}")"
    initial_lower_bound="$(awk -F, '$1 ~ /^random_/ { print $7; exit }' "${output_file}")"
    elapsed="$(awk -F, '$1 ~ /^random_/ { print $8; exit }' "${output_file}")"
    nodes="$(awk -F, '$1 ~ /^random_/ { print $9; exit }' "${output_file}")"
    warmup="$(awk -F, '$1 == "benchmark" && $2 == "warmup_elapsed_ms" { print $3; exit }' "${output_file}")"

    echo "${variant},${seed},${status:-Unknown},${optimal:-false},${move_count:-0},${initial_lower_bound:-0},${elapsed:-0},${nodes:-0},${warmup:-0},${output_file}" \
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
        move_count[seed, variant] = $5
        lower_bound[seed, variant] = $6
        elapsed[seed, variant] = $7
        nodes[seed, variant] = $8
        seen[seed] = 1
    }
    END {
        print "seed,move_count,initial_lower_bound,base_elapsed_ms,strong_elapsed_ms,elapsed_delta_ms,elapsed_delta_percent,base_nodes,strong_nodes,nodes_delta,winner"
        for (seed in seen) {
            if ((seed, "base") in elapsed && (seed, "strong") in elapsed) {
                base_moves = move_count[seed, "base"] + 0
                base_lower_bound = lower_bound[seed, "base"] + 0
                base_elapsed = elapsed[seed, "base"] + 0
                strong_elapsed = elapsed[seed, "strong"] + 0
                base_nodes = nodes[seed, "base"] + 0
                strong_nodes = nodes[seed, "strong"] + 0
                delta = strong_elapsed - base_elapsed
                pct = base_elapsed == 0 ? 0 : (delta * 100.0 / base_elapsed)
                winner = delta < 0 ? "strong" : (delta > 0 ? "base" : "tie")
                printf "%s,%d,%d,%d,%d,%d,%.2f,%d,%d,%d,%s\n", seed, base_moves, base_lower_bound, base_elapsed, strong_elapsed, delta, pct, base_nodes, strong_nodes, strong_nodes - base_nodes, winner
            }
        }
    }
' "${summary_file}" | sort -t, -k1,1n > "${comparison_file}"

echo "auto tail ordering A/B manifest: ${manifest_file}"
echo "auto tail ordering A/B summary: ${summary_file}"
echo "auto tail ordering A/B comparison: ${comparison_file}"

exit "${suite_status}"
