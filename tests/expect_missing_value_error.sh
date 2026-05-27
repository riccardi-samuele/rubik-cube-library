#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 3 ]]; then
    echo "Usage: tests/expect_missing_value_error.sh SCRIPT USAGE_PREFIX OPTION [OPTION...]" >&2
    exit 2
fi

script_path="$1"
usage_prefix="$2"
shift 2

tmp_dir="$(mktemp -d)"
trap 'rm -rf "${tmp_dir}"' EXIT

for option in "$@"; do
    stdout_file="${tmp_dir}/${option#--}.stdout"
    stderr_file="${tmp_dir}/${option#--}.stderr"

    set +e
    "${script_path}" "${option}" > "${stdout_file}" 2> "${stderr_file}"
    exit_code="$?"
    set -e

    if [[ "${exit_code}" -ne 2 ]]; then
        echo "missing value test failed for ${script_path} ${option}: expected exit code 2, got ${exit_code}" >&2
        cat "${stderr_file}" >&2
        exit 1
    fi

    if ! grep -q "${usage_prefix}" "${stderr_file}"; then
        echo "missing value test failed for ${script_path} ${option}: usage was not printed to stderr" >&2
        cat "${stderr_file}" >&2
        exit 1
    fi

    if grep -q "unbound variable" "${stderr_file}"; then
        echo "missing value test failed for ${script_path} ${option}: shell error leaked to stderr" >&2
        cat "${stderr_file}" >&2
        exit 1
    fi
done
