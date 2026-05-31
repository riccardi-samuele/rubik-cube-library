#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/run_v6_conservative_root_targeted_corpus.sh}"

"${script}" --build-dir > /tmp/run_v6_conservative_root_targeted_corpus_missing_value.out 2>&1 && {
    cat /tmp/run_v6_conservative_root_targeted_corpus_missing_value.out >&2
    exit 1
}

grep -q "Usage: scripts/run_v6_conservative_root_targeted_corpus.sh" /tmp/run_v6_conservative_root_targeted_corpus_missing_value.out
