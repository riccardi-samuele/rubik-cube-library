# Embedded Optimal Tail-Case Diagnostics - 2026-05-25

Host:

- CPU: AMD Ryzen 9 8940HX, 16 cores / 32 threads
- Build preset: `release-native-lto`
- Cache: warm, `RUBIK_TABLE_CACHE_DIR=/tmp/rubik_cube_library_optimal_tail_cache`
- Suite: `optimal-tail-cases`
- Profile: `Embedded`

Command:

```sh
cmake --build out/release-native-lto --target rubik-benchmark-optimal-tail-cases
cmake --build out/release-native-lto --target rubik-benchmark-optimal-tail-cases-gates
```

This suite replays the slowest `Embedded/Optimal` cases found by the first
`optimal-stress` run. It is intended as the inner-loop benchmark before changing
optimal-mode pruning or search policy.

## Summary

| Case | Solved | Elapsed ms | Nodes | Initial LB | Cheap candidate prunes | Three-phase checks | Three-phase prunes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `random_12345_4` | 1 | 9,678 | 4,731,004 | 7 | 54,280,420 | 8,881,049 | 4,149,902 |
| `random_42_2` | 1 | 6,637 | 3,174,144 | 6 | 35,107,454 | 7,263,307 | 4,089,120 |
| `random_42_1` | 1 | 8,408 | 4,668,997 | 7 | 54,180,756 | 8,141,232 | 3,472,149 |
| `random_20260525_7` | 1 | 8,078 | 4,373,890 | 7 | 51,022,291 | 7,356,446 | 2,982,832 |
| `random_12345_2` | 1 | 5,871 | 2,864,678 | 7 | 32,565,500 | 5,673,955 | 2,809,651 |

## Interpretation

All five replay cases solved and all gates passed. After enabling
three-direction phase-1 lower bounds for `Embedded/Optimal`, the worst replay
case dropped from `random_42_2` at 18.912 seconds to `random_12345_4` at 9.678
seconds. Diagnostic rows now show millions of three-phase candidate checks and
prunes, confirming that the stronger embedded lower bound is active by default.

The embedded optimal pruning-table payload remains unchanged at 22,123,535
bytes because the three-direction bound reuses tables already generated for the
embedded profile. The next optimization should use this replay suite to test
deeper tails or new admissible bounds.

After promoting corner-state pruning by default on 2026-05-26, the same replay
suite solved all five cases with a worst elapsed time of `2687 ms` and a worst
node count of `1,001,517`. The active tail-case gate was tightened from
`12000 ms` to `4000 ms`.

## Active Gates

```sh
cmake --build out/release-native-lto --target rubik-benchmark-optimal-tail-cases-gates
```

- Each replay case must solve.
- Each replay case must stay below `4000 ms` for p95, p99, and max.
