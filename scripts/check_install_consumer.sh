#!/usr/bin/env bash
set -euo pipefail

build_dir=""
install_prefix=""
consumer_build_dir=""

usage() {
    cat <<'USAGE'
Usage: scripts/check_install_consumer.sh --build-dir DIR --install-prefix DIR --consumer-build-dir DIR

Installs the current build tree, configures a standalone downstream CMake
consumer with find_package(rubik CONFIG REQUIRED), builds it, and runs it.
USAGE
}

require_value() {
    if [[ $# -lt 2 ]]; then
        usage >&2
        exit 2
    fi
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir)
            require_value "$@"
            build_dir="$2"
            shift 2
            ;;
        --install-prefix)
            require_value "$@"
            install_prefix="$2"
            shift 2
            ;;
        --consumer-build-dir)
            require_value "$@"
            consumer_build_dir="$2"
            shift 2
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

if [[ -z "${build_dir}" || -z "${install_prefix}" || -z "${consumer_build_dir}" ]]; then
    usage >&2
    exit 2
fi

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="$(cd "${build_dir}" && pwd)"
install_prefix="$(mkdir -p "${install_prefix}" && cd "${install_prefix}" && pwd)"
consumer_build_dir="$(mkdir -p "${consumer_build_dir}" && cd "${consumer_build_dir}" && pwd)"

cmake --install "${build_dir}" --prefix "${install_prefix}"
cmake -S "${repo_root}/tests/consumer_smoke" -B "${consumer_build_dir}" \
    -DCMAKE_PREFIX_PATH="${install_prefix}"
cmake --build "${consumer_build_dir}"
"${consumer_build_dir}/rubik_consumer_smoke"
