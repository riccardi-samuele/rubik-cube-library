#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${repo_root}"

matches="$(
    rg -n -i \
        "raspberry pi [0-9]+ estimate|planning range|[0-9]+x-[0-9]+x slower|desktop estimates for raspberry|desktop/orin|orin-style|temporary files and generated local artifacts|internal planning notes|internal process notes|local process notes|non-user-facing planning artifacts" \
        README.md CHANGELOG.md docs || true
)"

if [[ -n "${matches}" ]]; then
    echo "public docs check failed: remove unmeasured hardware estimates or internal process notes" >&2
    printf '%s\n' "${matches}" >&2
    exit 1
fi

echo "public docs hardware estimate check passed"
