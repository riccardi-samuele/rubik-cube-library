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
- Slowest solver elapsed time: `9257 ms`
- Slowest wall elapsed time: `9966 ms`
- Slowest seed: `1009`

## Corpus Rows

| Seed | Solver ms | Nodes | Wall ms |
| --- | ---: | ---: | ---: |
| `1009` | 9257 | 33116693 | 9966 |
| `987654321` | 7608 | 26268147 | 8247 |
| `2016` | 5318 | 19696742 | 5968 |
| `8675309` | 4896 | 18154517 | 5549 |
| `12345` | 4801 | 17395459 | 5446 |
| `555` | 2901 | 9544701 | 3557 |
| `424242` | 2894 | 10083737 | 3548 |
| `99` | 2045 | 6168043 | 2695 |
| `888` | 1897 | 5597292 | 2542 |
| `666` | 971 | 2806840 | 1619 |
| `20260525` | 101 | 142691 | 749 |

## Root Diagnostics

| Seed | Solution root ms | Max root ms | Roots before solution |
| --- | ---: | ---: | ---: |
| `1009` | 8061 | 8062 | 11 |
| `987654321` | 6655 | 6655 | 13 |
| `2016` | 4578 | 4579 | 10 |
| `12345` | 4035 | 4035 | 3 |
| `8675309` | 4008 | 4008 | 3 |
| `424242` | 2143 | 2144 | 13 |
| `555` | 1875 | 1875 | 6 |
| `888` | 1066 | 1066 | 15 |
| `99` | 1049 | 1049 | 12 |
| `666` | 134 | 134 | 12 |
| `20260525` | 33 | 33 | 6 |

## Interpretation

The corpus gives V4 a deterministic CPU-only target set for optimization. The
largest current issue is still seed `1009`, where the solution root itself costs
about `8061 ms` and appears after `11` roots before the solution. This points to
root ordering and root scheduling as the first CPU optimization areas to test.

No Raspberry Pi, Jetson, Orin, or other embedded hardware measurements are
included in this document.
