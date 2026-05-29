# V6 optimal latency pass 34

## Goal

Check whether `high_bound_first` can help the remaining
`lb9_low_strong_min` tail cases after Pass 33 confirmed the lb8 improvement.
This pass is a measurement pass only; it does not change solver behavior.

## Probe 1: seed 555 root ordering

Command:

```bash
scripts/benchmark_root_ordering_experiments.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --cache-mode reuse \
  --output-dir out/release-native-lto/benchmark-results/v6-pass34-lb9-low-root-ordering-555 \
  --seeds 555 \
  --variants default,reverse_tie,high_bound_first,phase2_tiebreak \
  --timeout-ms 30000 \
  --max-depth 15 \
  --random-depth 15 \
  --threads 0 \
  --max-memory-mb 2048
```

Result:

| Ordering | Solver ms | Nodes | Solution rank |
| --- | ---: | ---: | ---: |
| `default` | 1789 | 11616395 | 7 |
| `reverse_tie` | 2967 | 19423721 | 13 |
| `high_bound_first` | 1571 | 10108156 | 6 |
| `phase2_tiebreak` | 1814 | 11635585 | 7 |

The single seed favors `high_bound_first`, so the next check expanded the probe
to the measured `lb9_low_strong_min` subset from the V6 corpus.

## Probe 2: measured lb9-low subset

Default run:

```bash
scripts/run_v6_tail_baseline.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --cache-mode reuse \
  --output-dir out/release-native-lto/benchmark-results/v6-pass34-lb9-low-subset-default \
  --tail-seeds 555 \
  --hardening-seeds 12345,123456789,271828,99 \
  --deep-opt14-count 2 \
  --deep-opt15-count 1 \
  --threads 0 \
  --max-memory-mb 2048 \
  --timeout-ms 30000
```

High-bound run:

```bash
RUBIK_EXPERIMENTAL_ROOT_ORDERING=high_bound_first scripts/run_v6_tail_baseline.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --cache-mode reuse \
  --output-dir out/release-native-lto/benchmark-results/v6-pass34-lb9-low-subset-high-bound \
  --tail-seeds 555 \
  --hardening-seeds 12345,123456789,271828,99 \
  --deep-opt14-count 2 \
  --deep-opt15-count 1 \
  --threads 0 \
  --max-memory-mb 2048 \
  --timeout-ms 30000
```

Comparison:

```bash
scripts/compare_v6_latency.py \
  --baseline-dir out/release-native-lto/benchmark-results/v6-pass34-lb9-low-subset-default \
  --candidate-dir out/release-native-lto/benchmark-results/v6-pass34-lb9-low-subset-high-bound \
  --group-by-reason \
  --sort-by elapsed_delta_ms \
  --sort-desc \
  --output out/release-native-lto/benchmark-results/v6-pass34-compare-lb9-low-subset-high-bound-reason-groups.csv
```

Grouped result:

| Reason | Default ms | High-bound ms | Delta ms | Winner |
| --- | ---: | ---: | ---: | --- |
| `depth14_conservative_root` | 661 | 1126 | +465 | Default |
| `lb9_low_strong_min` | 3286 | 3732 | +446 | Default |
| `conservative_root` | 1266 | 1275 | +9 | Default |
| `lb9_mid_strong_min` | 286 | 180 | -106 | High-bound |
| `remaining_depth_lt_5` | 436 | 271 | -165 | High-bound |
| Summary | 5935 | 6584 | +649 | Default |

Important rows:

| Case | Default ms | High-bound ms | Delta ms |
| --- | ---: | ---: | ---: |
| tail depth 15 seed 555 | 1813 | 1625 | -188 |
| hardening depth 15 seed 12345 | 1191 | 1801 | +610 |
| hardening depth 15 seed 123456789 | 197 | 234 | +37 |
| hardening depth 14 seed 99 row 2 | 19 | 6 | -13 |
| hardening depth 14 seed 271828 row 1 | 66 | 66 | 0 |

## Decision

Reject promoting `high_bound_first` for `lb9_low_strong_min`. The seed 555
improvement is real in this run, but the broader measured subset regresses by
446 ms inside the target bucket and by 649 ms overall. The large regression on
hardening depth 15 seed 12345 is enough to block the policy.

The next V6 pass should look for a stronger discriminator inside
`lb9_low_strong_min` or move back to the remaining `conservative_root` tail
cases. A broad high-bound rule is not acceptable for this bucket.
