#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "${tmp_dir}"' EXIT

test_repo="${tmp_dir}/repo"
fake_bin="${tmp_dir}/bin"
log_file="${tmp_dir}/commands.log"

mkdir -p "${test_repo}/scripts" "${test_repo}/dist" "${fake_bin}"

cp "${repo_root}/scripts/release_check.sh" "${test_repo}/scripts/release_check.sh"
chmod +x "${test_repo}/scripts/release_check.sh"

cat > "${test_repo}/CMakeLists.txt" <<'CMAKE'
cmake_minimum_required(VERSION 3.20)
project(rubik_cube_library VERSION 9.8.7 LANGUAGES CXX)
CMAKE

cat > "${test_repo}/scripts/check_release_archive.sh" <<'SCRIPT'
#!/usr/bin/env bash
set -euo pipefail

output_dir=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --output-dir)
            output_dir="$2"
            shift 2
            ;;
        *)
            shift
            ;;
    esac
done

version="$(
    sed -nE 's/^[[:space:]]*project\([^)]*VERSION[[:space:]]+([^[:space:])]+).*/\1/p' \
        CMakeLists.txt | head -n 1
)"
archive_root="rubik_cube_library-${version}"
mkdir -p "${output_dir}" "${archive_root}"
printf 'cmake_minimum_required(VERSION 3.20)\nproject(rubik_cube_library VERSION %s LANGUAGES CXX)\n' "${version}" > "${archive_root}/CMakeLists.txt"
tar -czf "${output_dir}/${archive_root}.tar.gz" "${archive_root}"
rm -rf "${archive_root}"
SCRIPT
chmod +x "${test_repo}/scripts/check_release_archive.sh"

cat > "${fake_bin}/cmake" <<'SCRIPT'
#!/usr/bin/env bash
set -euo pipefail
printf 'cmake' >> "${RELEASE_CHECK_ROUTE_LOG}"
printf ' %q' "$@" >> "${RELEASE_CHECK_ROUTE_LOG}"
printf '\n' >> "${RELEASE_CHECK_ROUTE_LOG}"
SCRIPT

cat > "${fake_bin}/ctest" <<'SCRIPT'
#!/usr/bin/env bash
set -euo pipefail
printf 'ctest' >> "${RELEASE_CHECK_ROUTE_LOG}"
printf ' %q' "$@" >> "${RELEASE_CHECK_ROUTE_LOG}"
printf '\n' >> "${RELEASE_CHECK_ROUTE_LOG}"
SCRIPT

chmod +x "${fake_bin}/cmake" "${fake_bin}/ctest"

RELEASE_CHECK_ROUTE_LOG="${log_file}" \
PATH="${fake_bin}:${PATH}" \
    "${test_repo}/scripts/release_check.sh" --profile quick --with-v3-auto > /dev/null

if ! grep -q -- "--target rubik-benchmark-v3-auto-gates" "${log_file}"; then
    echo "release_check v3-auto route test failed: missing V3 Auto gate target" >&2
    cat "${log_file}" >&2
    exit 1
fi

for target in \
    rubik-benchmark-profile-realistic \
    rubik-benchmark-embedded-multiseed \
    rubik-benchmark-optimal-stress \
    rubik-benchmark-optimal-large-local \
    rubik-benchmark-optimal-large-local-tail-8threads
do
    if grep -q -- "--target ${target}" "${log_file}"; then
        echo "release_check v3-auto route test failed: unexpected target ${target}" >&2
        cat "${log_file}" >&2
        exit 1
    fi
done
