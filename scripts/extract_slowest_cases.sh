#!/usr/bin/env bash
set -euo pipefail

input_dir="benchmark-results"
output_file=""
limit="25"

usage() {
    cat <<'USAGE'
Usage: scripts/extract_slowest_cases.sh [options]

Options:
  --input-dir DIR    directory containing rubik-bench CSV output, default: benchmark-results
  --output FILE      write CSV output to FILE instead of stdout
  --limit N          maximum number of rows to emit, default: 25
  -h, --help         show this help
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
        --input-dir)
            require_value "$@"
            input_dir="$2"
            shift 2
            ;;
        --output)
            require_value "$@"
            output_file="$2"
            shift 2
            ;;
        --limit)
            require_value "$@"
            limit="$2"
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

if [[ ! "${limit}" =~ ^[0-9]+$ || "${limit}" -lt 1 ]]; then
    usage >&2
    exit 2
fi

if [[ ! -d "${input_dir}" ]]; then
    echo "slowest extraction failed: input directory not found: ${input_dir}" >&2
    exit 1
fi

tmp_file="$(mktemp)"
trap 'rm -f "${tmp_file}"' EXIT

find "${input_dir}" -type f -name '*.csv' -print0 |
    sort -z |
    xargs -0 awk -F, '
        BEGIN {
            benchmark = "";
            mode = "";
            profile = "";
        }
        FNR == 1 {
            benchmark = "";
            mode = "";
            profile = "";
        }
        $1 == "benchmark" && $2 == "name" {
            benchmark = $3;
        }
        $1 == "benchmark" && $2 == "mode" {
            mode = $3;
        }
        $1 == "benchmark" && $2 == "profile" {
            profile = $3;
        }
        $1 == "slowest" {
            gsub(/^"/, "", $5);
            gsub(/"$/, "", $5);
            gsub(/^"/, "", $10);
            gsub(/"$/, "", $10);
            print $8 "," FILENAME "," benchmark "," mode "," profile "," $2 "," $3 "," $4 "," $5 "," $6 "," $7 "," $9 "," $10;
        }
    ' > "${tmp_file}"

emit() {
    echo "elapsed_ms,source_file,benchmark,mode,profile,slowest_rank,case_name,case_depth,scramble,status,move_count,nodes_expanded,solution"
    sort -t, -k1,1nr "${tmp_file}" | head -n "${limit}"
}

if [[ -n "${output_file}" ]]; then
    mkdir -p "$(dirname "${output_file}")"
    emit > "${output_file}"
    echo "slowest cases: ${output_file}"
else
    emit
fi
