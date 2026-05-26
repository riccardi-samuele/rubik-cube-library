#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "${tmp_dir}"' EXIT

test_repo="${tmp_dir}/repo"
mkdir -p \
    "${test_repo}/.github" \
    "${test_repo}/apps" \
    "${test_repo}/cmake" \
    "${test_repo}/docs" \
    "${test_repo}/examples" \
    "${test_repo}/include/rubik" \
    "${test_repo}/scripts" \
    "${test_repo}/src" \
    "${test_repo}/tests/consumer_smoke" \
    "${test_repo}/tests/fixtures/benchmark-results"

cp "${repo_root}/scripts/check_release_archive.sh" "${test_repo}/scripts/check_release_archive.sh"
chmod +x "${test_repo}/scripts/check_release_archive.sh"

cat > "${test_repo}/CMakeLists.txt" <<'CMAKE'
cmake_minimum_required(VERSION 3.20)
project(rubik_cube_library VERSION 9.8.7 LANGUAGES CXX)
CMAKE

touch \
    "${test_repo}/.gitignore" \
    "${test_repo}/CHANGELOG.md" \
    "${test_repo}/CMakePresets.json" \
    "${test_repo}/LICENSE" \
    "${test_repo}/NOTICE" \
    "${test_repo}/README.md" \
    "${test_repo}/cmake/version.hpp.in" \
    "${test_repo}/docs/release-9.8.7.md" \
    "${test_repo}/docs/release-candidate-2026-05-26.md" \
    "${test_repo}/docs/github-release-v9.8.7.md" \
    "${test_repo}/docs/benchmarks.md" \
    "${test_repo}/include/rubik/solver.hpp" \
    "${test_repo}/src/solver.cpp" \
    "${test_repo}/tests/fixtures/benchmark-results/sample_a.csv" \
    "${test_repo}/tests/fixtures/benchmark-results/sample_b.csv" \
    "${test_repo}/tests/consumer_smoke/CMakeLists.txt"

"${test_repo}/scripts/check_release_archive.sh" \
    --version 9.8.7 \
    --output-dir "${tmp_dir}/dist" > "${tmp_dir}/archive.log"

if ! grep -q "release_archive,status,passed" "${tmp_dir}/archive.log"; then
    echo "check_release_archive versioned docs test failed" >&2
    cat "${tmp_dir}/archive.log" >&2
    exit 1
fi

"${test_repo}/scripts/check_release_archive.sh" \
    --output-dir "${tmp_dir}/dist-default" > "${tmp_dir}/archive-default.log"

if ! grep -q "release_archive,path,${tmp_dir}/dist-default/rubik_cube_library-9.8.7.tar.gz" "${tmp_dir}/archive-default.log"; then
    echo "check_release_archive default CMake version test failed" >&2
    cat "${tmp_dir}/archive-default.log" >&2
    exit 1
fi

checksum_path="${tmp_dir}/dist-default/rubik_cube_library-9.8.7.tar.gz.sha256"

if [[ ! -f "${checksum_path}" ]]; then
    echo "check_release_archive checksum test failed: missing ${checksum_path}" >&2
    cat "${tmp_dir}/archive-default.log" >&2
    exit 1
fi

if ! grep -Eq "^[0-9a-f]{64}  rubik_cube_library-9\\.8\\.7\\.tar\\.gz$" "${checksum_path}"; then
    echo "check_release_archive checksum test failed: invalid checksum format" >&2
    cat "${checksum_path}" >&2
    exit 1
fi

if ! grep -q "release_archive,sha256_path,${checksum_path}" "${tmp_dir}/archive-default.log"; then
    echo "check_release_archive checksum test failed: checksum path was not reported" >&2
    cat "${tmp_dir}/archive-default.log" >&2
    exit 1
fi
