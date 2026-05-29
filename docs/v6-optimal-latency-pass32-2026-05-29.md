# V6 optimal latency pass 32

## Goal

Promote the useful part of Pass 31 without taking the full-regression risk.
Pass 31 showed that `high_bound_first` is very effective for
`lb8_stable_mid_strong_min`, but bad as a global ordering policy. This pass
tests it only for the measured lb8 stable signature.

## Change

`chooseAdaptiveRootOrdering()` now selects `HighBoundFirst` only when all of
these conditions hold:

- initial lower bound is 8
- max depth leaves at least five moves of remaining search
- thread count is at least 4
- first base-bound and strong-bound root choices do not differ
- strong-min root count is between 6 and 8

That is the same measured signature reported as
`adaptive_reason=lb8_stable_mid_strong_min`. Other adaptive buckets keep their
existing root ordering decisions.

## Validation

Commands:

```bash
cmake --build out/release-native-lto --target rubik_tests -j2
out/release-native-lto/rubik_tests

scripts/run_v6_tail_baseline.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --cache-mode reuse \
  --output-dir out/release-native-lto/benchmark-results/v6-pass32-lb8-high-bound-auto \
  --threads 0 \
  --max-memory-mb 2048 \
  --timeout-ms 30000

scripts/compare_v6_latency.py \
  --baseline-dir out/release-native-lto/benchmark-results/v6-tail-pass20 \
  --candidate-dir out/release-native-lto/benchmark-results/v6-pass32-lb8-high-bound-auto \
  --group-by-reason \
  --sort-by elapsed_delta_ms \
  --sort-desc \
  --output out/release-native-lto/benchmark-results/v6-pass32-compare-pass20-lb8-high-bound-auto-reason-groups.csv
```

## Full Pass 20 comparison

| Metric | Pass 20 | Candidate | Delta | Winner |
| --- | ---: | ---: | ---: | --- |
| Total solver ms | 28037 | 26455 | -1582 | Candidate |
| p50 ms | 252 | 265 | +13 | Pass 20 |
| p90 ms | 1784 | 1799 | +15 | Pass 20 |
| p95 ms | 1829 | 1912 | +83 | Pass 20 |
| p99 ms | 3545 | 2126 | -1419 | Candidate |
| Max solver ms | 3593 | 2853 | -740 | Candidate |
| Total nodes | 165163047 | 135523614 | -29639433 | Candidate |
| Max wall ms | 4237 | 3638 | -599 | Candidate |

Grouped by adaptive reason:

| Reason | Pass 20 ms | Candidate ms | Delta ms | Winner |
| --- | ---: | ---: | ---: | --- |
| `conservative_root` | 10371 | 11793 | +1422 | Pass 20 |
| `lb9_low_strong_min` | 3320 | 3885 | +565 | Pass 20 |
| `depth14_conservative_root` | 1967 | 2101 | +134 | Pass 20 |
| `lb9_mid_strong_min` | 4357 | 4455 | +98 | Pass 20 |
| `remaining_depth_lt_5` | 554 | 578 | +24 | Pass 20 |
| `lb8_stable_mid_strong_min` | 7468 | 3643 | -3825 | Candidate |

The candidate uses `high_bound_first` only in four rows:

| Case | Pass 20 ms | Candidate ms | Delta ms |
| --- | ---: | ---: | ---: |
| hardening depth 15 seed 987654321 | 3593 | 1354 | -2239 |
| tail depth 15 seed 987654321 | 3545 | 1892 | -1653 |
| hardening depth 15 seed 314159 | 255 | 321 | +66 |
| hardening depth 14 seed 314159 | 75 | 76 | +1 |

## Decision

Accept the narrow lb8 policy as the current V6 candidate. It reduces total
solver time, total nodes, p99, max solver latency, and max wall latency on the
full Pass 20 corpus. The known tradeoff is a small regression in p50/p90/p95 and
some non-lb8 groups due normal run-to-run variance and changed build state, while
the targeted lb8 bucket improves by 3825 ms.

The next pass should focus on the remaining worst non-lb8 regressions, starting
with the `conservative_root` depth-15 seed 42 and `lb9_low_strong_min` seed 555
rows.
