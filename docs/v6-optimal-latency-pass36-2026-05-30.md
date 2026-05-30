# V6 optimal latency pass 36

## Goal

Make V6 tail-latency benchmark runs reproducible without accidentally measuring
first-run large-local table generation as solver latency.

Pass 35 showed that a partially warm cache can dominate a latency probe. This
pass records the benchmark guard added after that result.

## Change

The V6 baseline runner now supports `--cache-mode require-warm`, and CMake has a
dedicated target:

```bash
cmake --build out/release-native-lto --target rubik-benchmark-v6-tail-baseline-require-warm
```

The target calls:

```bash
scripts/run_v6_tail_baseline.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-tail-baseline-require-warm \
  --threads 0 \
  --max-memory-mb 2048 \
  --cache-mode require-warm
```

In `require-warm` mode, the runner uses `rubik-cache-setup --dry-run` before
creating per-suite benchmark output directories. If required table files are
missing, it exits with an error instead of warming the cache during a latency
measurement.

## Measured smoke run

After the large-local cache was warmed under
`/tmp/rubik_cube_library_v6_tail_baseline_cache`, the smoke command was:

```bash
RUBIK_BENCH_COMMAND_TIMEOUT_MS=45000 scripts/run_v6_tail_baseline.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --cache-mode require-warm \
  --output-dir out/release-native-lto/benchmark-results/v6-pass36-require-warm-smoke \
  --tail-seeds 99 \
  --hardening-seeds 42 \
  --deep-opt14-count 1 \
  --deep-opt15-count 1 \
  --threads 0 \
  --max-memory-mb 2048 \
  --timeout-ms 30000
```

Cache dry-run result:

| Field | Value |
| --- | ---: |
| Effective profile | `large-local` |
| Payload bytes | 1392639935 |
| Cache warm | true |
| Bytes missing | 0 |

Measured rows:

| Case | Elapsed ms |
| --- | ---: |
| Tail seed 99, depth 15 | 1200 |
| Hardening seed 42, depth 14 | 213 |
| Hardening seed 42, depth 15 | 2300 |

These are local measurements from this workstation only.

## Verification

Commands run after adding the target:

```bash
cmake --preset release-native-lto
ctest --test-dir out/release-native-lto -R 'cmake_v6_require_warm_target|run_v6_tail_baseline_require_warm_rejects_cold_cache|run_v6_tail_baseline_rejects_missing_values' --output-on-failure
ctest --test-dir out/release-native-lto --output-on-failure
git diff --check
```

Results:

| Check | Result |
| --- | ---: |
| Targeted V6 tests | 3/3 passed |
| Full CTest suite | 94/94 passed |
| Whitespace check | passed |

## Decision

Use `rubik-benchmark-v6-tail-baseline-require-warm` for latency-sensitive V6
comparison runs. Use the default `rubik-benchmark-v6-tail-baseline` target when
the benchmark workflow should prepare and record cache setup explicitly.
