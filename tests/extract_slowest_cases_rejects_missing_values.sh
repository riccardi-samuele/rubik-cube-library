#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "${tmp_dir}"' EXIT

check_missing_value() {
    local option="$1"
    local stdout_file="${tmp_dir}/${option#--}.stdout"
    local stderr_file="${tmp_dir}/${option#--}.stderr"
    local exit_code

    set +e
    "${repo_root}/scripts/extract_slowest_cases.sh" "${option}" > "${stdout_file}" 2> "${stderr_file}"
    exit_code="$?"
    set -e

    if [[ "${exit_code}" -ne 2 ]]; then
        echo "extract_slowest_cases missing value test failed for ${option}: expected exit code 2, got ${exit_code}" >&2
        cat "${stderr_file}" >&2
        exit 1
    fi

    if ! grep -q "Usage: scripts/extract_slowest_cases.sh" "${stderr_file}"; then
        echo "extract_slowest_cases missing value test failed for ${option}: usage was not printed to stderr" >&2
        cat "${stderr_file}" >&2
        exit 1
    fi

    if grep -q "unbound variable" "${stderr_file}"; then
        echo "extract_slowest_cases missing value test failed for ${option}: shell error leaked to stderr" >&2
        cat "${stderr_file}" >&2
        exit 1
    fi
}

check_missing_value --input-dir
check_missing_value --output
check_missing_value --limit
