#!/usr/bin/env bash
set -euo pipefail

version="1.0.0"
output_dir="dist"

usage() {
    cat <<'USAGE'
Usage: scripts/check_release_archive.sh [options]

Creates a source release archive and verifies that generated local artifacts are
not included.

Options:
  --version VERSION       archive version label, default: 1.0.0
  --output-dir DIR        output directory, default: dist
  -h, --help              show this help
USAGE
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --version)
            version="$2"
            shift 2
            ;;
        --output-dir)
            output_dir="$2"
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

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${repo_root}"

mkdir -p "${output_dir}"

archive_name="rubik_cube_library-${version}.tar.gz"
archive_path="${output_dir}/${archive_name}"
prefix="rubik_cube_library-${version}/"
contents_file="$(mktemp)"
trap 'rm -f "${contents_file}"' EXIT

tar \
    --create \
    --gzip \
    --file "${archive_path}" \
    --transform "s#^#${prefix}#" \
    --exclude-vcs \
    --exclude='build' \
    --exclude='out' \
    --exclude='benchmark-results' \
    --exclude='dist' \
    --exclude='install' \
    --exclude='Testing' \
    --exclude='.cache' \
    --exclude='cmake-build-*' \
    --exclude='.idea' \
    --exclude='.vscode' \
    --exclude='compile_commands.json' \
    --exclude='*.rpt' \
    --exclude='*.rpt.tmp' \
    .github \
    apps \
    cmake \
    docs \
    examples \
    include \
    scripts \
    src \
    tests \
    .gitignore \
    CHANGELOG.md \
    CMakeLists.txt \
    CMakePresets.json \
    LICENSE \
    NOTICE \
    README.md

tar -tzf "${archive_path}" | sed "s#^${prefix}\\./#${prefix}#" > "${contents_file}"

for forbidden in \
    "${prefix}build/" \
    "${prefix}out/" \
    "${prefix}benchmark-results/" \
    "${prefix}dist/" \
    "${prefix}install/" \
    "${prefix}Testing/" \
    "${prefix}.cache/"
do
    if grep -q "^${forbidden}" "${contents_file}"; then
        echo "release archive check failed: included generated path ${forbidden}" >&2
        exit 1
    fi
done

if grep -Eq "(^|/)(build|out|benchmark-results|dist|install|Testing|\\.cache|cmake-build-[^/]+)/" "${contents_file}"; then
    echo "release archive check failed: included generated directory" >&2
    grep -E "(^|/)(build|out|benchmark-results|dist|install|Testing|\\.cache|cmake-build-[^/]+)/" "${contents_file}" >&2
    exit 1
fi

if grep -Eq "(^|/)(compile_commands\\.json|[^/]+\\.rpt|[^/]+\\.rpt\\.tmp)$" "${contents_file}"; then
    echo "release archive check failed: included generated file" >&2
    grep -E "(^|/)(compile_commands\\.json|[^/]+\\.rpt|[^/]+\\.rpt\\.tmp)$" "${contents_file}" >&2
    exit 1
fi

for required in \
    "${prefix}CMakeLists.txt" \
    "${prefix}README.md" \
    "${prefix}LICENSE" \
    "${prefix}NOTICE" \
    "${prefix}CHANGELOG.md" \
    "${prefix}docs/release-1.0.0.md" \
    "${prefix}docs/release-candidate-2026-05-26.md" \
    "${prefix}docs/github-release-v1.0.0.md" \
    "${prefix}docs/benchmarks.md" \
    "${prefix}include/rubik/solver.hpp" \
    "${prefix}src/solver.cpp" \
    "${prefix}tests/consumer_smoke/CMakeLists.txt"
do
    if ! grep -q "^${required}$" "${contents_file}"; then
        echo "release archive check failed: missing required path ${required}" >&2
        exit 1
    fi
done

echo "release_archive,path,${archive_path}"
echo "release_archive,status,passed"
