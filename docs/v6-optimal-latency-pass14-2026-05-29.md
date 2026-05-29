# V6 Optimal Latency Pass 14 - Candidate Cheap Bound Fusion

Date: 2026-05-29

## Goal

Check whether fusing candidate coordinate generation with the cheap lower-bound
pruning path improves local `SolveMode::Optimal` latency after pass 12.

## Trial Change

The experiment replaced the normal candidate flow:

1. move all search coordinates;
2. evaluate cheap lower-bound tables;
3. discard the candidate if the bound exceeds the active limit.

with a fused helper that moved coordinates incrementally and checked pruning
tables after each coordinate/table group. The intent was to avoid computing
later candidate coordinates when an early pruning table was already enough to
discard the move.

## Benchmark Command

```sh
scripts/run_v6_tail_baseline.sh \
  --build-dir out/release-native-lto \
  --output-dir out/release-native-lto/benchmark-results/v6-tail-pass14 \
  --cache-dir /tmp/rubik_cube_library_v6_tail_cache \
  --threads 0 \
  --max-memory-mb 4096 \
  --cache-mode reuse
```

## Results

Corpus: V6 tail baseline, 43 optimal cases, warm table cache.

| Metric | Pass 12 | Candidate-bound fusion trial |
| --- | ---: | ---: |
| Solved | 43 / 43 | 43 / 43 |
| Total solver elapsed | 28861 ms | 29091 ms |
| Total nodes expanded | 169155502 | 169136834 |
| p50 solver elapsed | 307 ms | 317 ms |
| p90 solver elapsed | 1742 ms | 1755 ms |
| p95 solver elapsed | 1835 ms | 1842 ms |
| p99 solver elapsed | 3645 ms | 3653 ms |
| Max solver elapsed | 3749 ms | 3713 ms |
| Max wall elapsed | 4506 ms | 4441 ms |

## Decision

Rejected. The trial reduced node count slightly and improved max solver/wall
latency, but it regressed total solver time and the p50, p90, p95, and p99
latency bands. The fused code also duplicated a large section of lower-bound
logic, increasing maintenance risk without a strong enough measured gain.

Pass 12 remains the accepted implementation. The source change was reverted and
only this investigation note is retained.

## Verification

- `cmake --build build --target rubik_tests rubik-bench -j$(nproc)`
- `ctest --test-dir build --output-on-failure -R "rubik_tests|optimal|validation"`
- `cmake --build out/release-native-lto --target rubik_tests rubik-bench rubik-cache-setup -j$(nproc)`
- `ctest --test-dir out/release-native-lto --output-on-failure -R "rubik_tests|optimal|validation"`
- `scripts/run_v6_tail_baseline.sh --build-dir out/release-native-lto --output-dir out/release-native-lto/benchmark-results/v6-tail-pass14 --cache-dir /tmp/rubik_cube_library_v6_tail_cache --threads 0 --max-memory-mb 4096 --cache-mode reuse`
