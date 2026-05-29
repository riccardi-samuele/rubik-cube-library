# V6 Optimal Latency Pass 17 - LB8 Split-Depth 3 Trial

Date: 2026-05-29

## Goal

Check whether the slow `lb8_stable_mid_strong_min` tail pattern benefits from a
deeper adaptive deep-split scheduler.

## Trial Change

The experiment changed only the `lb8_stable_mid_strong_min` subcase with
`strong_min_count == 7` from split depth 2 to split depth 3.

The trial produced `split_depth=3` and `split_tasks=3240` for the targeted
`random_987654321_1` depth-15 cases. All other adaptive deep-split groups kept
the existing depth-2 scheduler.

## Benchmark Command

```sh
scripts/run_v6_tail_baseline.sh \
  --build-dir out/release-native-lto \
  --output-dir out/release-native-lto/benchmark-results/v6-tail-pass17 \
  --cache-dir /tmp/rubik_cube_library_v6_tail_cache \
  --threads 0 \
  --max-memory-mb 4096 \
  --cache-mode reuse
```

## Results

Corpus: V6 tail baseline, 43 optimal cases, warm table cache.

| Metric | Pass 12 | Split-depth 3 trial |
| --- | ---: | ---: |
| Solved | 43 / 43 | 43 / 43 |
| Total solver elapsed | 28861 ms | 29735 ms |
| Total nodes expanded | 169155502 | 170836048 |
| p50 solver elapsed | 307 ms | 318 ms |
| p90 solver elapsed | 1742 ms | 1760 ms |
| p95 solver elapsed | 1835 ms | 1903 ms |
| p99 solver elapsed | 3645 ms | 3643 ms |
| Max solver elapsed | 3749 ms | 3933 ms |
| Max wall elapsed | 4506 ms | 4592 ms |

The targeted slow group did create more scheduling work:

| Case | Trial solver elapsed | Trial nodes | Adaptive reason | Split depth |
| --- | ---: | ---: | --- | ---: |
| `random_987654321_1` | 3933 ms | 24652947 | `lb8_stable_mid_strong_min` | 3 |
| `random_987654321_1` | 3643 ms | 24647817 | `lb8_stable_mid_strong_min` | 3 |

## Decision

Rejected. The trial solved every case, but it regressed total solver time,
node count, p50, p90, p95, max solver latency, and max wall latency. The tiny
`2 ms` p99 improvement is not enough to justify the slower total and worse max
latency.

Pass 12 remains the accepted solver implementation. The source change was
reverted and only this investigation note is retained.

## Verification

- `cmake --build out/release-native-lto --target rubik_tests rubik-bench rubik-cache-setup -j$(nproc)`
- `ctest --test-dir out/release-native-lto -R "rubik_tests|cli_bench_reports_root_ordering_profile|cli_bench_optimal_diagnostics_smoke" --output-on-failure`
- `scripts/run_v6_tail_baseline.sh --build-dir out/release-native-lto --output-dir out/release-native-lto/benchmark-results/v6-tail-pass17 --cache-dir /tmp/rubik_cube_library_v6_tail_cache --threads 0 --max-memory-mb 4096 --cache-mode reuse`
- `ctest --test-dir out/release-native-lto --output-on-failure`
- `git diff --check`
