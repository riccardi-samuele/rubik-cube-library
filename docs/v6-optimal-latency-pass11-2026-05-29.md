# V6 Optimal Latency Pass 11 - Candidate Sort Investigation

Date: 2026-05-29

## Goal

Check whether replacing `std::sort` on small candidate arrays with a custom
selection sort reduces optimal-search overhead.

## Trial Change

The experiment added a specialized candidate sorter for arrays of at most 18
moves. The intent was to reduce `CandidateMove` swaps because each candidate
contains a `SearchNode`.

## Benchmark Command

```sh
scripts/run_v6_tail_baseline.sh \
  --build-dir out/release-native-lto \
  --output-dir out/release-native-lto/benchmark-results/v6-tail-pass11 \
  --cache-dir /tmp/rubik_cube_library_v6_tail_cache \
  --threads 0 \
  --max-memory-mb 4096 \
  --cache-mode reuse
```

## Results

Corpus: V6 tail baseline, 43 optimal cases, warm table cache.

| Metric | Pass 10 | Sort trial |
| --- | ---: | ---: |
| Solved | 43 / 43 | 43 / 43 |
| Total solver elapsed | 30480 ms | 30645 ms |
| Total nodes expanded | 169485581 | 169314253 |
| p50 solver elapsed | 328 ms | 330 ms |
| p90 solver elapsed | 1849 ms | 1853 ms |
| p95 solver elapsed | 1947 ms | 1941 ms |
| p99 solver elapsed | 3846 ms | 3885 ms |
| Max solver elapsed | 3997 ms | 4110 ms |
| Max wall elapsed | 4750 ms | 4850 ms |

## Decision

Rejected. The trial slightly improved p95 but regressed total solver time, p99,
max solver time, and max wall time. The source change was reverted; only this
investigation note is retained.

## Verification

- `cmake --build build --target rubik_tests rubik-bench -j$(nproc)`
- `ctest --test-dir build --output-on-failure -R "rubik_tests|optimal|validation"`
- `cmake --build out/release-native-lto --target rubik_tests rubik-bench rubik-cache-setup -j$(nproc)`
- `ctest --test-dir out/release-native-lto --output-on-failure -R "rubik_tests|optimal|validation"`
- `scripts/run_v6_tail_baseline.sh --build-dir out/release-native-lto --output-dir out/release-native-lto/benchmark-results/v6-tail-pass11 --cache-dir /tmp/rubik_cube_library_v6_tail_cache --threads 0 --max-memory-mb 4096 --cache-mode reuse`
