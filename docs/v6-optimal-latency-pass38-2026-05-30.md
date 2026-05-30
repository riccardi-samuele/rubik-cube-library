# V6 optimal latency pass 38

## Goal

Validate the V6 `require-warm` manifest metadata on a real warm-cache smoke run
after adding `cache_setup_output` and `cache_setup_elapsed_ms`.

## Command

```bash
RUBIK_BENCH_COMMAND_TIMEOUT_MS=45000 scripts/run_v6_tail_baseline.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --cache-mode require-warm \
  --output-dir out/release-native-lto/benchmark-results/v6-pass38-manifest-smoke \
  --tail-seeds 99 \
  --hardening-seeds 42 \
  --deep-opt14-count 1 \
  --deep-opt15-count 1 \
  --threads 0 \
  --max-memory-mb 2048 \
  --timeout-ms 30000
```

## Manifest

`manifest.csv` recorded:

| Field | Value |
| --- | --- |
| `git_revision` | `69f3430` |
| `cache_mode` | `require-warm` |
| `cache_setup_output` | `out/release-native-lto/benchmark-results/v6-pass38-manifest-smoke/cache_setup.csv` |
| `cache_setup_elapsed_ms` | 3 |

`cache_setup.csv` recorded:

| Field | Value |
| --- | ---: |
| Status | `Ready` |
| Effective profile | `large-local` |
| Threads | 16 |
| Payload bytes | 1392639935 |
| Cache warm | true |
| Bytes missing | 0 |
| Message | `dry run: cache warm` |

## Smoke Results

| Case | Solve ms | Warmup ms | Wall ms | Nodes |
| --- | ---: | ---: | ---: | ---: |
| Tail seed 99, depth 15 | 1355 | 625 | 2064 | 6199302 |
| Hardening seed 42, depth 14 | 240 | 605 | 926 | 1009085 |
| Hardening seed 42, depth 15 | 2642 | 609 | 3331 | 13242425 |

These are local workstation smoke measurements. They validate benchmark
artifact structure; they are not a replacement for the full V6 tail baseline.

## Decision

Keep the V6 manifest metadata. The top-level manifest now identifies the exact
cache setup artifact and wrapper cache-check duration, while the per-suite CSV
files continue to report solver latency, warm-up time, wall time, nodes, and
diagnostics.
