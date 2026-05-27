# Optimal Root Ordering Experiments - 2026-05-27

This benchmark compares experimental root ordering strategies for optimal Auto solves on known depth-15 tail cases.

The experiment used the existing large-local cache, `profile=auto`, `max-depth=15`, `timeout-ms=30000`, `max-memory-mb=2048`, and automatic thread selection.

## Variants

- `default`: current Auto root ordering.
- `reverse_tie`: current lower-bound ordering with reversed deterministic move tie-breaks.
- `phase2_tiebreak`: current lower-bound ordering with phase-2 ordering data available at the root.

## Results

| Seed | Default ms | Reverse Tie ms | Phase2 Tie-Break ms | Winner |
|---:|---:|---:|---:|---|
| 99 | 2048 | 2265 | 2042 | phase2_tiebreak |
| 555 | 3030 | 2906 | 3029 | reverse_tie |
| 666 | 1014 | 1181 | 1023 | default |
| 888 | 1951 | 1860 | 1885 | reverse_tie |
| 1009 | 10020 | 10221 | 10145 | default |
| 2016 | 5696 | 5720 | 5609 | phase2_tiebreak |
| 424242 | 3094 | 3187 | 3108 | default |
| 987654321 | 7584 | 7811 | 7599 | default |

## Decision

No experimental root ordering strategy is promoted to the default Auto policy.

The current default remains the best choice for the worst measured case in this set, seed `1009`, and also wins the largest single group of cases. The alternative strategies are useful as diagnostic tools, but their wins are mixed and do not justify changing production behavior.

The experimental selector remains available for local benchmark work through `RUBIK_EXPERIMENTAL_ROOT_ORDERING`.
