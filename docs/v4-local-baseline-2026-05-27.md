# V4 Local Baseline - 2026-05-27

This document records the local V3 baseline used before V4 CPU tail-latency
work. The numbers are local desktop measurements only.

## Commands

```sh
cmake --build out/release-native-lto --target rubik-bench rubik-cache-setup
cmake --build out/release-native-lto --target rubik-benchmark-v3-auto-gates
scripts/extract_slowest_cases.sh \
  --input-dir out/release-native-lto/benchmark-results/optimal-auto-tail \
  --output out/release-native-lto/benchmark-results/v4_baseline_slowest.csv \
  --limit 20
```

## Configuration

- Build preset: `release-native-lto`
- Cache mode: warm
- Auto effective profile for tail runs: `large-local`
- Auto threads on this host: `16`
- Auto maximum memory setting: `2048 MB`
- Auto large-local table payload: `1392639935` bytes

## Result

- V3 Auto gates: passed
- Auto tail cases: `7/7` solved
- Slowest known depth-15 Auto tail seed in this run: `1009`
- Highest solver elapsed time observed in this run: `9439 ms`
- Highest wall elapsed time observed in this run: `10061 ms`

## Auto Tail Rows

| Seed | Solver ms | Nodes | Wall ms |
| --- | ---: | ---: | ---: |
| `1009` | 9439 | 33099605 | 10061 |
| `987654321` | 7495 | 26131167 | 8109 |
| `424242` | 2941 | 10279345 | 3564 |
| `555` | 2836 | 9417211 | 3469 |
| `99` | 2035 | 6120418 | 2672 |
| `888` | 1903 | 5602298 | 2541 |
| `666` | 970 | 2823143 | 1603 |

## Interpretation

The V4 baseline confirms that the current local optimal tail is dominated by
seed `1009`. The initial V4 performance target remains to reduce the known
depth-15 tail maximum below `8000 ms` solver time if the expanded V4 corpus
shows that target is realistic.

No Raspberry Pi, Jetson, Orin, or other embedded hardware measurements are
included in this document.
