# V2 Depth-15 Large-Local Validation - 2026-05-26

Host:

- CPU: AMD Ryzen 9 8940HX, 16 cores / 32 threads
- Build preset: `release-native-lto`
- Cache mode: warm

## Baseline Probe

Command:

```sh
cmake --build out/release-native-lto --target rubik-benchmark-optimal-deep-probe
```

The default optimal profiles still do not control the selected depth-15
frontier case within 30 seconds:

| Profile | Benchmark | Solved | Max ms | Nodes |
| --- | --- | ---: | ---: | ---: |
| Embedded | seed 12345 depth-14 x2 | 2/2 | 9304 | 3,816,078 |
| Embedded | seed 12345 depth-15 x1 | 0/1 | 30000 | 11,369,800 |
| Default | seed 12345 depth-14 x2 | 2/2 | 4940 | 1,500,490 |
| Default | seed 12345 depth-15 x1 | 0/1 | 30000 | 8,694,670 |
| Performance | seed 12345 depth-14 x2 | 2/2 | 5193 | 1,486,568 |
| Performance | seed 12345 depth-15 x1 | 0/1 | 30000 | 8,333,966 |

## Large-Local Probe

Command:

```sh
cmake --build out/release-native-lto --target rubik-benchmark-optimal-large-local
cmake --build out/release-native-lto --target rubik-benchmark-optimal-large-local-gates
```

Configuration:

- profile: `large-local`
- threads: `4`
- max memory option: `2048 MB`
- extra bounds: public large-local policy, including corner-state plus both
  corner/edge-group bounds
- table payload reported by the benchmark: `1,392,639,935` bytes

The target now executes all 24 fixed depth-15 seeds required by its gate. The
previous mismatch, where the CMake target executed only three seeds while the
gate expected 24, has been corrected.

## Results

| Metric | Value |
| --- | ---: |
| Cases | 24 |
| Solved | 24 |
| Failed | 0 |
| Worst elapsed ms | 27,572 |
| Worst nodes expanded | 25,589,013 |

Slowest cases:

| Seed | Elapsed ms | Nodes |
| --- | ---: | ---: |
| 987654321 | 27,572 | 25,589,013 |
| 424242 | 26,325 | 24,150,397 |
| 666 | 23,801 | 22,164,482 |
| 555 | 23,509 | 21,878,550 |
| 99 | 22,612 | 20,868,030 |
| 8675309 | 21,241 | 20,053,399 |
| 888 | 20,469 | 18,616,696 |
| 42 | 20,175 | 18,559,027 |

## 8-Thread Tail Replay

Command:

```sh
cmake --build out/release-native-lto --target rubik-benchmark-optimal-large-local-tail-8threads
cmake --build out/release-native-lto --target rubik-benchmark-optimal-large-local-tail-8threads-gates
```

The 8-thread tail target replays the six slowest known depth-15 large-local
seeds with an 18 second gate.

| Seed | 4-thread ms | 8-thread ms | 8-thread nodes |
| --- | ---: | ---: | ---: |
| 987654321 | 27,572 | 15,475 | 28,001,618 |
| 424242 | 26,325 | 13,712 | 23,848,206 |
| 666 | 23,801 | 9,984 | 17,642,628 |
| 555 | 23,509 | 12,288 | 21,692,898 |
| 99 | 22,612 | 10,311 | 17,957,140 |
| 888 | 20,469 | 8,716 | 14,917,903 |

The gate passed for all six cases. Worst 8-thread elapsed time was `15,475 ms`.

## Decision

Depth-15 is controlled by `SolveProfile::LargeLocal` on the development desktop
for the current 24-seed gate, but not by the embedded/default/performance
profiles. The 8-thread tail replay gives the large-local profile additional
latency margin on the slowest known seeds. This profile remains suitable for
high-memory local compute after direct validation, not for the embedded default
policy.
