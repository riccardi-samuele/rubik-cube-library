# V6 Optimal Latency Pass 10 - Deferred Candidate Cubes

Date: 2026-05-29

## Goal

Reduce local `SolveMode::Optimal` latency by avoiding full cube move generation
for candidates that are rejected by coordinate-only lower bounds.

## Change

Candidate generation now creates a lightweight coordinate payload first. The
cheap lower bound and optional three-phase phase-1 lower bound run on that
coordinate payload. The full `CubieCube` child is only created after the
candidate survives those pruning checks.

This keeps the same optimal search contract while removing full cubie move work
from candidates that cannot be part of the current depth-limited solution.

## Benchmark Command

```sh
scripts/run_v6_tail_baseline.sh \
  --build-dir out/release-native-lto \
  --output-dir out/release-native-lto/benchmark-results/v6-tail-pass10 \
  --cache-dir /tmp/rubik_cube_library_v6_tail_cache \
  --threads 0 \
  --max-memory-mb 4096 \
  --cache-mode reuse
```

## Results

Corpus: V6 tail baseline, 43 optimal cases, warm table cache.

| Metric | Pass 9 | Pass 10 |
| --- | ---: | ---: |
| Solved | 43 / 43 | 43 / 43 |
| Total solver elapsed | 33528 ms | 30480 ms |
| Total nodes expanded | 169472765 | 169485581 |
| p50 solver elapsed | 367 ms | 328 ms |
| p90 solver elapsed | 2037 ms | 1849 ms |
| p95 solver elapsed | 2154 ms | 1947 ms |
| p99 solver elapsed | 4216 ms | 3846 ms |
| Max solver elapsed | 4327 ms | 3997 ms |
| Max wall elapsed | 7078 ms | 4750 ms |

This pass reduces measured solver time by `3048 ms` on the corpus versus pass 9,
about `9.1%`, while preserving the same solved count and optimal mode.

## Verification

- `cmake --build build --target rubik_tests rubik-bench -j$(nproc)`
- `ctest --test-dir build --output-on-failure -R "rubik_tests|optimal|validation"`
- `cmake --build out/release-native-lto --target rubik_tests rubik-bench rubik-cache-setup -j$(nproc)`
- `ctest --test-dir out/release-native-lto --output-on-failure -R "rubik_tests|optimal|validation"`
- `scripts/run_v6_tail_baseline.sh --build-dir out/release-native-lto --output-dir out/release-native-lto/benchmark-results/v6-tail-pass10 --cache-dir /tmp/rubik_cube_library_v6_tail_cache --threads 0 --max-memory-mb 4096 --cache-mode reuse`
- `ctest --test-dir out/release-native-lto --output-on-failure`
- `git diff --check`
