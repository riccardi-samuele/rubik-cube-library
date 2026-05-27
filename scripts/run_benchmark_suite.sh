#!/usr/bin/env bash
set -euo pipefail

suite="smoke"
build_dir="build"
cache_dir="${RUBIK_TABLE_CACHE_DIR:-/tmp/rubik_cube_library_benchmark_cache}"
output_dir="benchmark-results"
seed="12345"
seeds="12345,20260525,42"
fast_timeout_ms="5000"
fast_max_depth="24"
optimal_timeout_ms="30000"
cache_mode="warm"
profile="default"
realistic_fast_count="100"
realistic_optimal_depth12_count="50"
realistic_optimal_depth13_count="20"
deep_optimal_depth14_count="2"
deep_optimal_depth15_count="1"
benchmark_threads="1"
benchmark_max_memory_mb="1024"

usage() {
    cat <<'USAGE'
Usage: scripts/run_benchmark_suite.sh [options]

Options:
  --suite NAME             smoke|profile-smoke|profile-realistic|embedded-multiseed|optimal-stress|optimal-tail-cases|optimal-deep-probe|optimal-large-local|optimal-auto-tail|embedded-fast-tail-cases|embedded-fast-failures|fast-100|fast-1000|optimal-depth|tail-diagnostics|all
  --build-dir DIR          CMake build directory, default: build
  --cache-dir DIR          pruning table cache directory
  --cache-mode MODE        warm|cold, default: warm
  --output-dir DIR         benchmark output directory, default: benchmark-results
  --profile NAME           embedded|default|performance|large-local|auto, default: default
  --seed N                 random benchmark seed, default: 12345
  --seeds LIST             comma-separated seeds for embedded-multiseed, default: 12345,20260525,42
  --fast-timeout-ms N      fast-mode per-case timeout, default: 5000
  --fast-max-depth N       fast-mode maximum solution depth, default: 24
  --optimal-timeout-ms N   optimal-mode per-case timeout, default: 30000
  --realistic-fast-count N random depth-20 fast cases for profile-realistic, default: 100
  --realistic-opt12-count N random depth-12 optimal cases for profile-realistic, default: 50
  --realistic-opt13-count N random depth-13 optimal cases for profile-realistic, default: 20
  --deep-opt14-count N     random depth-14 optimal cases for optimal-deep-probe, default: 2
  --deep-opt15-count N     random depth-15 optimal cases for optimal-deep-probe, default: 1
  --threads N              rubik-bench solver threads, default: 1
  --max-memory-mb N        rubik-bench solver memory limit, default: 1024
  -h, --help               show this help
USAGE
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --suite)
            suite="$2"
            shift 2
            ;;
        --build-dir)
            build_dir="$2"
            shift 2
            ;;
        --cache-dir)
            cache_dir="$2"
            shift 2
            ;;
        --cache-mode)
            cache_mode="$2"
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
        --seed)
            seed="$2"
            shift 2
            ;;
        --seeds)
            seeds="$2"
            shift 2
            ;;
        --fast-timeout-ms)
            fast_timeout_ms="$2"
            shift 2
            ;;
        --fast-max-depth)
            fast_max_depth="$2"
            shift 2
            ;;
        --optimal-timeout-ms)
            optimal_timeout_ms="$2"
            shift 2
            ;;
        --realistic-fast-count)
            realistic_fast_count="$2"
            shift 2
            ;;
        --realistic-opt12-count)
            realistic_optimal_depth12_count="$2"
            shift 2
            ;;
        --realistic-opt13-count)
            realistic_optimal_depth13_count="$2"
            shift 2
            ;;
        --deep-opt14-count)
            deep_optimal_depth14_count="$2"
            shift 2
            ;;
        --deep-opt15-count)
            deep_optimal_depth15_count="$2"
            shift 2
            ;;
        --threads)
            benchmark_threads="$2"
            shift 2
            ;;
        --max-memory-mb)
            benchmark_max_memory_mb="$2"
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

if [[ "${cache_mode}" != "warm" && "${cache_mode}" != "cold" ]]; then
    usage >&2
    exit 2
fi

if [[ "${profile}" != "embedded" && "${profile}" != "default" && "${profile}" != "performance" && "${profile}" != "large-local" && "${profile}" != "auto" ]]; then
    usage >&2
    exit 2
fi

if (( realistic_fast_count < 0 || realistic_optimal_depth12_count < 1 || realistic_optimal_depth13_count < 1 || deep_optimal_depth14_count < 1 || deep_optimal_depth15_count < 1 )); then
    usage >&2
    exit 2
fi

if (( fast_max_depth < 1 || benchmark_threads < 0 || benchmark_max_memory_mb < 1 )); then
    usage >&2
    exit 2
fi

IFS=',' read -r -a seed_list <<< "${seeds}"
for seed_value in "${seed_list[@]}"; do
    if [[ -z "${seed_value}" ]]; then
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

run_benchmark() {
    local name="$1"
    shift

    if [[ "${cache_mode}" == "cold" ]]; then
        rm -rf "${cache_dir}"
        mkdir -p "${cache_dir}"
    fi

    local output_file="${output_dir}/${cache_mode}_${name}.csv"
    echo "==> ${name}"
    echo "    cache_mode: ${cache_mode}"
    echo "    cache_dir: ${cache_dir}"
    echo "    output: ${output_file}"

    {
        echo "benchmark,name,${name}"
        echo "benchmark,cache_mode,${cache_mode}"
        echo "benchmark,cache_dir,${cache_dir}"
        echo "benchmark,suite_profile,${profile}"
    } | tee "${output_file}"

    local started_at
    started_at="$(date +%s%3N)"

    set +e
    RUBIK_TABLE_CACHE_DIR="${cache_dir}" "${bench}" "$@" | tee -a "${output_file}"
    local status="${PIPESTATUS[0]}"
    set -e

    local ended_at
    ended_at="$(date +%s%3N)"
    echo "benchmark,wall_elapsed_ms,$((ended_at - started_at))" | tee -a "${output_file}"

    return "${status}"
}

run_smoke() {
    run_benchmark "smoke_fast_random_5_seed_${seed}" \
        --mode fast \
        --profile "${profile}" \
        --timeout-ms "${fast_timeout_ms}" \
        --max-depth "${fast_max_depth}" \
        --case-set random \
        --random-count 5 \
        --random-depth 20 \
        --random-seed "${seed}" \
        --slowest-count 5

    run_benchmark "smoke_optimal_depth_7" \
        --mode optimal \
        --profile "${profile}" \
        --timeout-ms "${optimal_timeout_ms}" \
        --max-depth 7 \
        --max-case-depth 7 \
        --slowest-count 5
}

run_fast_100() {
    run_benchmark "fast_random_100_depth_20_seed_${seed}" \
        --mode fast \
        --profile "${profile}" \
        --timeout-ms "${fast_timeout_ms}" \
        --max-depth "${fast_max_depth}" \
        --case-set random \
        --random-count 100 \
        --random-depth 20 \
        --random-seed "${seed}" \
        --slowest-count 10
}

run_fast_1000() {
    run_benchmark "fast_random_1000_depth_20_seed_${seed}" \
        --mode fast \
        --profile "${profile}" \
        --timeout-ms "${fast_timeout_ms}" \
        --max-depth "${fast_max_depth}" \
        --case-set random \
        --random-count 1000 \
        --random-depth 20 \
        --random-seed "${seed}" \
        --slowest-count 25
}

run_optimal_depth() {
    run_benchmark "optimal_deterministic_depth_13" \
        --mode optimal \
        --profile "${profile}" \
        --timeout-ms "${optimal_timeout_ms}" \
        --max-depth 13 \
        --max-case-depth 13 \
        --slowest-count 10
}

run_tail_diagnostics() {
    for index in 5 50 33 92 47 75 10 34 13 45; do
        run_benchmark "fast_diagnostic_random_${seed}_${index}" \
            --mode fast \
            --profile "${profile}" \
            --timeout-ms "${fast_timeout_ms}" \
            --max-depth "${fast_max_depth}" \
            --case-set random \
            --random-count 1 \
            --random-depth 20 \
            --random-seed "${seed}" \
            --random-start-index "${index}" \
            --slowest-count 1 \
            --diagnose-fast
    done
}

run_embedded_fast_failures() {
    local previous_profile="${profile}"
    profile="embedded"

    for index in 5 13; do
        run_benchmark "embedded_fast_failure_random_${seed}_${index}" \
            --mode fast \
            --profile embedded \
            --timeout-ms "${fast_timeout_ms}" \
            --max-depth "${fast_max_depth}" \
            --case-set random \
            --random-count 1 \
            --random-depth 20 \
            --random-seed "${seed}" \
            --random-start-index "${index}" \
            --slowest-count 1 \
            --diagnose-fast || true
    done

    profile="${previous_profile}"
}

run_embedded_fast_tail_cases() {
    local previous_profile="${profile}"
    profile="embedded"

    local tail_cases=(
        "12345:5"
        "12345:13"
        "20260525:58"
        "20260525:61"
        "42:99"
    )

    for entry in "${tail_cases[@]}"; do
        local current_seed="${entry%%:*}"
        local index="${entry##*:}"
        run_benchmark "embedded_fast_tail_random_${current_seed}_${index}" \
            --mode fast \
            --profile embedded \
            --timeout-ms "${fast_timeout_ms}" \
            --max-depth "${fast_max_depth}" \
            --case-set random \
            --random-count 1 \
            --random-depth 20 \
            --random-seed "${current_seed}" \
            --random-start-index "${index}" \
            --slowest-count 1 \
            --diagnose-fast || true
    done

    profile="${previous_profile}"
}

append_profile_summary_row() {
    local summary_file="$1"
    local profile_name="$2"
    local mode_name="$3"
    local benchmark_name="$4"
    local output_file="$5"

    local total_cases solved failed total_elapsed total_nodes p50_elapsed p90_elapsed p95_elapsed p99_elapsed max_elapsed warmup_elapsed wall_elapsed
    total_cases="$(awk -F, '$1 == "summary" && $2 == "total_cases" { print $3 }' "${output_file}")"
    solved="$(awk -F, '$1 == "summary" && $2 == "solved" { print $3 }' "${output_file}")"
    failed="$(awk -F, '$1 == "summary" && $2 == "failed" { print $3 }' "${output_file}")"
    total_elapsed="$(awk -F, '$1 == "summary" && $2 == "total_elapsed_ms" { print $3 }' "${output_file}")"
    total_nodes="$(awk -F, '$1 == "summary" && $2 == "total_nodes_expanded" { print $3 }' "${output_file}")"
    p50_elapsed="$(awk -F, '$1 == "summary" && $2 == "p50_elapsed_ms" { print $3 }' "${output_file}")"
    p90_elapsed="$(awk -F, '$1 == "summary" && $2 == "p90_elapsed_ms" { print $3 }' "${output_file}")"
    p95_elapsed="$(awk -F, '$1 == "summary" && $2 == "p95_elapsed_ms" { print $3 }' "${output_file}")"
    p99_elapsed="$(awk -F, '$1 == "summary" && $2 == "p99_elapsed_ms" { print $3 }' "${output_file}")"
    max_elapsed="$(awk -F, '$1 == "summary" && $2 == "max_elapsed_ms" { print $3 }' "${output_file}")"
    warmup_elapsed="$(awk -F, '$1 == "benchmark" && $2 == "warmup_elapsed_ms" { print $3 }' "${output_file}")"
    wall_elapsed="$(awk -F, '$1 == "benchmark" && $2 == "wall_elapsed_ms" { print $3 }' "${output_file}")"

    if [[ -z "${total_cases}" ]]; then
        total_cases="0"
        solved="0"
        failed="1"
        total_elapsed="0"
        total_nodes="0"
        p50_elapsed="0"
        p90_elapsed="0"
        p95_elapsed="0"
        p99_elapsed="0"
        max_elapsed="0"
        warmup_elapsed="0"
        wall_elapsed="0"
    fi

    echo "${profile_name},${mode_name},${benchmark_name},${total_cases},${solved},${failed},${total_elapsed},${total_nodes},${p50_elapsed},${p90_elapsed},${p95_elapsed},${p99_elapsed},${max_elapsed},${warmup_elapsed},${wall_elapsed},${output_file}" \
        >> "${summary_file}"
}

run_profile_smoke() {
    local previous_profile="${profile}"
    local summary_file="${output_dir}/${cache_mode}_profile_smoke_summary.csv"

    {
        echo "profile,mode,benchmark,total_cases,solved,failed,total_elapsed_ms,total_nodes_expanded,p50_elapsed_ms,p90_elapsed_ms,p95_elapsed_ms,p99_elapsed_ms,max_elapsed_ms,warmup_elapsed_ms,wall_elapsed_ms,output_file"
    } > "${summary_file}"

    for profile_name in embedded default performance; do
        profile="${profile_name}"
        local fast_name="profile_${profile_name}_fast_random_5_seed_${seed}"
        local optimal_name="profile_${profile_name}_optimal_depth_7"
        local fast_output="${output_dir}/${cache_mode}_${fast_name}.csv"
        local optimal_output="${output_dir}/${cache_mode}_${optimal_name}.csv"

        run_benchmark "${fast_name}" \
            --mode fast \
            --profile "${profile}" \
            --timeout-ms "${fast_timeout_ms}" \
            --max-depth "${fast_max_depth}" \
            --case-set random \
            --random-count 5 \
            --random-depth 8 \
            --random-seed "${seed}" \
            --slowest-count 5
        append_profile_summary_row "${summary_file}" "${profile_name}" "fast" "random_depth_8_count_5" "${fast_output}"

        run_benchmark "${optimal_name}" \
            --mode optimal \
            --profile "${profile}" \
            --timeout-ms "${optimal_timeout_ms}" \
            --max-depth 7 \
            --max-case-depth 7 \
            --slowest-count 5
        append_profile_summary_row "${summary_file}" "${profile_name}" "optimal" "deterministic_depth_7" "${optimal_output}"
    done
    profile="${previous_profile}"

    echo "profile summary: ${summary_file}"
}

run_profile_realistic() {
    local previous_profile="${profile}"
    local summary_file="${output_dir}/${cache_mode}_profile_realistic_summary.csv"
    local suite_status=0

    {
        echo "profile,mode,benchmark,total_cases,solved,failed,total_elapsed_ms,total_nodes_expanded,p50_elapsed_ms,p90_elapsed_ms,p95_elapsed_ms,p99_elapsed_ms,max_elapsed_ms,warmup_elapsed_ms,wall_elapsed_ms,output_file"
    } > "${summary_file}"

    local profile_names=(embedded default performance)
    if [[ "${previous_profile}" == "auto" ]]; then
        profile_names=(auto)
    fi

    for profile_name in "${profile_names[@]}"; do
        profile="${profile_name}"

        local fast_name="profile_${profile_name}_fast_random_${realistic_fast_count}_depth_20_seed_${seed}"
        local opt12_name="profile_${profile_name}_optimal_random_${realistic_optimal_depth12_count}_depth_12_seed_${seed}"
        local opt13_name="profile_${profile_name}_optimal_random_${realistic_optimal_depth13_count}_depth_13_seed_${seed}"
        local fast_output="${output_dir}/${cache_mode}_${fast_name}.csv"
        local opt12_output="${output_dir}/${cache_mode}_${opt12_name}.csv"
        local opt13_output="${output_dir}/${cache_mode}_${opt13_name}.csv"

        if (( realistic_fast_count > 0 )); then
            run_benchmark "${fast_name}" \
                --mode fast \
                --profile "${profile}" \
                --threads "${benchmark_threads}" \
                --max-memory-mb "${benchmark_max_memory_mb}" \
                --timeout-ms "${fast_timeout_ms}" \
                --max-depth "${fast_max_depth}" \
                --case-set random \
                --random-count "${realistic_fast_count}" \
                --random-depth 20 \
                --random-seed "${seed}" \
                --slowest-count 10 || suite_status=1
            append_profile_summary_row "${summary_file}" "${profile_name}" "fast" "random_depth_20_count_${realistic_fast_count}" "${fast_output}"
        fi

        run_benchmark "${opt12_name}" \
            --mode optimal \
            --profile "${profile}" \
            --threads "${benchmark_threads}" \
            --max-memory-mb "${benchmark_max_memory_mb}" \
            --timeout-ms "${optimal_timeout_ms}" \
            --max-depth 12 \
            --case-set random \
            --random-count "${realistic_optimal_depth12_count}" \
            --random-depth 12 \
            --random-seed "${seed}" \
            --slowest-count 10 || suite_status=1
        append_profile_summary_row "${summary_file}" "${profile_name}" "optimal" "random_depth_12_count_${realistic_optimal_depth12_count}" "${opt12_output}"

        run_benchmark "${opt13_name}" \
            --mode optimal \
            --profile "${profile}" \
            --threads "${benchmark_threads}" \
            --max-memory-mb "${benchmark_max_memory_mb}" \
            --timeout-ms "${optimal_timeout_ms}" \
            --max-depth 13 \
            --case-set random \
            --random-count "${realistic_optimal_depth13_count}" \
            --random-depth 13 \
            --random-seed "${seed}" \
            --slowest-count 10 || suite_status=1
        append_profile_summary_row "${summary_file}" "${profile_name}" "optimal" "random_depth_13_count_${realistic_optimal_depth13_count}" "${opt13_output}"
    done
    profile="${previous_profile}"

    echo "profile realistic summary: ${summary_file}"
    return "${suite_status}"
}

run_embedded_multiseed() {
    local previous_profile="${profile}"
    local summary_file="${output_dir}/${cache_mode}_embedded_multiseed_summary.csv"
    local suite_status=0

    profile="embedded"

    {
        echo "profile,mode,benchmark,total_cases,solved,failed,total_elapsed_ms,total_nodes_expanded,p50_elapsed_ms,p90_elapsed_ms,p95_elapsed_ms,p99_elapsed_ms,max_elapsed_ms,warmup_elapsed_ms,wall_elapsed_ms,output_file"
    } > "${summary_file}"

    for current_seed in "${seed_list[@]}"; do
        local fast_name="embedded_multiseed_fast_random_${realistic_fast_count}_depth_20_seed_${current_seed}"
        local opt13_name="embedded_multiseed_optimal_random_${realistic_optimal_depth13_count}_depth_13_seed_${current_seed}"
        local fast_output="${output_dir}/${cache_mode}_${fast_name}.csv"
        local opt13_output="${output_dir}/${cache_mode}_${opt13_name}.csv"

        run_benchmark "${fast_name}" \
            --mode fast \
            --profile embedded \
            --timeout-ms "${fast_timeout_ms}" \
            --max-depth "${fast_max_depth}" \
            --case-set random \
            --random-count "${realistic_fast_count}" \
            --random-depth 20 \
            --random-seed "${current_seed}" \
            --slowest-count 10 || suite_status=1
        append_profile_summary_row "${summary_file}" "embedded" "fast" "random_seed_${current_seed}_depth_20_count_${realistic_fast_count}" "${fast_output}"

        run_benchmark "${opt13_name}" \
            --mode optimal \
            --profile embedded \
            --timeout-ms "${optimal_timeout_ms}" \
            --max-depth 13 \
            --case-set random \
            --random-count "${realistic_optimal_depth13_count}" \
            --random-depth 13 \
            --random-seed "${current_seed}" \
            --slowest-count 10 || suite_status=1
        append_profile_summary_row "${summary_file}" "embedded" "optimal" "random_seed_${current_seed}_depth_13_count_${realistic_optimal_depth13_count}" "${opt13_output}"
    done

    profile="${previous_profile}"

    echo "embedded multiseed summary: ${summary_file}"
    return "${suite_status}"
}

run_optimal_stress() {
    local previous_profile="${profile}"
    local summary_file="${output_dir}/${cache_mode}_optimal_stress_summary.csv"
    local suite_status=0

    {
        echo "profile,mode,benchmark,total_cases,solved,failed,total_elapsed_ms,total_nodes_expanded,p50_elapsed_ms,p90_elapsed_ms,p95_elapsed_ms,p99_elapsed_ms,max_elapsed_ms,warmup_elapsed_ms,wall_elapsed_ms,output_file"
    } > "${summary_file}"

    for profile_name in embedded default performance; do
        profile="${profile_name}"

        for current_seed in "${seed_list[@]}"; do
            local opt13_name="optimal_stress_${profile_name}_random_${realistic_optimal_depth13_count}_depth_13_seed_${current_seed}"
            local opt13_output="${output_dir}/${cache_mode}_${opt13_name}.csv"

            run_benchmark "${opt13_name}" \
                --mode optimal \
                --profile "${profile}" \
                --timeout-ms "${optimal_timeout_ms}" \
                --max-depth 13 \
                --case-set random \
                --random-count "${realistic_optimal_depth13_count}" \
                --random-depth 13 \
                --random-seed "${current_seed}" \
                --slowest-count 10 || suite_status=1
            append_profile_summary_row "${summary_file}" "${profile_name}" "optimal" "random_seed_${current_seed}_depth_13_count_${realistic_optimal_depth13_count}" "${opt13_output}"
        done
    done

    profile="${previous_profile}"

    echo "optimal stress summary: ${summary_file}"
    return "${suite_status}"
}

run_optimal_tail_cases() {
    local previous_profile="${profile}"
    local summary_file="${output_dir}/${cache_mode}_optimal_tail_cases_summary.csv"
    local suite_status=0

    local tail_cases=(
        "12345:4"
        "42:2"
        "42:1"
        "20260525:7"
        "12345:2"
    )

    profile="embedded"

    {
        echo "profile,mode,benchmark,total_cases,solved,failed,total_elapsed_ms,total_nodes_expanded,p50_elapsed_ms,p90_elapsed_ms,p95_elapsed_ms,p99_elapsed_ms,max_elapsed_ms,warmup_elapsed_ms,wall_elapsed_ms,output_file"
    } > "${summary_file}"

    for entry in "${tail_cases[@]}"; do
        local current_seed="${entry%%:*}"
        local index="${entry##*:}"
        local opt_name="optimal_tail_embedded_random_${current_seed}_${index}_depth_13"
        local opt_output="${output_dir}/${cache_mode}_${opt_name}.csv"

        run_benchmark "${opt_name}" \
            --mode optimal \
            --profile embedded \
            --timeout-ms "${optimal_timeout_ms}" \
            --max-depth 13 \
            --case-set random \
            --random-count 1 \
            --random-depth 13 \
            --random-seed "${current_seed}" \
            --random-start-index "${index}" \
            --slowest-count 1 \
            --diagnose-optimal || suite_status=1
        append_profile_summary_row "${summary_file}" "embedded" "optimal" "random_seed_${current_seed}_index_${index}_depth_13" "${opt_output}"
    done

    profile="${previous_profile}"

    echo "optimal tail-cases summary: ${summary_file}"
    return "${suite_status}"
}

run_optimal_deep_probe() {
    local previous_profile="${profile}"
    local summary_file="${output_dir}/${cache_mode}_optimal_deep_probe_summary.csv"

    {
        echo "profile,mode,benchmark,total_cases,solved,failed,total_elapsed_ms,total_nodes_expanded,p50_elapsed_ms,p90_elapsed_ms,p95_elapsed_ms,p99_elapsed_ms,max_elapsed_ms,warmup_elapsed_ms,wall_elapsed_ms,output_file"
    } > "${summary_file}"

    for profile_name in embedded default performance; do
        profile="${profile_name}"

        for current_seed in "${seed_list[@]}"; do
            local opt14_name="optimal_deep_probe_${profile_name}_random_${deep_optimal_depth14_count}_depth_14_seed_${current_seed}"
            local opt15_name="optimal_deep_probe_${profile_name}_random_${deep_optimal_depth15_count}_depth_15_seed_${current_seed}"
            local opt14_output="${output_dir}/${cache_mode}_${opt14_name}.csv"
            local opt15_output="${output_dir}/${cache_mode}_${opt15_name}.csv"

            run_benchmark "${opt14_name}" \
                --mode optimal \
                --profile "${profile}" \
                --threads "${benchmark_threads}" \
                --max-memory-mb "${benchmark_max_memory_mb}" \
                --timeout-ms "${optimal_timeout_ms}" \
                --max-depth 14 \
                --case-set random \
                --random-count "${deep_optimal_depth14_count}" \
                --random-depth 14 \
                --random-seed "${current_seed}" \
                --slowest-count "${deep_optimal_depth14_count}" \
                --diagnose-optimal || true
            append_profile_summary_row "${summary_file}" "${profile_name}" "optimal" "random_seed_${current_seed}_depth_14_count_${deep_optimal_depth14_count}" "${opt14_output}"

            run_benchmark "${opt15_name}" \
                --mode optimal \
                --profile "${profile}" \
                --threads "${benchmark_threads}" \
                --max-memory-mb "${benchmark_max_memory_mb}" \
                --timeout-ms "${optimal_timeout_ms}" \
                --max-depth 15 \
                --case-set random \
                --random-count "${deep_optimal_depth15_count}" \
                --random-depth 15 \
                --random-seed "${current_seed}" \
                --slowest-count "${deep_optimal_depth15_count}" \
                --diagnose-optimal || true
            append_profile_summary_row "${summary_file}" "${profile_name}" "optimal" "random_seed_${current_seed}_depth_15_count_${deep_optimal_depth15_count}" "${opt15_output}"
        done
    done

    profile="${previous_profile}"

    echo "optimal deep probe summary: ${summary_file}"
}

run_optimal_large_local() {
    local previous_profile="${profile}"
    local summary_file="${output_dir}/${cache_mode}_optimal_large_local_summary.csv"
    profile="large-local"

    {
        echo "profile,mode,benchmark,total_cases,solved,failed,total_elapsed_ms,total_nodes_expanded,p50_elapsed_ms,p90_elapsed_ms,p95_elapsed_ms,p99_elapsed_ms,max_elapsed_ms,warmup_elapsed_ms,wall_elapsed_ms,output_file"
    } > "${summary_file}"

    for current_seed in "${seed_list[@]}"; do
        local opt15_name="optimal_large_local_large_local_random_${deep_optimal_depth15_count}_depth_15_seed_${current_seed}"
        local opt15_output="${output_dir}/${cache_mode}_${opt15_name}.csv"

        run_benchmark "${opt15_name}" \
            --mode optimal \
            --profile large-local \
            --threads "${benchmark_threads}" \
            --max-memory-mb "${benchmark_max_memory_mb}" \
            --timeout-ms "${optimal_timeout_ms}" \
            --max-depth 15 \
            --case-set random \
            --random-count "${deep_optimal_depth15_count}" \
            --random-depth 15 \
            --random-seed "${current_seed}" \
            --slowest-count "${deep_optimal_depth15_count}" \
            --diagnose-optimal || true
        append_profile_summary_row "${summary_file}" "large-local" "optimal" "random_seed_${current_seed}_depth_15_count_${deep_optimal_depth15_count}" "${opt15_output}"
    done

    profile="${previous_profile}"

    echo "optimal large local summary: ${summary_file}"
}

run_optimal_auto_tail() {
    local previous_profile="${profile}"
    local summary_file="${output_dir}/${cache_mode}_optimal_auto_tail_summary.csv"
    profile="auto"

    {
        echo "profile,mode,benchmark,total_cases,solved,failed,total_elapsed_ms,total_nodes_expanded,p50_elapsed_ms,p90_elapsed_ms,p95_elapsed_ms,p99_elapsed_ms,max_elapsed_ms,warmup_elapsed_ms,wall_elapsed_ms,output_file"
    } > "${summary_file}"

    for current_seed in "${seed_list[@]}"; do
        local opt15_name="optimal_auto_tail_auto_random_${deep_optimal_depth15_count}_depth_15_seed_${current_seed}"
        local opt15_output="${output_dir}/${cache_mode}_${opt15_name}.csv"

        run_benchmark "${opt15_name}" \
            --mode optimal \
            --profile auto \
            --threads "${benchmark_threads}" \
            --max-memory-mb "${benchmark_max_memory_mb}" \
            --timeout-ms "${optimal_timeout_ms}" \
            --max-depth 15 \
            --case-set random \
            --random-count "${deep_optimal_depth15_count}" \
            --random-depth 15 \
            --random-seed "${current_seed}" \
            --slowest-count "${deep_optimal_depth15_count}" \
            --diagnose-optimal || true
        append_profile_summary_row "${summary_file}" "auto" "optimal" "random_seed_${current_seed}_depth_15_count_${deep_optimal_depth15_count}" "${opt15_output}"
    done

    profile="${previous_profile}"

    echo "optimal auto tail summary: ${summary_file}"
}

case "${suite}" in
    smoke)
        run_smoke
        ;;
    profile-smoke)
        run_profile_smoke
        ;;
    profile-realistic)
        run_profile_realistic
        ;;
    embedded-multiseed)
        run_embedded_multiseed
        ;;
    optimal-stress)
        run_optimal_stress
        ;;
    optimal-tail-cases)
        run_optimal_tail_cases
        ;;
    optimal-deep-probe)
        run_optimal_deep_probe
        ;;
    optimal-large-local)
        run_optimal_large_local
        ;;
    optimal-auto-tail)
        run_optimal_auto_tail
        ;;
    embedded-fast-tail-cases)
        run_embedded_fast_tail_cases
        ;;
    embedded-fast-failures)
        run_embedded_fast_failures
        ;;
    fast-100)
        run_fast_100
        ;;
    fast-1000)
        run_fast_1000
        ;;
    optimal-depth)
        run_optimal_depth
        ;;
    tail-diagnostics)
        run_tail_diagnostics
        ;;
    all)
        run_smoke
        run_profile_smoke
        run_profile_realistic
        run_embedded_multiseed
        run_optimal_stress
        run_optimal_tail_cases
        run_optimal_deep_probe
        run_optimal_auto_tail
        run_embedded_fast_tail_cases
        run_embedded_fast_failures
        run_fast_100
        run_optimal_depth
        run_tail_diagnostics
        ;;
    *)
        usage >&2
        exit 2
        ;;
esac
