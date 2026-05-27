#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

"${repo_root}/tests/expect_missing_value_error.sh" \
    "${repo_root}/scripts/check_install_consumer.sh" \
    "Usage: scripts/check_install_consumer.sh" \
    --build-dir \
    --install-prefix \
    --consumer-build-dir
