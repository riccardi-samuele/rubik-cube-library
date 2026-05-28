#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${repo_root}"

version="$(
    sed -nE 's/^[[:space:]]*project\([^)]*VERSION[[:space:]]+([^[:space:])]+).*/\1/p' \
        CMakeLists.txt | head -n 1
)"

if [[ -z "${version}" ]]; then
    echo "public docs version check failed: could not read project version" >&2
    exit 1
fi

major="${version%%.*}"

require_text() {
    local file="$1"
    local text="$2"

    if ! grep -Fq "${text}" "${file}"; then
        echo "public docs version check failed: ${file} is missing: ${text}" >&2
        exit 1
    fi
}

reject_text() {
    local file="$1"
    local text="$2"

    if grep -Fq "${text}" "${file}"; then
        echo "public docs version check failed: ${file} contains stale text: ${text}" >&2
        exit 1
    fi
}

require_text README.md "currently at \`${version}\`"
require_text README.md "static_assert(rubik::version_major == ${major});"
require_text README.md "docs/release-${version}.md"
require_text README.md "docs/github-release-v${version}.md"
require_text README.md "[API Stability - ${version}](docs/api-stability-${version}.md)"
require_text README.md "[Release Checklist - ${version}](docs/release-${version}.md)"

require_text docs/api.md "current \`${version}\`"
require_text docs/api.md "[API Stability - ${version}](api-stability-${version}.md)"
require_text docs/roadmap.md "The \`${version}\` release"
require_text docs/roadmap.md "Current ${major}.0 Release"
require_text docs/benchmarks.md "legacy V3 adaptive Auto regression gate"

for stale in 1.0.0 2.0.0 3.0.0; do
    if [[ "${stale}" != "${version}" ]]; then
        reject_text README.md "currently at \`${stale}\`"
        reject_text docs/api.md "current \`${stale}\`"
        reject_text docs/roadmap.md "The \`${stale}\` release is"
    fi
done

reject_text docs/benchmarks.md "current V3"
reject_text docs/benchmarks.md "release-candidate V3"

echo "public docs current version check passed"
