# V4 Root Ordering Experiments - 2026-05-27

This document records a V4 CPU-only root-ordering experiment on the current
tail corpus seeds. The numbers are local desktop measurements only.

## Command

```sh
scripts/benchmark_root_ordering_experiments.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_root_ordering_cache \
  --output-dir out/release-native-lto/benchmark-results/v4-root-ordering-experiments \
  --seeds 987654321,424242,1009,2016,666,555,99,888,12345,8675309 \
  --threads 0 \
  --max-memory-mb 2048 \
  --cache-mode warm
```

## Configuration

- Build preset: `release-native-lto`
- Mode: `SolveMode::Optimal`
- Requested profile: `auto`
- Effective profile: `large-local`
- Threads: `16`
- Maximum memory setting: `2048 MB`
- Table payload: `1392639935` bytes
- Variants: `default`, `reverse_tie`, `phase2_tiebreak`
- Cache setup elapsed time in this run: `226562 ms`

## Comparison

| Seed | Default ms | Reverse Tie ms | Phase2 Tiebreak ms | Winner |
| --- | ---: | ---: | ---: | --- |
| `1009` | 9410 | 9682 | 9550 | `default` |
| `987654321` | 7532 | 7871 | 7567 | `default` |
| `2016` | 5319 | 5342 | 5330 | `default` |
| `8675309` | 4937 | 5038 | 4965 | `default` |
| `12345` | 4828 | 4837 | 4819 | `phase2_tiebreak` |
| `555` | 2933 | 2797 | 2881 | `reverse_tie` |
| `424242` | 2920 | 3019 | 2905 | `phase2_tiebreak` |
| `99` | 2055 | 2251 | 2023 | `phase2_tiebreak` |
| `888` | 1928 | 1873 | 1908 | `reverse_tie` |
| `666` | 974 | 1131 | 972 | `phase2_tiebreak` |

Aggregate:

| Variant | Average solver ms | Max solver ms |
| --- | ---: | ---: |
| `default` | 4283 | 9410 |
| `reverse_tie` | 4384 | 9682 |
| `phase2_tiebreak` | 4292 | 9550 |

## Decision

No root-ordering policy change is accepted from this experiment. The default
ordering has the best average latency and the best worst-case latency on this
V4 seed set. `phase2_tiebreak` and `reverse_tie` improve selected seeds, but
both regress the most important hard cases.

The next V4 optimization work should focus on root scheduling diagnostics or a
more selective policy that only changes ordering when data predicts a benefit.

No Raspberry Pi, Jetson, Orin, or other embedded hardware measurements are
included in this document.
