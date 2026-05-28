#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${repo_root}"

failures=0

require_text() {
    local file="$1"
    local text="$2"

    if ! grep -Fq "${text}" "${file}"; then
        echo "public examples API check failed: ${file} is missing: ${text}" >&2
        failures=1
    fi
}

reject_text() {
    local file="$1"
    local text="$2"

    if grep -Fq "${text}" "${file}"; then
        echo "public examples API check failed: ${file} contains stale text: ${text}" >&2
        failures=1
    fi
}

require_text examples/solve_optimal.cpp ".profile = rubik::SolveProfile::Auto"
require_text examples/solve_optimal.cpp ".cachePolicy = rubik::CachePolicy::Auto"
require_text examples/solve_optimal.cpp ".threads = 0"
require_text examples/solve_optimal.cpp "result.plan.effectiveProfile"
require_text examples/cache_setup.cpp "#include \"rubik/cache.hpp\""
require_text examples/cache_setup.cpp "rubik::prepareCache"
require_text examples/solve_fast.cpp "non_optimal: true"

reject_text examples/solve_optimal.cpp ".profile = rubik::SolveProfile::Default"
reject_text examples/cache_setup.cpp "rubik/pruning_tables.hpp"
reject_text examples/cache_setup.cpp "rubik::pruning_tables::"

if [[ "${failures}" != "0" ]]; then
    exit 1
fi

echo "public examples API check passed"
