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
    "${repo_root}/scripts/release_check.sh" --profile quick --with-large-local > /dev/null

for target in \
    rubik-benchmark-optimal-auto-tail \
    rubik-benchmark-optimal-auto-tail-gates \
    rubik-benchmark-optimal-auto-hardening \
    rubik-benchmark-optimal-auto-hardening-gates
do
    if ! grep -q -- "--target ${target}" "${log_file}"; then
        echo "release_check large-local route test failed: missing target ${target}" >&2
        cat "${log_file}" >&2
        exit 1
    fi
done
