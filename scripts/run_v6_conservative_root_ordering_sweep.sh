#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

build_dir="out/release-native-lto"
cache_dir="${RUBIK_TABLE_CACHE_DIR:-/tmp/rubik_cube_library_v6_tail_baseline_cache}"
output_dir="benchmark-results/v6-conservative-root-ordering-sweep"
corpus_file="benchmarks/v6_conservative_root_corpus.csv"
timeout_ms="30000"
threads="0"
max_memory_mb="2048"
candidates="reverse_tie,high_bound_first,phase2_tiebreak,positive_high_bound"
probe_script="${script_dir}/run_v6_conservative_root_probe.sh"
compare_script="${script_dir}/compare_v6_latency.py"

usage() {
    cat <<'USAGE'
Usage: scripts/run_v6_conservative_root_ordering_sweep.sh [options]

Options:
  --build-dir DIR        CMake build directory, default: out/release-native-lto
  --cache-dir DIR        pruning table cache directory
  --output-dir DIR       output directory, default: benchmark-results/v6-conservative-root-ordering-sweep
  --corpus-file FILE     CSV corpus, default: benchmarks/v6_conservative_root_corpus.csv
  --timeout-ms N         per-case timeout, default: 30000
  --threads N            solver threads, default: 0
  --max-memory-mb N      solver memory limit, default: 2048
  --candidates LIST      comma-separated candidates: reverse_tie,high_bound_first,phase2_tiebreak,positive_high_bound
  --probe-script FILE    probe runner, default: scripts/run_v6_conservative_root_probe.sh
  --compare-script FILE  comparison script, default: scripts/compare_v6_latency.py
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
        --candidates)
            require_value "$@"
            candidates="$2"
            shift 2
            ;;
        --probe-script)
            require_value "$@"
            probe_script="$2"
            shift 2
            ;;
        --compare-script)
            require_value "$@"
            compare_script="$2"
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

if [[ ! -x "${probe_script}" ]]; then
    echo "v6 conservative root ordering sweep failed: probe script is not executable: ${probe_script}" >&2
    exit 1
fi

if [[ ! -x "${compare_script}" ]]; then
    echo "v6 conservative root ordering sweep failed: compare script is not executable: ${compare_script}" >&2
    exit 1
fi

IFS=',' read -r -a candidate_list <<< "${candidates}"
if (( ${#candidate_list[@]} < 1 )); then
    usage >&2
    exit 2
fi

for candidate in "${candidate_list[@]}"; do
    case "${candidate}" in
        reverse_tie|high_bound_first|phase2_tiebreak|positive_high_bound)
            ;;
        *)
            echo "unsupported root ordering candidate: ${candidate}" >&2
            exit 2
            ;;
    esac
done

mkdir -p "${output_dir}"
summary_file="${output_dir}/summary.csv"
{
    echo "candidate,common_cases,baseline_elapsed_ms,candidate_elapsed_ms,elapsed_delta_ms,elapsed_delta_percent,baseline_max_elapsed_ms,candidate_max_elapsed_ms,max_elapsed_delta_ms,baseline_nodes,candidate_nodes,nodes_delta,winner,comparison_file"
} > "${summary_file}"

for candidate in "${candidate_list[@]}"; do
    candidate_output_dir="${output_dir}/${candidate}"
    comparison_file="${candidate_output_dir}/comparison.csv"

    "${probe_script}" \
        --build-dir "${build_dir}" \
        --cache-dir "${cache_dir}" \
        --output-dir "${candidate_output_dir}" \
        --corpus-file "${corpus_file}" \
        --timeout-ms "${timeout_ms}" \
        --threads "${threads}" \
        --max-memory-mb "${max_memory_mb}" \
        --cache-mode require-warm \
        --candidate-env "RUBIK_EXPERIMENTAL_ROOT_ORDERING=${candidate}"

    "${compare_script}" \
        --baseline-dir "${candidate_output_dir}/default" \
        --candidate-dir "${candidate_output_dir}/candidate" \
        --output "${comparison_file}"

    python3 - "${comparison_file}" "${summary_file}" "${candidate}" <<'PY'
import csv
import sys

comparison_file, summary_file, candidate = sys.argv[1:]
with open(comparison_file, newline="") as handle:
    rows = list(csv.DictReader(handle))
summary_rows = [row for row in rows if row.get("case_key") == "__summary__"]
if len(summary_rows) != 1:
    print(f"comparison summary missing for {candidate}: {comparison_file}", file=sys.stderr)
    sys.exit(1)
row = summary_rows[0]
with open(summary_file, "a", newline="") as handle:
    writer = csv.writer(handle)
    writer.writerow([
        candidate,
        row["common_cases"],
        row["baseline_elapsed_ms"],
        row["candidate_elapsed_ms"],
        row["elapsed_delta_ms"],
        row["elapsed_delta_percent"],
        row["baseline_max_elapsed_ms"],
        row["candidate_max_elapsed_ms"],
        row["max_elapsed_delta_ms"],
        row["baseline_nodes"],
        row["candidate_nodes"],
        row["nodes_delta"],
        row["winner"],
        comparison_file,
    ])
PY
done

echo "v6 conservative root ordering sweep summary: ${summary_file}"
