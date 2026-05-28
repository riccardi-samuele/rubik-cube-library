# V6 Optimal Latency Pass 4 - 2026-05-28

This document records the fourth V6 local optimal-latency optimization pass.
The solver contract is unchanged: `SolveStatus::Optimal` still means a proven
minimum-length HTM solution for the requested options.

## Change

Candidate generation now reuses the base lower-bound value that is already
computed as part of the full lower-bound check. Before this pass, base-bound
ordering recalculated `nodeBaseLowerBound()` for every accepted candidate even
though `nodeLowerBoundWithoutThreePhase1()` had just computed the same value.

The change is internal only:

- no public API changes;
- no solver status changes;
- no optimality contract changes;
- same root ordering policy as pass 2.

## Targeted Tail Check

The target case was run three times after the change:

| Case | Run 1 ms | Run 2 ms | Run 3 ms | Average ms |
| --- | ---: | ---: | ---: | ---: |
| `random_seed_987654321_depth_15_count_1` | 6328 | 6418 | 6426 | 6391 |

The previous pass 3 base-bound repeat average for this same case was 6581 ms.

## Corpus Verification

Benchmark command:

```sh
scripts/run_v6_tail_baseline.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-pass4-optimized-tail-baseline \
  --tail-seeds 987654321,424242,1009,666,555,99,888 \
  --hardening-seeds 12345,20260525,42,314159,271828,987654321,7,99,123456789,424242,8675309,20240525 \
  --timeout-ms 30000 \
  --threads 0 \
  --max-memory-mb 2048 \
  --deep-opt14-count 2 \
  --deep-opt15-count 1 \
  --cache-mode reuse
```

These are local desktop measurements only. No external hardware, GPU, or cloud
latency claims are included.

## Corpus Comparison

Baseline source: `docs/v6-optimal-latency-pass2-2026-05-28.md`.

| Metric | Pass 2 | Pass 4 | Delta |
| --- | ---: | ---: | ---: |
| Cases | 43 | 43 | 0 |
| Solved | 43 | 43 | 0 |
| Failed | 0 | 0 | 0 |
| Total solver ms | 52030 | 48972 | -3058 |
| Total nodes | 169550565 | 169037604 | -512961 |
| p50 solver ms | 539 | 523 | -16 |
| p90 solver ms | 3018 | 3005 | -13 |
| p95 solver ms | 3591 | 3129 | -462 |
| p99 solver ms | 6748 | 6401 | -347 |
| Max solver ms | 6963 | 6437 | -526 |
| Max wall ms | 7638 | 7078 | -560 |

## Suite Results

| Suite | Cases | Solved | Failed | Total solver ms | Total nodes | Max solver ms | Max wall ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| optimal-auto-tail | 7 | 7 | 0 | 20475 | 71686115 | 6401 | 7038 |
| optimal-auto-hardening | 36 | 36 | 0 | 28497 | 97351489 | 6437 | 7078 |
| combined | 43 | 43 | 0 | 48972 | 169037604 | 6437 | 7078 |

## Slowest Rows After Pass 4

| Suite | Benchmark | Solver ms | Wall ms | Nodes |
| --- | --- | ---: | ---: | ---: |
| optimal-auto-hardening | `random_seed_987654321_depth_15_count_1` | 6437 | 7078 | 23893073 |
| optimal-auto-tail | `random_seed_987654321_depth_15_count_1` | 6401 | 7038 | 23959716 |
| optimal-auto-hardening | `random_seed_42_depth_15_count_1` | 3833 | 4477 | 13191128 |
| optimal-auto-tail | `random_seed_555_depth_15_count_1` | 3129 | 3779 | 11585106 |
| optimal-auto-tail | `random_seed_1009_depth_15_count_1` | 3125 | 3763 | 11322022 |
| optimal-auto-tail | `random_seed_424242_depth_15_count_1` | 3005 | 3648 | 10236547 |
| optimal-auto-hardening | `random_seed_424242_depth_15_count_1` | 2997 | 3651 | 10146739 |
| optimal-auto-hardening | `random_seed_8675309_depth_15_count_1` | 2484 | 3130 | 9330404 |

## Reading

Pass 4 improves the measured local tail without changing solver policy. The
largest benefit appears in base-bound ordered searches, where the solver now
avoids recomputing the base lower bound for accepted candidates.
