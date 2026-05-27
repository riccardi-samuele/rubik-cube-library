#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "${tmp_dir}"' EXIT

stdout_file="${tmp_dir}/stdout.txt"
stderr_file="${tmp_dir}/stderr.txt"

set +e
"${repo_root}/scripts/release_check.sh" --profile > "${stdout_file}" 2> "${stderr_file}"
exit_code="$?"
set -e

if [[ "${exit_code}" -ne 2 ]]; then
    echo "release_check missing profile test failed: expected exit code 2, got ${exit_code}" >&2
    cat "${stderr_file}" >&2
    exit 1
fi

if ! grep -q "Usage: scripts/release_check.sh" "${stderr_file}"; then
    echo "release_check missing profile test failed: usage was not printed to stderr" >&2
    cat "${stderr_file}" >&2
    exit 1
fi

if grep -q "unbound variable" "${stderr_file}"; then
    echo "release_check missing profile test failed: shell error leaked to stderr" >&2
    cat "${stderr_file}" >&2
    exit 1
fi
