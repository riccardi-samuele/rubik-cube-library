# V6 Local Baseline - 2026-05-28

This document records the starting local baseline for V6 optimal-latency work.
No solver behavior changed for this run.

## Scope

V6 focuses on reducing local `SolveMode::Optimal` latency while preserving the
certified HTM optimality contract. When a result reports `SolveStatus::Optimal`,
the returned solution must remain a proven minimum-length HTM solution for the
requested options.

These are local desktop measurements only. Raspberry Pi, Jetson Nano, Jetson
Orin, GPU, cloud, and other external hardware latency claims are not included.

## Environment

- Commit: `a4698e5b028bcb63bd3fe8b956b393c256f69d8f`
- Tag at baseline: `v5.0.0`
- Version reported by benchmarks: `5.0.0`
- Build preset: `release-native-lto`
- Compiler flags: native CPU enabled, LTO enabled by preset
- CPU: AMD Ryzen 9 8940HX with Radeon Graphics
- CPU topology: 16 cores, 32 threads
- Benchmark profile: `SolveProfile::Auto`
- Effective benchmark profile: `large-local`
- Solver mode: `SolveMode::Optimal`
- Threads selected by Auto: 16
- Memory budget: 2048 MiB
- Table payload prepared: 1392639935 bytes
- Cache mode: warm

## Verification Before Benchmarking

```sh
cmake --preset release-native-lto
cmake --build --preset release-native-lto
ctest --test-dir out/release-native-lto -R 'rubik_tests|public_examples_current_api|public_docs_current_version' --output-on-failure
```

Result: selected tests passed, 3/3.

## Benchmark Commands

```sh
cmake --build out/release-native-lto --target rubik-benchmark-optimal-auto-tail
cmake --build out/release-native-lto --target rubik-benchmark-optimal-auto-hardening
```

Output directories:

- `out/release-native-lto/benchmark-results/optimal-auto-tail`
- `out/release-native-lto/benchmark-results/optimal-auto-hardening`

Summary files:

- `out/release-native-lto/benchmark-results/optimal-auto-tail/warm_optimal_auto_tail_summary.csv`
- `out/release-native-lto/benchmark-results/optimal-auto-hardening/warm_optimal_auto_hardening_summary.csv`

## Aggregate Results

| Suite | Cases | Solved | Failed | Total solver ms | Total nodes | Max solver ms | Max wall ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| optimal-auto-tail | 7 | 7 | 0 | 25818 | 89141916 | 7677 | 8310 |
| optimal-auto-hardening | 36 | 36 | 0 | 32691 | 110003140 | 7623 | 8258 |
| combined | 43 | 43 | 0 | 58509 | 199145056 | 7677 | 8310 |

The slowest measured solver case in this baseline is
`random_seed_987654321_depth_15_count_1` from `optimal-auto-tail`:

- solver elapsed: 7677 ms;
- wall elapsed: 8310 ms;
- nodes expanded: 26540316;
- scramble: `B' R' B' R' F B D' F R U2 F B U' B2 F`;
- solution length: 15 HTM.

## Slowest Rows

| Suite | Benchmark | Solver ms | Wall ms | Nodes |
| --- | --- | ---: | ---: | ---: |
| optimal-auto-tail | `random_seed_987654321_depth_15_count_1` | 7677 | 8310 | 26540316 |
| optimal-auto-hardening | `random_seed_987654321_depth_15_count_1` | 7623 | 8258 | 26378326 |
| optimal-auto-tail | `random_seed_1009_depth_15_count_1` | 7464 | 8098 | 28272897 |
| optimal-auto-hardening | `random_seed_12345_depth_15_count_1` | 4775 | 5406 | 17457695 |
| optimal-auto-hardening | `random_seed_42_depth_15_count_1` | 3985 | 4619 | 13341815 |
| optimal-auto-hardening | `random_seed_424242_depth_15_count_1` | 2972 | 3614 | 10356348 |
| optimal-auto-tail | `random_seed_555_depth_15_count_1` | 2909 | 3546 | 9667373 |
| optimal-auto-tail | `random_seed_424242_depth_15_count_1` | 2908 | 3543 | 10118159 |
| optimal-auto-hardening | `random_seed_8675309_depth_15_count_1` | 2429 | 3079 | 9395995 |
| optimal-auto-hardening | `random_seed_99_depth_15_count_1` | 2042 | 2689 | 6134751 |
| optimal-auto-tail | `random_seed_99_depth_15_count_1` | 2024 | 2664 | 6139249 |
| optimal-auto-tail | `random_seed_888_depth_15_count_1` | 1883 | 2519 | 5604208 |

## Initial Reading

This baseline did not reproduce a 30-second case in the selected V5 Auto tail
and hardening suites. The measured local worst case is still high enough to
justify V6 latency work: the top three rows spend about 7.5 to 7.7 seconds in
solver time and expand roughly 26 to 28 million nodes.

The first V6 optimization work should focus on depth-15 Auto large-local cases,
especially cases where the diagnostics show many root candidates searched
before the solution root is found.
