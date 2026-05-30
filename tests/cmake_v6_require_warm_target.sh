#!/usr/bin/env bash
set -euo pipefail

cmake_file="${1:-CMakeLists.txt}"

grep -q "rubik-benchmark-v6-tail-baseline-require-warm" "${cmake_file}"
grep -q -- "--cache-mode require-warm" "${cmake_file}"
