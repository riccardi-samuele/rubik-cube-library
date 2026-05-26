#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 ]]; then
    echo "Usage: tests/expect_cli_error.sh <expected-text> <command> [args...]" >&2
    exit 2
fi

expected="$1"
shift

set +e
output="$("$@" 2>&1)"
status=$?
set -e

printf '%s\n' "${output}"

if [[ "${status}" -eq 0 ]]; then
    echo "Expected command to fail, but it exited with status 0" >&2
    exit 1
fi

if ! grep -Fq "${expected}" <<<"${output}"; then
    echo "Expected output to contain: ${expected}" >&2
    exit 1
fi
