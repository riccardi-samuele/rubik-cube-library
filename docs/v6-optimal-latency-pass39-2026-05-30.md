# V6 optimal latency pass 39

## Goal

Refresh the full V6 local tail baseline using the `require-warm` benchmark
target, after adding cache metadata to the V6 runner manifest.

This pass is a measurement refresh, not a solver-policy change.

## Command

```bash
RUBIK_BENCH_COMMAND_TIMEOUT_MS=45000 \
  cmake --build out/release-native-lto \
  --target rubik-benchmark-v6-tail-baseline-require-warm
```

The runner wrote output under:

```text
out/release-native-lto/benchmark-results/v6-tail-baseline-require-warm
```

## Cache State

`manifest.csv` and `cache_setup.csv` recorded:

| Field | Value |
| --- | --- |
| Git revision | `293a40a` |
| Cache mode | `require-warm` |
| Effective profile | `large-local` |
| Payload bytes | 1392639935 |
| Cache warm | true |
| Bytes missing | 0 |
| Cache setup elapsed ms | 3 |

## Summary

Tail depth-15 replay:

| Metric | Value |
| --- | ---: |
| Cases | 7 |
| Solved | 7 |
| Total solver ms | 11042 |
| Max solver ms | 2124 |
| Total nodes | 56185802 |

Hardening replay:

| Metric | Value |
| --- | ---: |
| Summary rows | 24 |
| Failed rows | 0 |
| Slowest depth-15 row | 2700 ms |
| Slowest depth-14 row | 636 ms |

Slowest cases:

| Case | Depth | Solver ms | Nodes |
| --- | ---: | ---: | ---: |
| hardening seed 42 | 15 | 2700 | 13497316 |
| tail seed 555 | 15 | 2124 | 11599042 |
| tail seed 424242 | 15 | 2052 | 10362341 |
| hardening seed 424242 | 15 | 2048 | 10238953 |
| tail seed 1009 | 15 | 2029 | 11209529 |

## Comparison To Earlier V6 Baselines

Against pass 20:

| Metric | Pass 20 | Current | Delta |
| --- | ---: | ---: | ---: |
| Common cases | 43 | 43 | 0 |
| Total solver ms | 28037 | 27207 | -830 |
| p50 solver ms | 252 | 293 | +41 |
| p90 solver ms | 1784 | 1695 | -89 |
| p95 solver ms | 1829 | 2048 | +219 |
| p99 solver ms | 3545 | 2124 | -1421 |
| Max solver ms | 3593 | 2700 | -893 |
| Total nodes | 165163047 | 135447282 | -29715765 |
| Max wall ms | 4237 | 3401 | -836 |

Against pass 32:

| Metric | Pass 32 | Current | Delta |
| --- | ---: | ---: | ---: |
| Common cases | 43 | 43 | 0 |
| Total solver ms | 26455 | 27207 | +752 |
| p50 solver ms | 265 | 293 | +28 |
| p90 solver ms | 1799 | 1695 | -104 |
| p95 solver ms | 1912 | 2048 | +136 |
| p99 solver ms | 2126 | 2124 | -2 |
| Max solver ms | 2853 | 2700 | -153 |
| Total nodes | 135523614 | 135447282 | -76332 |
| Max wall ms | 3638 | 3401 | -237 |

## Reason Groups

Against pass 20, the current build still wins overall because
`lb8_stable_mid_strong_min` improved sharply:

| Reason | Pass 20 ms | Current ms | Delta |
| --- | ---: | ---: | ---: |
| `lb8_stable_mid_strong_min` | 7468 | 3559 | -3909 |
| `conservative_root` | 10371 | 11961 | +1590 |
| `lb9_mid_strong_min` | 4357 | 4957 | +600 |
| `lb9_low_strong_min` | 3320 | 3854 | +534 |
| `depth14_conservative_root` | 1967 | 2226 | +259 |

Against pass 32, total solver time is slightly worse, but max solver time and
max wall time are better. The node count is effectively flat, so the current
differences should be treated as benchmark variation plus policy side effects,
not as a new accepted performance win.

## Decision

Keep the current V6 policy. The `require-warm` target is now the preferred way
to refresh the local V6 baseline because it prevents table generation from
polluting solver-latency measurements.

The next solver work should focus on the remaining `conservative_root` depth-15
cases without broad forced deep splitting, which pass 35 already rejected.
