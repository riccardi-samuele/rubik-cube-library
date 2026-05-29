# V6 optimal latency pass 31

## Goal

Test a narrow root-ordering hypothesis from Pass 30: the worst
`lb8_stable_mid_strong_min` cases spend most work on failed roots before the
solution root, so trying higher base-bound roots first might move the solution
root earlier.

## Change

Added an experimental root ordering mode:

```bash
RUBIK_EXPERIMENTAL_ROOT_ORDERING=high_bound_first
```

The mode sorts root candidates by descending root order bound, preserving normal
move order for ties. It is available only through the existing experimental
environment-variable path. It is not promoted to the adaptive default policy.

`scripts/benchmark_root_ordering_experiments.sh` also accepts
`high_bound_first` as a benchmark variant.

## Target A/B result

Command:

```bash
scripts/benchmark_root_ordering_experiments.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --cache-mode reuse \
  --output-dir out/release-native-lto/benchmark-results/v6-pass31-root-ordering-987 \
  --seeds 987654321 \
  --variants default,high_bound_first \
  --threads 16 \
  --max-memory-mb 2048 \
  --timeout-ms 30000 \
  --max-depth 15 \
  --random-depth 15
```

| Variant | Solver ms | Nodes | Solution rank | Root ordering |
| --- | ---: | ---: | ---: | --- |
| default | 3707 | 23953022 | 14 | `default` |
| high_bound_first | 1344 | 8792496 | 4 | `high_bound_first` |

The hypothesis works on the target seed: it moves the solution root earlier and
reduces solver time by 2363 ms.

## Full Pass 20 corpus result

Command:

```bash
RUBIK_EXPERIMENTAL_ROOT_ORDERING=high_bound_first \
scripts/run_v6_tail_baseline.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --cache-mode reuse \
  --output-dir out/release-native-lto/benchmark-results/v6-pass31-high-bound-full \
  --threads 0 \
  --max-memory-mb 2048 \
  --timeout-ms 30000

scripts/compare_v6_latency.py \
  --baseline-dir out/release-native-lto/benchmark-results/v6-tail-pass20 \
  --candidate-dir out/release-native-lto/benchmark-results/v6-pass31-high-bound-full \
  --group-by-reason \
  --sort-by elapsed_delta_ms \
  --sort-desc \
  --output out/release-native-lto/benchmark-results/v6-pass31-compare-pass20-high-bound-reason-groups.csv
```

Summary:

| Metric | Pass 20 | High-bound-first | Delta | Winner |
| --- | ---: | ---: | ---: | --- |
| Total solver ms | 28037 | 34211 | +6174 | Pass 20 |
| p50 ms | 252 | 242 | -10 | Candidate |
| p90 ms | 1784 | 1683 | -101 | Candidate |
| p95 ms | 1829 | 3951 | +2122 | Pass 20 |
| p99 ms | 3545 | 4486 | +941 | Pass 20 |
| Max solver ms | 3593 | 4590 | +997 | Pass 20 |
| Total nodes | 165163047 | 200599553 | +35436506 | Pass 20 |

Grouped result:

| Reason | Pass 20 ms | Candidate ms | Delta ms | Winner |
| --- | ---: | ---: | ---: | --- |
| `conservative_root` | 10371 | 15647 | +5276 | Pass 20 |
| `lb9_mid_strong_min` | 4357 | 9069 | +4712 | Pass 20 |
| `depth14_conservative_root` | 1967 | 2483 | +516 | Pass 20 |
| `lb9_low_strong_min` | 3320 | 3565 | +245 | Pass 20 |
| `remaining_depth_lt_5` | 554 | 367 | -187 | Candidate |
| `lb8_stable_mid_strong_min` | 7468 | 3080 | -4388 | Candidate |

## Decision

Do not promote `high_bound_first` as the adaptive default. It is excellent for
the target `lb8_stable_mid_strong_min` bucket, but it creates larger regressions
in `conservative_root` and `lb9_mid_strong_min`, increasing total latency and
tail latency on the full corpus.

Keep the mode as an experimental benchmark control. The next candidate should
gate high-bound-first much more narrowly, likely only when the adaptive reason is
`lb8_stable_mid_strong_min`, then rerun the full corpus before accepting it.
