#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 3 ]]; then
    echo "Usage: tests/expect_cli_success_without_faces.sh <expected-text> <blocked-faces> <command> [args...]" >&2
    exit 2
fi

expected="$1"
blocked_faces_csv="$2"
shift 2

output="$("$@" 2>&1)"
printf '%s\n' "${output}"

if ! grep -Fq "${expected}" <<<"${output}"; then
    echo "Expected output to contain: ${expected}" >&2
    exit 1
fi

solution_line="$(grep -E '^solution: ' <<<"${output}" || true)"
if [[ -z "${solution_line}" ]]; then
    echo "Expected output to contain a solution line" >&2
    exit 1
fi

IFS=',' read -r -a blocked_faces <<<"${blocked_faces_csv}"
for face in "${blocked_faces[@]}"; do
    if [[ "${solution_line}" =~ (^|[[:space:]])${face}(2|\'|)($|[[:space:]]) ]]; then
        echo "Solution unexpectedly contains blocked face ${face}: ${solution_line}" >&2
        exit 1
    fi
done
