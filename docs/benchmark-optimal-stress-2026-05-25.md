# Optimal Stress Benchmark - 2026-05-25

Host:

- CPU: AMD Ryzen 9 8940HX, 16 cores / 32 threads
- Build preset: `release-native-lto`
- Cache: warm, `RUBIK_TABLE_CACHE_DIR=/tmp/rubik_cube_library_optimal_stress_cache`
- Seeds: `12345`, `20260525`, `42`
- Suite: `optimal-stress`

Command:

```sh
cmake --build out/release-native-lto --target rubik-benchmark-optimal-stress
cmake --build out/release-native-lto --target rubik-benchmark-optimal-stress-gates
```

This is the first dedicated stress suite for optimal-mode tail latency. It runs
depth-13 random cases across all public profiles and multiple deterministic
seeds.

## Summary

| Profile | Mode | Benchmark | Cases | Solved | Failed | P50 | P90 | P95 | P99 | Max | Nodes |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Embedded | Optimal | seed 12345 depth-13 | 10 | 10 | 0 | 1,079 | 5,934 | 8,958 | 8,958 | 8,958 | 14,207,363 |
| Embedded | Optimal | seed 20260525 depth-13 | 10 | 10 | 0 | 48 | 4,086 | 7,994 | 7,994 | 7,994 | 7,386,908 |
| Embedded | Optimal | seed 42 depth-13 | 10 | 10 | 0 | 372 | 6,778 | 8,720 | 8,720 | 8,720 | 12,750,730 |
| Default | Optimal | seed 12345 depth-13 | 10 | 10 | 0 | 352 | 2,068 | 4,199 | 4,199 | 4,199 | 3,080,077 |
| Default | Optimal | seed 20260525 depth-13 | 10 | 10 | 0 | 24 | 2,778 | 3,190 | 3,190 | 3,190 | 2,011,214 |
| Default | Optimal | seed 42 depth-13 | 10 | 10 | 0 | 134 | 2,265 | 2,781 | 2,781 | 2,781 | 2,602,342 |
| Performance | Optimal | seed 12345 depth-13 | 10 | 10 | 0 | 339 | 1,829 | 3,932 | 3,932 | 3,932 | 2,993,467 |
| Performance | Optimal | seed 20260525 depth-13 | 10 | 10 | 0 | 23 | 2,694 | 3,086 | 3,086 | 3,086 | 1,908,327 |
| Performance | Optimal | seed 42 depth-13 | 10 | 10 | 0 | 138 | 2,421 | 2,840 | 2,840 | 2,840 | 2,519,940 |

## Slowest Cases

The current slowest embedded cases are:

| Case | Profile | Elapsed ms | Nodes | Scramble |
| --- | --- | ---: | ---: | --- |
| `random_12345_4` | Embedded | 8,958 | 4,731,004 | `B' F D' F B U2 F' L U2 F2 R D' B` |
| `random_42_1` | Embedded | 8,720 | 4,668,997 | `L2 D' L2 U' B2 U2 D2 F R2 F2 U D L` |
| `random_20260525_7` | Embedded | 7,994 | 4,373,890 | `F R' F' D F' U2 L' B2 F' U2 R B' R` |
| `random_42_2` | Embedded | 6,778 | 3,174,144 | `D' L' B' L2 F' U L2 U' F2 U2 D F' L2` |
| `random_12345_2` | Embedded | 5,934 | 2,864,678 | `R2 U F U B2 L B2 F2 L' D2 U F' B` |

## Interpretation

All 90 optimal solves completed and all gates passed. After enabling the
three-direction phase-1 lower bound for `Embedded/Optimal`, the worst embedded
case in this stress suite dropped from 15.997 seconds to 8.958 seconds. The
embedded optimal pruning-table payload stayed at 22,123,535 bytes because the
bound reuses tables already present in the embedded profile.

`Embedded/Optimal` is still the limiting local profile, but its slow-tail node
count is now roughly half of the first stress run. The next optimal work should
focus on deeper tail cases and additional admissible pruning rather than
reverting to the old embedded policy.

## Active Gates

```sh
cmake --build out/release-native-lto --target rubik-benchmark-optimal-stress-gates
```

- `Embedded/Optimal`: `solved=10`, `p95/p99/max<=12000 ms` per seed.
- `Default/Optimal`: `solved=10`, `p95/p99/max<=5500 ms` per seed.
- `Performance/Optimal`: `solved=10`, `p95/p99/max<=5500 ms` per seed.
