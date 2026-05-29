# V6 Optimal Latency Pass 12 - Pruned Base Bound

Date: 2026-05-29

## Goal

Reduce local `SolveMode::Optimal` latency by stopping base lower-bound table
reads as soon as the bound already exceeds the current pruning threshold.

## Change

`nodeBaseLowerBound` now accepts an optional `pruneAbove` threshold. Calls that
need the exact base bound keep the default behavior. Candidate pruning paths pass
the active threshold, so the base bound can return early when the node is already
known to be impossible at the current depth.

This preserves the optimal search contract because early return only happens
after the bound is already strong enough to prune the candidate.

## Benchmark Command

```sh
scripts/run_v6_tail_baseline.sh \
  --build-dir out/release-native-lto \
  --output-dir out/release-native-lto/benchmark-results/v6-tail-pass12 \
  --cache-dir /tmp/rubik_cube_library_v6_tail_cache \
  --threads 0 \
  --max-memory-mb 4096 \
  --cache-mode reuse
```

## Results

Corpus: V6 tail baseline, 43 optimal cases, warm table cache.

| Metric | Pass 10 | Pass 12 |
| --- | ---: | ---: |
| Solved | 43 / 43 | 43 / 43 |
| Total solver elapsed | 30480 ms | 28861 ms |
| Total nodes expanded | 169485581 | 169155502 |
| p50 solver elapsed | 328 ms | 307 ms |
| p90 solver elapsed | 1849 ms | 1742 ms |
| p95 solver elapsed | 1947 ms | 1835 ms |
| p99 solver elapsed | 3846 ms | 3645 ms |
| Max solver elapsed | 3997 ms | 3749 ms |
| Max wall elapsed | 4750 ms | 4506 ms |

This pass reduces measured solver time by `1619 ms` on the corpus versus pass
10, about `5.3%`, while preserving the same solved count and optimal mode.

## Verification

- `cmake --build build --target rubik_tests rubik-bench -j$(nproc)`
- `ctest --test-dir build --output-on-failure -R "rubik_tests|optimal|validation"`
- `cmake --build out/release-native-lto --target rubik_tests rubik-bench rubik-cache-setup -j$(nproc)`
- `ctest --test-dir out/release-native-lto --output-on-failure -R "rubik_tests|optimal|validation"`
- `scripts/run_v6_tail_baseline.sh --build-dir out/release-native-lto --output-dir out/release-native-lto/benchmark-results/v6-tail-pass12 --cache-dir /tmp/rubik_cube_library_v6_tail_cache --threads 0 --max-memory-mb 4096 --cache-mode reuse`
