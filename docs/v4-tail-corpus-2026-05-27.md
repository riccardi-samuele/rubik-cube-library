# V4 Tail Corpus - 2026-05-27

This document records the first replayable V4 CPU-only optimal tail corpus. The
corpus replays the slowest cases selected by the V4 discovery run.

## Commands

```sh
cmake --build out/release-native-lto --target rubik-benchmark-v4-tail-corpus -- -j1
scripts/analyze_root_search_profile.py \
  --input-dir out/release-native-lto/benchmark-results/v4-tail-corpus \
  --summary \
  --output out/release-native-lto/benchmark-results/v4-tail-corpus/root_summary.csv
```

The corpus target runs:

```sh
scripts/run_v4_tail_corpus.sh \
  --build-dir out/release-native-lto \
  --cases-file out/release-native-lto/benchmark-results/v4-tail-discovery/slowest.csv \
  --cache-dir /tmp/rubik_cube_library_v4_tail_discovery_cache \
  --output-dir out/release-native-lto/benchmark-results/v4-tail-corpus \
  --threads 0 \
  --max-memory-mb 2048 \
  --cache-mode reuse
```

## Configuration

- Build preset: `release-native-lto`
- Mode: `SolveMode::Optimal`
- Requested profile: `auto`
- Effective profile: `large-local`
- Threads: `16`
- Maximum memory setting: `2048 MB`
- Cache mode: `reuse`
- Cases file: `out/release-native-lto/benchmark-results/v4-tail-discovery/slowest.csv`
- Timeout: `30000 ms`

## Result

- Cases: `11/11` solved
- Failed cases: `0`
- Slowest solver elapsed time: `9270 ms`
- Slowest wall elapsed time: `9900 ms`
- Slowest seed: `1009`

## Corpus Rows

| Seed | Solver ms | Nodes | Wall ms |
| --- | ---: | ---: | ---: |
| `1009` | 9270 | 33191519 | 9900 |
| `987654321` | 7564 | 26169805 | 8208 |
| `2016` | 5321 | 19685916 | 5971 |
| `8675309` | 4970 | 18530747 | 5621 |
| `12345` | 4834 | 17570138 | 5483 |
| `424242` | 2950 | 10324023 | 3606 |
| `555` | 2898 | 9565082 | 3555 |
| `99` | 2034 | 6151114 | 2694 |
| `888` | 1890 | 5617792 | 2547 |
| `666` | 971 | 2812550 | 1619 |
| `20260525` | 100 | 142003 | 750 |

## Root Diagnostics

| Seed | Solution root ms | Max root ms | Roots before solution | Worker roots min/max | Worker imbalance ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| `1009` | 8066 | 8066 | 11 | 1/2 | 1745 |
| `987654321` | 6615 | 6615 | 13 | 1/2 | 696 |
| `2016` | 4572 | 4572 | 10 | 1/2 | 1 |
| `12345` | 4070 | 4070 | 3 | 1/1 | 0 |
| `8675309` | 4090 | 4090 | 3 | 1/1 | 0 |
| `424242` | 2203 | 2203 | 13 | 1/1 | 0 |
| `555` | 1870 | 1870 | 6 | 1/1 | 0 |
| `888` | 1064 | 1064 | 15 | 1/1 | 0 |
| `99` | 1037 | 1038 | 12 | 1/1 | 1 |
| `666` | 131 | 131 | 12 | 1/1 | 0 |
| `20260525` | 33 | 33 | 6 | 1/2 | 8 |

## Interpretation

The corpus gives V4 a deterministic CPU-only target set for optimization. The
largest current issue is still seed `1009`, where the solution root itself costs
about `8066 ms` and appears after `11` roots before the solution. Worker
diagnostics show that the worst case is dominated by one expensive root rather
than by a broad worker distribution problem. This points toward either more
selective root ordering or deeper task splitting as the next CPU optimization
areas to test.

No Raspberry Pi, Jetson, Orin, or other embedded hardware measurements are
included in this document.
