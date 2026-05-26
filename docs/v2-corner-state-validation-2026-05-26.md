# V2 Corner-State Validation - 2026-05-26

Host:

- CPU: AMD Ryzen 9 8940HX, 16 cores / 32 threads
- Build preset: `release-native-lto`
- Cache mode: warm
- Solver change: corner orientation/permutation pruning enabled by default

## Commands

```sh
cmake --build out/release-native-lto --target rubik-benchmark-profile-realistic
cmake --build out/release-native-lto --target rubik-benchmark-profile-realistic-gates
cmake --build out/release-native-lto --target rubik-benchmark-embedded-multiseed
cmake --build out/release-native-lto --target rubik-benchmark-embedded-multiseed-gates
cmake --build out/release-native-lto --target rubik-benchmark-optimal-stress
cmake --build out/release-native-lto --target rubik-benchmark-optimal-stress-gates
```

## Results

Profile-realistic:

| Profile | Mode | Benchmark | Solved | p95 ms | p99 ms | Max ms | Gate |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: |
| Embedded | Fast | depth-20 x100 | 100 | 211 | 267 | 290 | 350/500/700 |
| Embedded | Optimal | depth-13 x10 | 10 | 2478 | 2478 | 2478 | 4000/4000/4000 |
| Default | Optimal | depth-13 x10 | 10 | 1141 | 1141 | 1141 | 2500/2500/2500 |
| Performance | Optimal | depth-13 x10 | 10 | 1153 | 1153 | 1153 | 2500/2500/2500 |

Embedded-multiseed:

| Mode | Cases | Solved | Worst p95 ms | Worst p99 ms | Worst max ms | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Fast depth-20 x100 across 12 seeds | 1200 | 1200 | 295 | 325 | 387 | 350/500/700 |
| Optimal depth-13 x10 across 12 seeds | 120 | 120 | 2527 | 2527 | 2527 | 4000/4000/4000 |

Optimal-stress:

| Profile | Cases | Solved | Worst p95 ms | Worst p99 ms | Worst max ms | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Embedded | 30 | 30 | 2544 | 2544 | 2544 | 4000/4000/4000 |
| Default | 30 | 30 | 1223 | 1223 | 1223 | 2500/2500/2500 |
| Performance | 30 | 30 | 1268 | 1268 | 1268 | 2500/2500/2500 |

## Decision

The promoted corner-state bound passes the profile-realistic, embedded-multiseed,
and optimal-stress gates with the tighter V2 thresholds. The embedded-multiseed
gate now checks every seed produced by the CMake target, not only the first
three seeds.

The active optimal latency gates are:

- embedded optimal depth-13: p95/p99/max `4000 ms`;
- default and performance optimal depth-13: p95/p99/max `2500 ms`.
