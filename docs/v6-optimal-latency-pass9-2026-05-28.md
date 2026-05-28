# V6 Optimal Latency Pass 9 - Deferred Extra Phase1 Coordinates

Date: 2026-05-28

## Goal

Reduce optimal-solver latency by avoiding work on candidates that the cheap bound already proves impossible.

## Change

Candidate generation now creates each child without extra phase1-direction coordinates first. It evaluates the cheap bound immediately, and only computes the extra phase1-direction coordinates if the child survives that cheap prune.

This keeps the same search behavior for surviving candidates while avoiding extra move-table work for the large number of candidates pruned before three-phase bounds are needed.

## Benchmark Command

```sh
scripts/run_v6_tail_baseline.sh \
  --build-dir out/release-native-lto \
  --output-dir out/release-native-lto/benchmark-results/v6-tail-pass9 \
  --cache-dir /tmp/rubik_cube_library_v6_tail_cache \
  --threads 0 \
  --max-memory-mb 4096 \
  --cache-mode reuse
```

## Results

Corpus: V6 tail baseline, 43 optimal cases, warm table cache.

| Metric | Pass 8 | Pass 9 |
| --- | ---: | ---: |
| Solved | 43 / 43 | 43 / 43 |
| Total solver elapsed | 35426 ms | 33528 ms |
| Total nodes expanded | 169754968 | 169472765 |
| p50 solver elapsed | 388 ms | 367 ms |
| p90 solver elapsed | 2201 ms | 2037 ms |
| p95 solver elapsed | 2263 ms | 2154 ms |
| p99 solver elapsed | 4471 ms | 4216 ms |
| Max solver elapsed | 4546 ms | 4327 ms |

This pass improves CPU efficiency by delaying extra phase1-direction coordinate updates until they are actually needed.

## Verification

- `cmake --build --preset release-native-lto --target rubik_tests`
- `ctest --test-dir out/release-native-lto -R rubik_tests --output-on-failure`

Additional release checks are run before committing this pass.
