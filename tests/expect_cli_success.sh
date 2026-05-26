#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 ]]; then
    echo "Usage: tests/expect_cli_success.sh <expected-text> <command> [args...]" >&2
    exit 2
fi

expected="$1"
shift

output="$("$@" 2>&1)"
printf '%s\n' "${output}"

if ! grep -Fq "${expected}" <<<"${output}"; then
    echo "Expected output to contain: ${expected}" >&2
    exit 1
fi
