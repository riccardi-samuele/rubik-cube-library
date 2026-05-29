# V6 Optimal Latency Pass 13 - Lazy Index Investigation

Date: 2026-05-29

## Goal

Check whether delaying combined-index arithmetic inside lower-bound evaluation
improves local `SolveMode::Optimal` latency after pass 12.

## Trial Change

The experiment moved several combined-index calculations closer to the table
lookups that use them. The intent was to avoid computing indexes for tables that
would not be reached after an earlier pruning-table hit exceeded the active
threshold.

## Benchmark Command

```sh
scripts/run_v6_tail_baseline.sh \
  --build-dir out/release-native-lto \
  --output-dir out/release-native-lto/benchmark-results/v6-tail-pass13 \
  --cache-dir /tmp/rubik_cube_library_v6_tail_cache \
  --threads 0 \
  --max-memory-mb 4096 \
  --cache-mode reuse
```

## Results

Corpus: V6 tail baseline, 43 optimal cases, warm table cache.

| Metric | Pass 12 | Lazy-index trial |
| --- | ---: | ---: |
| Solved | 43 / 43 | 43 / 43 |
| Total solver elapsed | 28861 ms | 28974 ms |
| Total nodes expanded | 169155502 | 169283164 |
| p50 solver elapsed | 307 ms | 312 ms |
| p90 solver elapsed | 1742 ms | 1755 ms |
| p95 solver elapsed | 1835 ms | 1842 ms |
| p99 solver elapsed | 3645 ms | 3622 ms |
| Max solver elapsed | 3749 ms | 3738 ms |
| Max wall elapsed | 4506 ms | 4489 ms |

## Decision

Rejected. The trial slightly improved p99 and max latency, but regressed total
solver time, node count, p50, p90, and p95. Because pass 12 is stronger overall,
the source change was reverted and only this investigation note is retained.

## Verification

- `cmake --build build --target rubik_tests rubik-bench -j$(nproc)`
- `ctest --test-dir build --output-on-failure -R "rubik_tests|optimal|validation"`
- `cmake --build out/release-native-lto --target rubik_tests rubik-bench rubik-cache-setup -j$(nproc)`
- `ctest --test-dir out/release-native-lto --output-on-failure -R "rubik_tests|optimal|validation"`
- `scripts/run_v6_tail_baseline.sh --build-dir out/release-native-lto --output-dir out/release-native-lto/benchmark-results/v6-tail-pass13 --cache-dir /tmp/rubik_cube_library_v6_tail_cache --threads 0 --max-memory-mb 4096 --cache-mode reuse`
