#!/usr/bin/env bash
set -euo pipefail

profile="standard"
with_benchmarks="0"
with_large_local="0"

usage() {
    cat <<'USAGE'
Usage: scripts/release_check.sh [options]

Runs repeatable local release validation.

Profiles:
  quick       release-native-lto configure/build/test and install consumer check
  standard    release + release-native-lto configure/build/test and install consumer check
  full        standard checks plus asan-ubsan configure/build/test

Options:
  --profile NAME          quick|standard|full, default: standard
  --with-benchmarks       run profile-realistic, embedded-multiseed, and optimal-stress gates
  --with-large-local      also run large-local optimal benchmark gates
  -h, --help              show this help

Large-local gates use the public high-memory optimal profile and can take a
long time on a cold cache.
USAGE
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --profile)
            profile="$2"
            shift 2
            ;;
        --with-benchmarks)
            with_benchmarks="1"
            shift
            ;;
        --with-large-local)
            with_large_local="1"
            with_benchmarks="1"
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            usage >&2
            exit 2
            ;;
    esac
done

if [[ "${profile}" != "quick" && "${profile}" != "standard" && "${profile}" != "full" ]]; then
    usage >&2
    exit 2
fi

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${repo_root}"

run_step() {
    echo
    echo "==> $*"
    "$@"
}

run_preset() {
    local preset="$1"
    run_step cmake --preset "${preset}"
    run_step cmake --build --preset "${preset}"
    run_step ctest --preset "${preset}" --output-on-failure
}

echo "release_check,profile,${profile}"
echo "release_check,with_benchmarks,${with_benchmarks}"
echo "release_check,with_large_local,${with_large_local}"

case "${profile}" in
    quick)
        run_preset release-native-lto
        ;;
    standard)
        run_preset release
        run_preset release-native-lto
        ;;
    full)
        run_preset release
        run_preset release-native-lto
        run_preset asan-ubsan
        ;;
esac

if [[ "${profile}" == "quick" ]]; then
    install_build_dir="out/release-native-lto"
else
    install_build_dir="out/release"
fi

run_step cmake --build "${install_build_dir}" --target rubik-check-install-consumer
run_step "${repo_root}/scripts/check_release_archive.sh" --version 1.0.0 --output-dir "${repo_root}/dist"

if [[ "${with_benchmarks}" == "1" ]]; then
    run_step cmake --build out/release-native-lto --target rubik-benchmark-profile-realistic
    run_step cmake --build out/release-native-lto --target rubik-benchmark-profile-realistic-gates
    run_step cmake --build out/release-native-lto --target rubik-benchmark-embedded-multiseed
    run_step cmake --build out/release-native-lto --target rubik-benchmark-embedded-multiseed-gates
    run_step cmake --build out/release-native-lto --target rubik-benchmark-optimal-stress
    run_step cmake --build out/release-native-lto --target rubik-benchmark-optimal-stress-gates
fi

if [[ "${with_large_local}" == "1" ]]; then
    run_step cmake --build out/release-native-lto --target rubik-benchmark-optimal-large-local
    run_step cmake --build out/release-native-lto --target rubik-benchmark-optimal-large-local-gates
    run_step cmake --build out/release-native-lto --target rubik-benchmark-optimal-large-local-tail-8threads
    run_step cmake --build out/release-native-lto --target rubik-benchmark-optimal-large-local-tail-8threads-gates
fi

echo
echo "release_check,status,passed"
