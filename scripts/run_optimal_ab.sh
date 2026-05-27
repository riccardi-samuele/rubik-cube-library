#!/usr/bin/env bash
set -euo pipefail

build_dir="build"
cache_dir="${RUBIK_TABLE_CACHE_DIR:-/tmp/rubik_cube_library_optimal_ab_cache}"
output_dir="benchmark-results/optimal-ab"
repetitions="3"
case_set="deterministic"
max_case_depth="13"
max_depth="13"
timeout_ms="30000"
random_count="8"
random_depth="12"
random_seed="20260525"

usage() {
    cat <<'USAGE'
Usage: scripts/run_optimal_ab.sh [options]

Options:
  --build-dir DIR          CMake build directory, default: build
  --cache-dir DIR          pruning table cache directory
  --output-dir DIR         output directory, default: benchmark-results/optimal-ab
  --repetitions N          A/B repetitions, default: 3
  --case-set NAME          deterministic|random, default: deterministic
  --max-case-depth N       deterministic max case depth, default: 13
  --max-depth N            solver max depth, default: 13
  --timeout-ms N           per-case timeout, default: 30000
  --random-count N         random cases, default: 8
  --random-depth N         random scramble depth, default: 12
  --random-seed N          random seed, default: 20260525
  -h, --help               show this help
USAGE
}

require_value() {
    if [[ $# -lt 2 ]]; then
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
        --repetitions)
            require_value "$@"
            repetitions="$2"
            shift 2
            ;;
        --case-set)
            require_value "$@"
            case_set="$2"
            shift 2
            ;;
        --max-case-depth)
            require_value "$@"
            max_case_depth="$2"
            shift 2
            ;;
        --max-depth)
            require_value "$@"
            max_depth="$2"
            shift 2
            ;;
        --timeout-ms)
            require_value "$@"
            timeout_ms="$2"
            shift 2
            ;;
        --random-count)
            require_value "$@"
            random_count="$2"
            shift 2
            ;;
        --random-depth)
            require_value "$@"
            random_depth="$2"
            shift 2
            ;;
        --random-seed)
            require_value "$@"
            random_seed="$2"
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

if [[ "${case_set}" != "deterministic" && "${case_set}" != "random" ]]; then
    usage >&2
    exit 2
fi

if (( repetitions < 1 )); then
    usage >&2
    exit 2
fi

bench="${build_dir}/rubik-bench"
if [[ ! -x "${bench}" ]]; then
    echo "rubik-bench not found or not executable: ${bench}" >&2
    echo "Build first with: cmake --build ${build_dir}" >&2
    exit 1
fi

mkdir -p "${cache_dir}" "${output_dir}"
summary_file="${output_dir}/summary.csv"

{
    echo "variant,run,status,total_cases,solved,failed,total_elapsed_ms,total_nodes_expanded,max_elapsed_ms,output_file"
} > "${summary_file}"

case_args=()
case_name=""
if [[ "${case_set}" == "deterministic" ]]; then
    case_name="deterministic_depth_${max_case_depth}"
    case_args=(
        --case-set deterministic
        --max-case-depth "${max_case_depth}"
    )
else
    case_name="random_${random_count}_depth_${random_depth}_seed_${random_seed}"
    case_args=(
        --case-set random
        --random-count "${random_count}"
        --random-depth "${random_depth}"
        --random-seed "${random_seed}"
    )
fi

run_variant() {
    local variant="$1"
    local run="$2"
    local output_file="${output_dir}/${case_name}_${variant}_run_${run}.csv"

    echo "==> ${variant} run ${run}/${repetitions}"
    echo "    output: ${output_file}"

    {
        echo "benchmark,name,optimal_ab_${case_name}_${variant}_run_${run}"
        echo "benchmark,variant,${variant}"
        echo "benchmark,run,${run}"
        echo "benchmark,cache_dir,${cache_dir}"
    } > "${output_file}"

    local started_at
    started_at="$(date +%s%3N)"

    set +e
    if [[ "${variant}" == "three_phase" ]]; then
        RUBIK_TABLE_CACHE_DIR="${cache_dir}" RUBIK_EXPERIMENTAL_THREE_PHASE1_BOUNDS=1 \
            "${bench}" \
            --mode optimal \
            --timeout-ms "${timeout_ms}" \
            --max-depth "${max_depth}" \
            --slowest-count 5 \
            "${case_args[@]}" \
            >> "${output_file}"
    else
        RUBIK_TABLE_CACHE_DIR="${cache_dir}" RUBIK_DISABLE_THREE_PHASE1_BOUNDS=1 \
            "${bench}" \
            --mode optimal \
            --timeout-ms "${timeout_ms}" \
            --max-depth "${max_depth}" \
            --slowest-count 5 \
            "${case_args[@]}" \
            >> "${output_file}"
    fi
    local status="$?"
    set -e

    local ended_at
    ended_at="$(date +%s%3N)"
    echo "benchmark,wall_elapsed_ms,$((ended_at - started_at))" >> "${output_file}"

    local total_cases solved failed total_elapsed total_nodes max_elapsed
    total_cases="$(awk -F, '$1 == "summary" && $2 == "total_cases" { print $3 }' "${output_file}")"
    solved="$(awk -F, '$1 == "summary" && $2 == "solved" { print $3 }' "${output_file}")"
    failed="$(awk -F, '$1 == "summary" && $2 == "failed" { print $3 }' "${output_file}")"
    total_elapsed="$(awk -F, '$1 == "summary" && $2 == "total_elapsed_ms" { print $3 }' "${output_file}")"
    total_nodes="$(awk -F, '$1 == "summary" && $2 == "total_nodes_expanded" { print $3 }' "${output_file}")"
    max_elapsed="$(awk -F, '$1 == "summary" && $2 == "max_elapsed_ms" { print $3 }' "${output_file}")"

    if [[ -z "${total_cases}" ]]; then
        total_cases="0"
        solved="0"
        failed="1"
        total_elapsed="0"
        total_nodes="0"
        max_elapsed="0"
    fi

    echo "${variant},${run},${status},${total_cases},${solved},${failed},${total_elapsed},${total_nodes},${max_elapsed},${output_file}" \
        >> "${summary_file}"

    return 0
}

for run in $(seq 1 "${repetitions}"); do
    run_variant baseline "${run}"
    run_variant three_phase "${run}"
done

awk -F, '
    NR == 1 { next }
    {
        count[$1] += 1
        elapsed[$1] += $7
        nodes[$1] += $8
        failed[$1] += $6
    }
    END {
        print "variant,runs,avg_total_elapsed_ms,avg_total_nodes_expanded,total_failed"
        for (variant in count) {
            printf "%s,%d,%.2f,%.2f,%d\n",
                variant,
                count[variant],
                elapsed[variant] / count[variant],
                nodes[variant] / count[variant],
                failed[variant]
        }
    }
' "${summary_file}" | tee "${output_dir}/aggregate.csv"

echo "summary: ${summary_file}"
