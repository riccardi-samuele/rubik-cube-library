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
  --with-benchmarks       run profile-realistic, Auto, embedded-multiseed, and optimal-stress gates
  --with-large-local      also run large-local, Auto tail, and Auto hardening optimal benchmark gates
  -h, --help              show this help

Large-local gates use the public high-memory optimal profile and can take a
long time on a cold cache. Auto hardening extends the adaptive optimal tail
coverage and can add several minutes to the run.
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

tmp_dirs=()
cleanup_tmp_dirs() {
    if [[ "${#tmp_dirs[@]}" -gt 0 ]]; then
        rm -rf "${tmp_dirs[@]}"
    fi
}
trap cleanup_tmp_dirs EXIT

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

read_project_version() {
    local version

    version="$(
        sed -nE 's/^[[:space:]]*project\([^)]*VERSION[[:space:]]+([^[:space:])]+).*/\1/p' \
            CMakeLists.txt | head -n 1
    )"
    if [[ -z "${version}" ]]; then
        echo "release_check failed: could not read project version from CMakeLists.txt" >&2
        exit 1
    fi

    printf '%s\n' "${version}"
}

check_source_archive_build() {
    local archive_path="$1"
    local archive_root="$2"
    local work_dir
    local source_dir
    local build_dir

    work_dir="$(mktemp -d)"
    tmp_dirs+=("${work_dir}")
    source_dir="${work_dir}/${archive_root}"
    build_dir="${work_dir}/archive-build"

    run_step tar -xzf "${archive_path}" -C "${work_dir}"
    run_step cmake -S "${source_dir}" -B "${build_dir}" -DCMAKE_BUILD_TYPE=Release
    run_step cmake --build "${build_dir}"
    run_step ctest --test-dir "${build_dir}" --output-on-failure
}

echo "release_check,profile,${profile}"
echo "release_check,with_benchmarks,${with_benchmarks}"
echo "release_check,with_large_local,${with_large_local}"
release_version="$(read_project_version)"
archive_root="rubik_cube_library-${release_version}"
archive_path="${repo_root}/dist/${archive_root}.tar.gz"
echo "release_check,version,${release_version}"

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
run_step "${repo_root}/scripts/check_release_archive.sh" --output-dir "${repo_root}/dist"
check_source_archive_build "${archive_path}" "${archive_root}"

if [[ "${with_benchmarks}" == "1" ]]; then
    run_step cmake --build out/release-native-lto --target rubik-benchmark-profile-realistic
    run_step cmake --build out/release-native-lto --target rubik-benchmark-profile-realistic-gates
    run_step cmake --build out/release-native-lto --target rubik-benchmark-auto-profile
    run_step cmake --build out/release-native-lto --target rubik-benchmark-auto-profile-gates
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
    run_step cmake --build out/release-native-lto --target rubik-benchmark-optimal-auto-tail
    run_step cmake --build out/release-native-lto --target rubik-benchmark-optimal-auto-tail-gates
    run_step cmake --build out/release-native-lto --target rubik-benchmark-optimal-auto-hardening
    run_step cmake --build out/release-native-lto --target rubik-benchmark-optimal-auto-hardening-gates
fi

echo
echo "release_check,status,passed"
