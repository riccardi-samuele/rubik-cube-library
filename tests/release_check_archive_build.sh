#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "${tmp_dir}"' EXIT

log_file="${tmp_dir}/commands.log"
fake_bin="${tmp_dir}/bin"
mkdir -p "${fake_bin}"

cat > "${fake_bin}/cmake" <<'SCRIPT'
#!/usr/bin/env bash
set -euo pipefail
printf 'cmake'
printf ' %q' "$@"
printf '\n'
SCRIPT

cat > "${fake_bin}/ctest" <<'SCRIPT'
#!/usr/bin/env bash
set -euo pipefail
printf 'ctest'
printf ' %q' "$@"
printf '\n'
SCRIPT

chmod +x "${fake_bin}/cmake" "${fake_bin}/ctest"

PATH="${fake_bin}:${PATH}" \
    "${repo_root}/scripts/release_check.sh" --profile quick > "${log_file}"

project_version="$(
    sed -nE 's/^[[:space:]]*project\([^)]*VERSION[[:space:]]+([^[:space:])]+).*/\1/p' \
        "${repo_root}/CMakeLists.txt" | head -n 1
)"
escaped_project_version="${project_version//./\\.}"

if ! grep -Eq "cmake -S .*rubik_cube_library-${escaped_project_version} -B .*archive-build" "${log_file}"; then
    echo "release_check archive-build test failed: missing extracted archive configure step" >&2
    cat "${log_file}" >&2
    exit 1
fi

if ! grep -Eq "ctest --test-dir .*archive-build --output-on-failure" "${log_file}"; then
    echo "release_check archive-build test failed: missing extracted archive ctest step" >&2
    cat "${log_file}" >&2
    exit 1
fi

version_repo="${tmp_dir}/version-repo"
mkdir -p "${version_repo}/scripts" "${version_repo}/dist"
cp "${repo_root}/scripts/release_check.sh" "${version_repo}/scripts/release_check.sh"
chmod +x "${version_repo}/scripts/release_check.sh"

cat > "${version_repo}/CMakeLists.txt" <<'CMAKE'
cmake_minimum_required(VERSION 3.20)
project(rubik_cube_library VERSION 9.8.7 LANGUAGES CXX)
CMAKE

cat > "${version_repo}/scripts/check_release_archive.sh" <<'SCRIPT'
#!/usr/bin/env bash
set -euo pipefail

version=""
output_dir=""
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
        *)
            shift
            ;;
    esac
done

if [[ -z "${version}" ]]; then
    version="$(
        sed -nE 's/^[[:space:]]*project\([^)]*VERSION[[:space:]]+([^[:space:])]+).*/\1/p' \
            CMakeLists.txt | head -n 1
    )"
fi

printf 'check_release_archive effective-version %s --output-dir %s\n' "${version}" "${output_dir}"
archive_root="rubik_cube_library-${version}"
mkdir -p "${output_dir}" "${archive_root}"
printf 'cmake_minimum_required(VERSION 3.20)\nproject(rubik_cube_library VERSION %s LANGUAGES CXX)\n' "${version}" > "${archive_root}/CMakeLists.txt"
tar -czf "${output_dir}/${archive_root}.tar.gz" "${archive_root}"
rm -rf "${archive_root}"
SCRIPT
chmod +x "${version_repo}/scripts/check_release_archive.sh"

version_log="${tmp_dir}/version.log"
PATH="${fake_bin}:${PATH}" \
    "${version_repo}/scripts/release_check.sh" --profile quick > "${version_log}"

if ! grep -q "check_release_archive effective-version 9.8.7" "${version_log}"; then
    echo "release_check archive-build test failed: release version was not read from CMakeLists.txt" >&2
    cat "${version_log}" >&2
    exit 1
fi

if ! grep -Eq "cmake -S .*rubik_cube_library-9\\.8\\.7 -B .*archive-build" "${version_log}"; then
    echo "release_check archive-build test failed: archive root did not use CMake project version" >&2
    cat "${version_log}" >&2
    exit 1
fi
