#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/run_v4_tail_corpus.sh}"

"${script}" --cases-file > /tmp/run_v4_tail_corpus_missing_value.out 2>&1 && {
    cat /tmp/run_v4_tail_corpus_missing_value.out >&2
    exit 1
}

grep -q "Usage: scripts/run_v4_tail_corpus.sh" /tmp/run_v4_tail_corpus_missing_value.out
