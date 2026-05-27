#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 3 ]]; then
    echo "Usage: tests/expect_cli_error_empty_cache.sh <expected-text> <cache-dir> <command> [args...]" >&2
    exit 2
fi

expected="$1"
cache_dir="$2"
shift 2

rm -rf "${cache_dir}"
mkdir -p "${cache_dir}"

set +e
output="$(RUBIK_TABLE_CACHE_DIR="${cache_dir}" "$@" 2>&1)"
status=$?
set -e

printf '%s\n' "${output}"

rm -rf "${cache_dir}"

if [[ "${status}" -eq 0 ]]; then
    echo "Expected command to fail, but it exited with status 0" >&2
    exit 1
fi

if ! grep -Fq "${expected}" <<<"${output}"; then
    echo "Expected output to contain: ${expected}" >&2
    exit 1
fi
