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

if ! grep -Eq "cmake -S .*rubik_cube_library-1\\.0\\.0 -B .*archive-build" "${log_file}"; then
    echo "release_check archive-build test failed: missing extracted archive configure step" >&2
    cat "${log_file}" >&2
    exit 1
fi

if ! grep -Eq "ctest --test-dir .*archive-build --output-on-failure" "${log_file}"; then
    echo "release_check archive-build test failed: missing extracted archive ctest step" >&2
    cat "${log_file}" >&2
    exit 1
fi
