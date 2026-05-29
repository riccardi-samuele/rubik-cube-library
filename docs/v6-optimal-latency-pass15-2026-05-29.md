# V6 Optimal Latency Pass 15 - LB8 Deep-Split Policy Check

Date: 2026-05-29

## Goal

Check whether the adaptive scheduler should keep using deep root splitting for
the `lb8_stable_mid_strong_min` pattern in local `SolveMode::Optimal` runs.

## Trial Change

The experiment removed the automatic deep-split decision for roots with:

- initial lower bound `8`;
- unchanged first move between base and strong ordering;
- strong minimum count from `6` to `8`.

Those cases then used the normal root-parallel scheduler. The goal was to test
whether the extra split work was still helping after the later V6 pruning
optimizations.

## Benchmark Command

```sh
scripts/run_v6_tail_baseline.sh \
  --build-dir out/release-native-lto \
  --output-dir out/release-native-lto/benchmark-results/v6-tail-pass15 \
  --cache-dir /tmp/rubik_cube_library_v6_tail_cache \
  --threads 0 \
  --max-memory-mb 4096 \
  --cache-mode reuse
```

## Results

Corpus: V6 tail baseline, 43 optimal cases, warm table cache.

| Metric | Pass 12 | LB8 root-scheduler trial |
| --- | ---: | ---: |
| Solved | 43 / 43 | 43 / 43 |
| Total solver elapsed | 28861 ms | 29777 ms |
| Total nodes expanded | 169155502 | 174149289 |
| p50 solver elapsed | 307 ms | 315 ms |
| p90 solver elapsed | 1742 ms | 1755 ms |
| p95 solver elapsed | 1835 ms | 1830 ms |
| p99 solver elapsed | 3645 ms | 4111 ms |
| Max solver elapsed | 3749 ms | 4138 ms |
| Max wall elapsed | 4506 ms | 4893 ms |

## Decision

Rejected. Disabling the `lb8_stable_mid_strong_min` deep split regressed total
solver time, node count, p50, p90, p99, max solver latency, and max wall
latency. The largest measured tail case moved from the deep-split path back to
the root scheduler and became slower.

Pass 12 remains the accepted implementation. The source change was reverted and
only this investigation note is retained.

## Verification

- `cmake --build out/release-native-lto --target rubik-bench rubik-cache-setup -j$(nproc)`
- `scripts/run_v6_tail_baseline.sh --build-dir out/release-native-lto --output-dir out/release-native-lto/benchmark-results/v6-tail-pass15 --cache-dir /tmp/rubik_cube_library_v6_tail_cache --threads 0 --max-memory-mb 4096 --cache-mode reuse`
