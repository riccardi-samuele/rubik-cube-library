#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"

build_dir="out/release-native-lto"
cache_dir="${RUBIK_TABLE_CACHE_DIR:-/tmp/rubik_cube_library_v6_tail_baseline_cache}"
output_dir="benchmark-results/v6-conservative-root-probe"
corpus_file="benchmarks/v6_conservative_root_corpus.csv"
timeout_ms="30000"
threads="0"
max_memory_mb="2048"
cache_mode="require-warm"
candidate_env=""

usage() {
    cat <<'USAGE'
Usage: scripts/run_v6_conservative_root_probe.sh [options]

Options:
  --build-dir DIR        CMake build directory, default: out/release-native-lto
  --cache-dir DIR        pruning table cache directory
  --output-dir DIR       output directory, default: benchmark-results/v6-conservative-root-probe
  --corpus-file FILE     CSV corpus, default: benchmarks/v6_conservative_root_corpus.csv
  --timeout-ms N         per-case timeout, default: 30000
  --threads N            solver threads, default: 0
  --max-memory-mb N      solver memory limit, default: 2048
  --cache-mode MODE      require-warm|reuse, default: require-warm
  --candidate-env ENV    optional KEY=VALUE candidate environment assignment
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
        --corpus-file)
            require_value "$@"
            corpus_file="$2"
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
        --cache-mode)
            require_value "$@"
            cache_mode="$2"
            shift 2
            ;;
        --candidate-env)
            require_value "$@"
            candidate_env="$2"
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

for numeric in "${timeout_ms}" "${threads}" "${max_memory_mb}"; do
    if [[ ! "${numeric}" =~ ^[0-9]+$ ]]; then
        usage >&2
        exit 2
    fi
done

if (( timeout_ms < 1 || max_memory_mb < 1 )); then
    usage >&2
    exit 2
fi

if [[ "${cache_mode}" != "require-warm" && "${cache_mode}" != "reuse" ]]; then
    usage >&2
    exit 2
fi

if [[ -n "${candidate_env}" && ! "${candidate_env}" =~ ^[A-Za-z_][A-Za-z0-9_]*=.*$ ]]; then
    usage >&2
    exit 2
fi

if [[ ! -f "${corpus_file}" ]]; then
    echo "v6 conservative root probe failed: corpus file not found: ${corpus_file}" >&2
    exit 1
fi

validate_corpus() {
    local file="$1"
    local line_no=0
    local rows=0
    while IFS=, read -r suite seed start_index depth count expected_reason; do
        line_no=$((line_no + 1))
        if (( line_no == 1 )); then
            if [[ "${suite},${seed},${start_index},${depth},${count},${expected_reason}" != "suite,seed,start_index,depth,count,expected_reason" ]]; then
                echo "v6 conservative root probe failed: invalid corpus header" >&2
                exit 1
            fi
            continue
        fi
        if [[ "${suite}" != "tail" && "${suite}" != "hardening" ]]; then
            echo "v6 conservative root probe failed: unsupported corpus row ${line_no}: ${suite},${seed},${start_index},${depth},${count},${expected_reason}" >&2
            exit 1
        fi
        if [[ ! "${seed}" =~ ^[0-9]+$ || ! "${start_index}" =~ ^[0-9]+$ || ! "${depth}" =~ ^[0-9]+$ || ! "${count}" =~ ^[0-9]+$ ]]; then
            echo "v6 conservative root probe failed: unsupported corpus row ${line_no}: ${suite},${seed},${start_index},${depth},${count},${expected_reason}" >&2
            exit 1
        fi
        if (( start_index < 1 || depth < 1 || count < 1 )); then
            echo "v6 conservative root probe failed: unsupported corpus row ${line_no}: ${suite},${seed},${start_index},${depth},${count},${expected_reason}" >&2
            exit 1
        fi
        if [[ "${expected_reason}" != "conservative_root" ]]; then
            echo "v6 conservative root probe failed: unsupported corpus row ${line_no}: ${suite},${seed},${start_index},${depth},${count},${expected_reason}" >&2
            exit 1
        fi
        rows=$((rows + 1))
    done < "${file}"

    if (( rows < 1 )); then
        echo "v6 conservative root probe failed: corpus contains no rows: ${file}" >&2
        exit 1
    fi
}

validate_corpus "${corpus_file}"

cmake --build "${build_dir}" --target rubik-bench rubik-cache-setup

bench="${build_dir}/rubik-bench"
cache_setup="${build_dir}/rubik-cache-setup"
if [[ ! -x "${bench}" || ! -x "${cache_setup}" ]]; then
    echo "required benchmark binaries are missing in ${build_dir}" >&2
    exit 1
fi

mkdir -p "${cache_dir}" "${output_dir}"

manifest_file="${output_dir}/manifest.csv"
cache_setup_output="${output_dir}/cache_setup.csv"
{
    echo "key,value"
    echo "git_revision,$(git -C "${repo_root}" rev-parse --short HEAD 2>/dev/null || echo unknown)"
    echo "build_dir,${build_dir}"
    echo "cache_dir,${cache_dir}"
    echo "output_dir,${output_dir}"
    echo "corpus_file,${corpus_file}"
    echo "timeout_ms,${timeout_ms}"
    echo "threads,${threads}"
    echo "max_memory_mb,${max_memory_mb}"
    echo "cache_mode,${cache_mode}"
    echo "candidate_env,${candidate_env}"
    echo "cache_setup_output,${cache_setup_output}"
} > "${manifest_file}"

if [[ "${cache_mode}" == "require-warm" ]]; then
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
        echo "cache is not warm for V6 conservative root probe; run rubik-cache-setup first (bytes_missing=${bytes_missing:-unknown})" >&2
        exit 1
    fi
else
    {
        echo "cache_setup,status,Skipped"
        echo "cache_setup,message,cache setup skipped by cache-mode reuse"
    } > "${cache_setup_output}"
fi

run_variant() {
    local variant_name="$1"
    local env_assignment="$2"
    local variant_dir="${output_dir}/${variant_name}"
    local summary_file="${variant_dir}/summary.csv"
    mkdir -p "${variant_dir}"

    {
        echo "variant,suite,seed,start_index,depth,count,status,optimal,move_count,elapsed_ms,nodes_expanded,adaptive_reason,output_file"
    } > "${summary_file}"

    while IFS=, read -r suite seed start_index depth count expected_reason; do
        if [[ "${suite}" == "suite" ]]; then
            continue
        fi

        local name="v6_conservative_root_${suite}_depth_${depth}_seed_${seed}_start_${start_index}_count_${count}"
        local output_file="${variant_dir}/warm_${name}.csv"
        {
            echo "benchmark,name,${name}"
            echo "benchmark,variant,${variant_name}"
            echo "benchmark,suite,${suite}"
            echo "benchmark,expected_reason,${expected_reason}"
            echo "benchmark,cache_dir,${cache_dir}"
        } > "${output_file}"

        set +e
        if [[ -n "${env_assignment}" ]]; then
            env RUBIK_TABLE_CACHE_DIR="${cache_dir}" "${env_assignment}" "${bench}" \
                --mode optimal \
                --profile auto \
                --threads "${threads}" \
                --max-memory-mb "${max_memory_mb}" \
                --timeout-ms "${timeout_ms}" \
                --max-depth "${depth}" \
                --case-set random \
                --random-count "${count}" \
                --random-depth "${depth}" \
                --random-seed "${seed}" \
                --random-start-index "${start_index}" \
                --slowest-count "${count}" \
                --diagnose-optimal \
                >> "${output_file}"
        else
            env RUBIK_TABLE_CACHE_DIR="${cache_dir}" "${bench}" \
                --mode optimal \
                --profile auto \
                --threads "${threads}" \
                --max-memory-mb "${max_memory_mb}" \
                --timeout-ms "${timeout_ms}" \
                --max-depth "${depth}" \
                --case-set random \
                --random-count "${count}" \
                --random-depth "${depth}" \
                --random-seed "${seed}" \
                --random-start-index "${start_index}" \
                --slowest-count "${count}" \
                --diagnose-optimal \
                >> "${output_file}"
        fi
        local command_status="$?"
        set -e

        local status
        local optimal
        local move_count
        local elapsed
        local nodes
        local adaptive_reason
        status="$(awk -F, '$1 ~ /^random_/ { print $4; exit }' "${output_file}")"
        optimal="$(awk -F, '$1 ~ /^random_/ { print $5; exit }' "${output_file}")"
        move_count="$(awk -F, '$1 ~ /^random_/ { print $6; exit }' "${output_file}")"
        elapsed="$(awk -F, '$1 ~ /^random_/ { print $8; exit }' "${output_file}")"
        nodes="$(awk -F, '$1 ~ /^random_/ { print $9; exit }' "${output_file}")"
        adaptive_reason="$(
            awk -F, '$1 ~ /^random_/ { print $16; exit }' "${output_file}" \
                | sed -n 's/.*adaptive_reason=\([^;"]*\).*/\1/p'
        )"
        echo "${variant_name},${suite},${seed},${start_index},${depth},${count},${status:-Unknown},${optimal:-false},${move_count:-0},${elapsed:-0},${nodes:-0},${adaptive_reason:-unknown},${output_file}" >> "${summary_file}"

        if (( command_status != 0 )); then
            echo "v6 conservative root probe failed: benchmark command failed for ${name}" >&2
            exit 1
        fi
        if [[ "${status}" != "Optimal" || "${optimal}" != "true" ]]; then
            echo "v6 conservative root probe failed: non-optimal result for ${name}" >&2
            exit 1
        fi
        if [[ "${adaptive_reason}" != "${expected_reason}" ]]; then
            echo "v6 conservative root probe failed: expected ${expected_reason}, got ${adaptive_reason:-unknown} for ${name}" >&2
            exit 1
        fi
    done < "${corpus_file}"
}

run_variant "default" ""
if [[ -n "${candidate_env}" ]]; then
    run_variant "candidate" "${candidate_env}"
fi

echo "v6 conservative root probe manifest: ${manifest_file}"
echo "v6 conservative root default summary: ${output_dir}/default/summary.csv"
if [[ -n "${candidate_env}" ]]; then
    echo "v6 conservative root candidate summary: ${output_dir}/candidate/summary.csv"
fi
