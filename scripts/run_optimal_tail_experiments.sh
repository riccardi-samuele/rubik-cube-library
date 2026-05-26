#!/usr/bin/env bash
set -euo pipefail

build_dir="out/release-native-lto"
cache_dir="${RUBIK_TABLE_CACHE_DIR:-/tmp/rubik_cube_library_optimal_tail_experiments_cache}"
output_dir="benchmark-results/optimal-tail-experiments"
profile="embedded"
timeout_ms="30000"
max_depth="13"
threads="1"
max_memory_mb="1024"
variants="baseline,no_corner_state"

usage() {
    cat <<'USAGE'
Usage: scripts/run_optimal_tail_experiments.sh [options]

Options:
  --build-dir DIR       CMake build directory, default: out/release-native-lto
  --cache-dir DIR       pruning table cache directory
  --output-dir DIR      output directory, default: benchmark-results/optimal-tail-experiments
  --profile NAME        embedded|default|performance, default: embedded
  --timeout-ms N        per-case timeout, default: 30000
  --max-depth N         solver max depth, default: 13
  --threads N           solver threads, default: 1
  --max-memory-mb N     solver memory limit, default: 1024
  --variants LIST       comma-separated variants, default: baseline,no_corner_state

Variants:
  baseline              current default optimal engine
  no_corner_state       current engine with RUBIK_DISABLE_CORNER_STATE_BOUNDS=1
  corner_state          RUBIK_EXPERIMENTAL_CORNER_STATE_BOUNDS=1
  corner_state_up       corner_state + RUBIK_EXPERIMENTAL_CORNER_UP_EDGE_BOUNDS=1
  corner_state_down     corner_state + RUBIK_EXPERIMENTAL_CORNER_DOWN_EDGE_BOUNDS=1
  corner_state_both     corner_state + both corner/edge-group tables
  phase2_ordering       RUBIK_EXPERIMENTAL_PHASE2_OPTIMAL_ORDERING=1
  strong_ordering       RUBIK_EXPERIMENTAL_STRONG_OPTIMAL_ORDERING=1
  goal_depth6           RUBIK_EXPERIMENTAL_OPTIMAL_GOAL_TABLE_DEPTH=6

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
        --profile)
            profile="$2"
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

if [[ "${profile}" != "embedded" && "${profile}" != "default" && "${profile}" != "performance" ]]; then
    usage >&2
    exit 2
fi

for numeric in "${timeout_ms}" "${max_depth}" "${threads}" "${max_memory_mb}"; do
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

mkdir -p "${cache_dir}" "${output_dir}"

IFS=',' read -r -a variant_list <<< "${variants}"
for variant in "${variant_list[@]}"; do
    if [[ -z "${variant}" ]]; then
        usage >&2
        exit 2
    fi
done

tail_cases=(
    "12345:4"
    "42:2"
    "42:1"
    "20260525:7"
    "12345:2"
)

summary_file="${output_dir}/summary.csv"
aggregate_file="${output_dir}/aggregate.csv"
manifest_file="${output_dir}/manifest.csv"

{
    echo "key,value"
    echo "git_revision,$(git rev-parse --short HEAD 2>/dev/null || echo unknown)"
    echo "build_dir,${build_dir}"
    echo "cache_dir,${cache_dir}"
    echo "profile,${profile}"
    echo "timeout_ms,${timeout_ms}"
    echo "max_depth,${max_depth}"
    echo "threads,${threads}"
    echo "max_memory_mb,${max_memory_mb}"
    echo "variants,${variants}"
} > "${manifest_file}"

{
    echo "variant,case_name,seed,index,status,solved,elapsed_ms,nodes_expanded,move_count,initial_lower_bound,warmup_elapsed_ms,table_payload_bytes,output_file"
} > "${summary_file}"

variant_env() {
    case "$1" in
        baseline)
            ;;
        no_corner_state)
            echo "RUBIK_DISABLE_CORNER_STATE_BOUNDS=1"
            ;;
        corner_state)
            echo "RUBIK_EXPERIMENTAL_CORNER_STATE_BOUNDS=1"
            ;;
        corner_state_up)
            echo "RUBIK_EXPERIMENTAL_CORNER_STATE_BOUNDS=1"
            echo "RUBIK_EXPERIMENTAL_CORNER_UP_EDGE_BOUNDS=1"
            ;;
        corner_state_down)
            echo "RUBIK_EXPERIMENTAL_CORNER_STATE_BOUNDS=1"
            echo "RUBIK_EXPERIMENTAL_CORNER_DOWN_EDGE_BOUNDS=1"
            ;;
        corner_state_both)
            echo "RUBIK_EXPERIMENTAL_CORNER_STATE_BOUNDS=1"
            echo "RUBIK_EXPERIMENTAL_CORNER_UP_EDGE_BOUNDS=1"
            echo "RUBIK_EXPERIMENTAL_CORNER_DOWN_EDGE_BOUNDS=1"
            ;;
        phase2_ordering)
            echo "RUBIK_EXPERIMENTAL_PHASE2_OPTIMAL_ORDERING=1"
            ;;
        strong_ordering)
            echo "RUBIK_EXPERIMENTAL_STRONG_OPTIMAL_ORDERING=1"
            ;;
        goal_depth6)
            echo "RUBIK_EXPERIMENTAL_OPTIMAL_GOAL_TABLE_DEPTH=6"
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
    local index="$3"
    local case_name="random_${seed}_${index}"
    local output_file="${output_dir}/${variant}_${case_name}.csv"

    echo "==> ${variant} ${case_name}"
    echo "    output: ${output_file}"

    {
        echo "benchmark,name,optimal_tail_experiment_${variant}_${case_name}"
        echo "benchmark,variant,${variant}"
        echo "benchmark,cache_dir,${cache_dir}"
    } > "${output_file}"

    mapfile -t env_vars < <(variant_env "${variant}")

    local started_at
    started_at="$(date +%s%3N)"

    set +e
    env RUBIK_TABLE_CACHE_DIR="${cache_dir}" "${env_vars[@]}" \
        "${bench}" \
        --mode optimal \
        --profile "${profile}" \
        --timeout-ms "${timeout_ms}" \
        --max-depth "${max_depth}" \
        --threads "${threads}" \
        --max-memory-mb "${max_memory_mb}" \
        --case-set random \
        --random-count 1 \
        --random-depth 13 \
        --random-seed "${seed}" \
        --random-start-index "${index}" \
        --slowest-count 1 \
        --diagnose-optimal \
        >> "${output_file}"
    local command_status="$?"
    set -e

    local ended_at
    ended_at="$(date +%s%3N)"
    echo "benchmark,wall_elapsed_ms,$((ended_at - started_at))" >> "${output_file}"

    local status solved elapsed nodes moves initial_lower_bound warmup_elapsed table_payload
    status="$(awk -F, '$1 ~ /^random_/ { print $4; exit }' "${output_file}")"
    elapsed="$(awk -F, '$1 ~ /^random_/ { print $8; exit }' "${output_file}")"
    nodes="$(awk -F, '$1 ~ /^random_/ { print $9; exit }' "${output_file}")"
    moves="$(awk -F, '$1 ~ /^random_/ { print $6; exit }' "${output_file}")"
    initial_lower_bound="$(awk -F, '$1 ~ /^random_/ { print $7; exit }' "${output_file}")"
    warmup_elapsed="$(awk -F, '$1 == "benchmark" && $2 == "warmup_elapsed_ms" { print $3; exit }' "${output_file}")"
    table_payload="$(awk -F, '$1 == "benchmark" && $2 == "warmup_table_payload_bytes" { print $3; exit }' "${output_file}")"

    if [[ "${status}" == "Optimal" || "${status}" == "Solved" || "${status}" == "Found" ]]; then
        solved="1"
    else
        solved="0"
    fi

    echo "${variant},${case_name},${seed},${index},${status:-Unknown},${solved},${elapsed:-0},${nodes:-0},${moves:-0},${initial_lower_bound:-0},${warmup_elapsed:-0},${table_payload:-0},${output_file}" \
        >> "${summary_file}"

    return "${command_status}"
}

suite_status=0
for variant in "${variant_list[@]}"; do
    for entry in "${tail_cases[@]}"; do
        seed="${entry%%:*}"
        index="${entry##*:}"
        run_case "${variant}" "${seed}" "${index}" || suite_status=1
    done
done

aggregate_rows="$(mktemp)"
trap 'rm -f "${aggregate_rows}"' EXIT

awk -F, '
    NR == 1 { next }
    {
        count[$1] += 1
        solved[$1] += $6
        elapsed[$1] += $7
        nodes[$1] += $8
        if ($7 > maxElapsed[$1]) {
            maxElapsed[$1] = $7
        }
        if ($8 > maxNodes[$1]) {
            maxNodes[$1] = $8
        }
        payload[$1] = $12
    }
    END {
        print "variant,cases,solved,avg_elapsed_ms,max_elapsed_ms,avg_nodes_expanded,max_nodes_expanded,table_payload_bytes"
        for (variant in count) {
            printf "%s,%d,%d,%.2f,%d,%.2f,%d,%d\n",
                variant,
                count[variant],
                solved[variant],
                elapsed[variant] / count[variant],
                maxElapsed[variant],
                nodes[variant] / count[variant],
                maxNodes[variant],
                payload[variant]
        }
    }
' "${summary_file}" > "${aggregate_rows}"

{
    head -n 1 "${aggregate_rows}"
    tail -n +2 "${aggregate_rows}" | sort
} > "${aggregate_file}"

echo "tail experiment manifest: ${manifest_file}"
echo "tail experiment summary: ${summary_file}"
echo "tail experiment aggregate: ${aggregate_file}"
exit "${suite_status}"
