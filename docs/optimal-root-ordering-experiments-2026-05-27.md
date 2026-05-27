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

## Warm Cache Root Search Follow-Up

The follow-up run separated cache preparation from timed solves through
`rubik-cache-setup --format csv` and the root-ordering benchmark runner
`--cache-mode warm`. This removes cold table setup from the per-case wall time
and makes the root search profile directly comparable across runs.

Command shape:

```sh
scripts/benchmark_root_ordering_experiments.sh \
  --cache-mode warm \
  --cache-dir /tmp/rubik_cube_library_v3_tail_probe_cache \
  --output-dir out/release-native-lto/benchmark-results/root-ordering-warm-tail-8-variants \
  --seeds 987654321,424242,1009,2016,666,555,99,888 \
  --variants default,reverse_tie,phase2_tiebreak \
  --timeout-ms 30000 \
  --max-depth 15 \
  --random-depth 15
```

Warm cache setup:

- Effective profile: `large-local`
- Threads: `16`
- Payload bytes: `1392639935`
- Cache warm: `true`
- Cache setup elapsed: `651 ms`

Variant summary:

| Variant | Cases | Avg solver ms | Max solver ms | Avg wall ms | Max wall ms | Avg nodes | Avg solution rank |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| default | 8 | 4104.4 | 9433 | 4748.8 | 10081 | 14273274 | 12.50 |
| phase2_tiebreak | 8 | 4111.8 | 9344 | 4757.2 | 9981 | 14196339 | 12.50 |
| reverse_tie | 8 | 4215.4 | 9591 | 4859.5 | 10237 | 14198347 | 9.00 |

Per-seed winners:

| Seed | Default ms | Reverse Tie ms | Phase2 Tie-Break ms | Winner |
| ---: | ---: | ---: | ---: | --- |
| 99 | 2045 | 2245 | 2062 | default |
| 555 | 2884 | 2780 | 2895 | reverse_tie |
| 666 | 970 | 1133 | 962 | phase2_tiebreak |
| 888 | 1903 | 1869 | 1904 | reverse_tie |
| 1009 | 9433 | 9591 | 9344 | phase2_tiebreak |
| 2016 | 5304 | 5371 | 5285 | phase2_tiebreak |
| 424242 | 2928 | 2978 | 2954 | default |
| 987654321 | 7368 | 7756 | 7488 | default |

Decision:

No root-ordering variant is promoted. `phase2_tiebreak` slightly reduces the
worst measured solver time, but it does not improve the average and its gain is
too small to justify adding ordering-table work to the default path.
`reverse_tie` improves solution rank but worsens solver and wall time, which
shows that first-root rank is not the dominant cost on this machine when the
solver uses enough root workers.

## Root Worker Count Check

The same warm-cache tail set was measured with fixed root worker counts to
verify whether automatic thread selection should be adjusted for the current
desktop target.

| Threads | Cases | Avg solver ms | Max solver ms | Avg wall ms | Max wall ms | Avg nodes |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 12 | 8 | 7320.9 | 12228 | 7967.0 | 12863 | 19896054 |
| 16 | 8 | 4104.4 | 9433 | 4748.8 | 10081 | 14273274 |
| 18 | 8 | 4249.8 | 9402 | 4897.8 | 10046 | 15285461 |

Decision:

Keep automatic thread selection at `16` on this desktop run. `12` workers
regresses because not all 18 root children can start immediately, so late
solution roots may wait behind long negative searches. `18` workers starts all
roots immediately but lowers per-thread throughput enough to lose on average.
The remaining tail cost is the depth-first search inside the solution-bearing
root, especially seed `1009`, not cache warmup or root worker starvation.
