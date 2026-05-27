#!/usr/bin/env bash
set -euo pipefail

version=""
output_dir="dist"

usage() {
    cat <<'USAGE'
Usage: scripts/check_release_archive.sh [options]

Creates a source release archive and verifies that generated local artifacts are
not included.

Options:
  --version VERSION       archive version label, default: CMake project version
  --output-dir DIR        output directory, default: dist
  -h, --help              show this help
USAGE
}

read_project_version() {
    local project_version

    project_version="$(
        sed -nE 's/^[[:space:]]*project\([^)]*VERSION[[:space:]]+([^[:space:])]+).*/\1/p' \
            CMakeLists.txt | head -n 1
    )"
    if [[ -z "${project_version}" ]]; then
        echo "release archive check failed: could not read project version from CMakeLists.txt" >&2
        exit 1
    fi

    printf '%s\n' "${project_version}"
}

require_value() {
    if [[ $# -lt 2 || "$2" == -* ]]; then
        usage >&2
        exit 2
    fi
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --version)
            require_value "$@"
            version="$2"
            shift 2
            ;;
        --output-dir)
            require_value "$@"
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

if [[ -z "${version}" ]]; then
    version="$(read_project_version)"
fi

mkdir -p "${output_dir}"

archive_name="rubik_cube_library-${version}.tar.gz"
archive_path="${output_dir}/${archive_name}"
checksum_path="${archive_path}.sha256"
prefix="rubik_cube_library-${version}/"
contents_file="$(mktemp)"
trap 'rm -f "${contents_file}"' EXIT

tar \
    --create \
    --gzip \
    --file "${archive_path}" \
    --transform "s#^#${prefix}#" \
    --exclude-vcs \
    --exclude='./build' \
    --exclude='./out' \
    --exclude='./benchmark-results' \
    --exclude='./dist' \
    --exclude='./install' \
    --exclude='./Testing' \
    --exclude='./.cache' \
    --exclude='./cmake-build-*' \
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

if grep -Eq "^${prefix}(build|out|benchmark-results|dist|install|Testing|\\.cache|cmake-build-[^/]+)/" "${contents_file}"; then
    echo "release archive check failed: included generated directory" >&2
    grep -E "^${prefix}(build|out|benchmark-results|dist|install|Testing|\\.cache|cmake-build-[^/]+)/" "${contents_file}" >&2
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
    "${prefix}cmake/version.hpp.in" \
    "${prefix}docs/release-${version}.md" \
    "${prefix}docs/release-candidate-2026-05-26.md" \
    "${prefix}docs/github-release-v${version}.md" \
    "${prefix}docs/benchmarks.md" \
    "${prefix}include/rubik/solver.hpp" \
    "${prefix}src/solver.cpp" \
    "${prefix}tests/fixtures/benchmark-results/sample_a.csv" \
    "${prefix}tests/fixtures/benchmark-results/sample_b.csv" \
    "${prefix}tests/consumer_smoke/CMakeLists.txt"
do
    if ! grep -q "^${required}$" "${contents_file}"; then
        echo "release archive check failed: missing required path ${required}" >&2
        exit 1
    fi
done

(
    cd "${output_dir}"
    sha256sum "${archive_name}" > "${archive_name}.sha256"
)

echo "release_archive,path,${archive_path}"
echo "release_archive,sha256_path,${checksum_path}"
echo "release_archive,status,passed"
