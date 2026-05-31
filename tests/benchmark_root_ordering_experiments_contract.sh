#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
script="${repo_root}/scripts/benchmark_root_ordering_experiments.sh"

if ! grep -q 'wall_elapsed_ms' "${script}"; then
    echo "benchmark_root_ordering_experiments.sh must report wall_elapsed_ms" >&2
    exit 1
fi

if ! grep -q 'benchmark,wall_elapsed_ms' "${script}"; then
    echo "benchmark_root_ordering_experiments.sh must append benchmark wall elapsed rows" >&2
    exit 1
fi

if ! grep -q 'date +%s%3N' "${script}"; then
    echo "benchmark_root_ordering_experiments.sh must measure wall elapsed in milliseconds" >&2
    exit 1
fi

if ! grep -q 'cache_mode' "${script}"; then
    echo "benchmark_root_ordering_experiments.sh must report cache mode" >&2
    exit 1
fi

if ! grep -q 'rubik-cache-setup' "${script}"; then
    echo "benchmark_root_ordering_experiments.sh must prepare warm cache before timed cases" >&2
    exit 1
fi

if ! grep -q 'cache_setup_output' "${script}"; then
    echo "benchmark_root_ordering_experiments.sh must persist cache setup output" >&2
    exit 1
fi

if ! grep -q 'cache_setup_elapsed_ms' "${script}"; then
    echo "benchmark_root_ordering_experiments.sh must report cache setup elapsed time" >&2
    exit 1
fi

if ! grep -q 'high_bound_first' "${script}"; then
    echo "benchmark_root_ordering_experiments.sh must support the high_bound_first variant" >&2
    exit 1
fi

if ! grep -q 'positive_high_bound' "${script}"; then
    echo "benchmark_root_ordering_experiments.sh must support the positive_high_bound variant" >&2
    exit 1
fi
