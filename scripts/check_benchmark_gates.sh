#!/usr/bin/env bash
set -euo pipefail

summary_file=""
declare -a gates=()

usage() {
    cat <<'USAGE'
Usage: scripts/check_benchmark_gates.sh --summary-file FILE --gate SPEC [--gate SPEC...]

Gate SPEC format:
  profile,mode,benchmark,min_solved,max_p95_ms,max_p99_ms,max_max_ms

Example:
  embedded,fast,random_depth_20_count_100,100,300,600,700

Use -1 for a latency threshold to skip that check.
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
        --summary-file)
            require_value "$@"
            summary_file="$2"
            shift 2
            ;;
        --gate)
            require_value "$@"
            gates+=("$2")
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

if [[ -z "${summary_file}" || ${#gates[@]} -eq 0 ]]; then
    usage >&2
    exit 2
fi

if [[ ! -f "${summary_file}" ]]; then
    echo "benchmark gate failed: summary file not found: ${summary_file}" >&2
    exit 1
fi

check_gate() {
    local gate="$1"
    local profile mode benchmark min_solved max_p95 max_p99 max_max
    IFS=',' read -r profile mode benchmark min_solved max_p95 max_p99 max_max <<< "${gate}"

    if [[ -z "${profile}" || -z "${mode}" || -z "${benchmark}" || -z "${min_solved}" || -z "${max_p95}" || -z "${max_p99}" || -z "${max_max}" ]]; then
        echo "benchmark gate failed: invalid gate spec: ${gate}" >&2
        return 1
    fi

    awk -F, -v gate="${gate}" \
        -v profile="${profile}" \
        -v mode="${mode}" \
        -v benchmark="${benchmark}" \
        -v min_solved="${min_solved}" \
        -v max_p95="${max_p95}" \
        -v max_p99="${max_p99}" \
        -v max_max="${max_max}" '
        NR == 1 {
            for (i = 1; i <= NF; ++i) {
                column[$i] = i
            }
            required[1] = "profile"
            required[2] = "mode"
            required[3] = "benchmark"
            required[4] = "solved"
            required[5] = "p95_elapsed_ms"
            required[6] = "p99_elapsed_ms"
            required[7] = "max_elapsed_ms"
            for (i = 1; i <= 7; ++i) {
                if (!(required[i] in column)) {
                    printf("benchmark gate failed: missing column %s in %s\n", required[i], FILENAME) > "/dev/stderr"
                    exit 2
                }
            }
            next
        }
        $column["profile"] == profile && $column["mode"] == mode && $column["benchmark"] == benchmark {
            found = 1
            solved = $column["solved"] + 0
            p95 = $column["p95_elapsed_ms"] + 0
            p99 = $column["p99_elapsed_ms"] + 0
            max_elapsed = $column["max_elapsed_ms"] + 0

            if (solved < min_solved) {
                printf("benchmark gate failed: %s solved=%d < min_solved=%d\n", gate, solved, min_solved) > "/dev/stderr"
                failed = 1
            }
            if (max_p95 >= 0 && p95 > max_p95) {
                printf("benchmark gate failed: %s p95_elapsed_ms=%d > max_p95_ms=%d\n", gate, p95, max_p95) > "/dev/stderr"
                failed = 1
            }
            if (max_p99 >= 0 && p99 > max_p99) {
                printf("benchmark gate failed: %s p99_elapsed_ms=%d > max_p99_ms=%d\n", gate, p99, max_p99) > "/dev/stderr"
                failed = 1
            }
            if (max_max >= 0 && max_elapsed > max_max) {
                printf("benchmark gate failed: %s max_elapsed_ms=%d > max_max_ms=%d\n", gate, max_elapsed, max_max) > "/dev/stderr"
                failed = 1
            }
            if (!failed) {
                printf("benchmark gate passed: %s solved=%d p95=%d p99=%d max=%d\n", gate, solved, p95, p99, max_elapsed)
            }
            exit failed
        }
        END {
            if (!found) {
                printf("benchmark gate failed: no row for %s/%s/%s in %s\n", profile, mode, benchmark, FILENAME) > "/dev/stderr"
                exit 1
            }
        }
    ' "${summary_file}"
}

status=0
for gate in "${gates[@]}"; do
    check_gate "${gate}" || status=1
done

exit "${status}"
