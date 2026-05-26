# Release Candidate Validation - 2026-05-26

This document records the local validation run for the `1.0.0` release
candidate.

## Host

- CPU: AMD Ryzen 9 8940HX, 16 cores / 32 threads
- Architecture: x86_64
- Build presets: `release`, `release-native-lto`, `asan-ubsan`
- Benchmark build preset: `release-native-lto`
- Benchmark cache mode: warm

Hardware-specific Raspberry Pi, Jetson Nano, and Jetson Orin latency claims are
not made from this run. Those targets still require direct hardware benchmarks.

## Command

```sh
scripts/release_check.sh --profile full --with-benchmarks
```

Final status:

```text
release_check,status,passed
```

## Validation Summary

| Gate | Result |
| --- | --- |
| `release` configure/build/test | Passed, 46/46 tests |
| `release-native-lto` configure/build/test | Passed, 46/46 tests |
| `asan-ubsan` configure/build/test | Passed, 46/46 tests |
| Install/export consumer smoke test | Passed |
| Source archive validation | Passed |
| Source archive clean-tree build/test | Passed, 46/46 tests |
| `profile-realistic` benchmark gates | Passed |
| `embedded-multiseed` benchmark gates | Passed |
| `optimal-stress` benchmark gates | Passed |

Validated source archive:

```text
dist/rubik_cube_library-1.0.0.tar.gz
```

Archive audit:

- Size: 120 KiB.
- Manifest entries: 107.
- Includes the release-candidate validation document.
- Excludes generated build trees, benchmark CSV output directories, package
  output directories, runtime caches, IDE files, and compile databases.
- A fresh extraction configured, built, and passed the complete local test suite:
  46/46 tests.

## Profile Realistic Summary

Source summary:

```text
out/release-native-lto/benchmark-results/profile-realistic/warm_profile_realistic_summary.csv
```

| Profile | Mode | Benchmark | Cases | Solved | Failed | P50 ms | P95 ms | P99 ms | Max ms |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Embedded | Fast | depth-20 x100 | 100 | 100 | 0 | 93 | 206 | 286 | 287 |
| Embedded | Optimal | depth-12 x20 | 20 | 20 | 0 | 93 | 423 | 501 | 501 |
| Embedded | Optimal | depth-13 x10 | 10 | 10 | 0 | 776 | 6,716 | 6,716 | 6,716 |
| Default | Fast | depth-20 x100 | 100 | 100 | 0 | 81 | 258 | 543 | 631 |
| Default | Optimal | depth-12 x20 | 20 | 20 | 0 | 48 | 201 | 201 | 201 |
| Default | Optimal | depth-13 x10 | 10 | 10 | 0 | 310 | 3,585 | 3,585 | 3,585 |
| Performance | Fast | depth-20 x100 | 100 | 100 | 0 | 181 | 434 | 528 | 598 |
| Performance | Optimal | depth-12 x20 | 20 | 20 | 0 | 49 | 196 | 201 | 201 |
| Performance | Optimal | depth-13 x10 | 10 | 10 | 0 | 312 | 3,576 | 3,576 | 3,576 |

## Embedded Multiseed Summary

Source summary:

```text
out/release-native-lto/benchmark-results/embedded-multiseed/warm_embedded_multiseed_summary.csv
```

This release-candidate run covered 12 deterministic seeds. For each seed:

- `Embedded/Fast`: depth-20 x100, `max-depth 28`.
- `Embedded/Optimal`: depth-13 x10.

Aggregate result:

| Mode | Total Cases | Solved | Failed | Worst P95 ms | Worst P99 ms | Worst Max ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Embedded/Fast | 1,200 | 1,200 | 0 | 285 | 315 | 381 |
| Embedded/Optimal | 120 | 120 | 0 | 7,430 | 7,430 | 7,430 |

The active gates require every fast seed to solve 100/100 cases with
`p95<=350 ms`, `p99<=500 ms`, and `max<=700 ms`, and every optimal seed to solve
10/10 cases with `p95/p99/max<=12000 ms`.

## Optimal Stress Summary

Source summary:

```text
out/release-native-lto/benchmark-results/optimal-stress/warm_optimal_stress_summary.csv
```

| Profile | Seed | Cases | Solved | Failed | P50 ms | P95 ms | Max ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Embedded | 12345 | 10 | 10 | 0 | 768 | 6,756 | 6,756 |
| Embedded | 20260525 | 10 | 10 | 0 | 39 | 6,295 | 6,295 |
| Embedded | 42 | 10 | 10 | 0 | 264 | 6,642 | 6,642 |
| Default | 12345 | 10 | 10 | 0 | 310 | 3,560 | 3,560 |
| Default | 20260525 | 10 | 10 | 0 | 20 | 2,814 | 2,814 |
| Default | 42 | 10 | 10 | 0 | 125 | 2,456 | 2,456 |
| Performance | 12345 | 10 | 10 | 0 | 311 | 3,542 | 3,542 |
| Performance | 20260525 | 10 | 10 | 0 | 21 | 2,788 | 2,788 |
| Performance | 42 | 10 | 10 | 0 | 124 | 2,465 | 2,465 |

The active gates require `Embedded/Optimal` to stay under 12 seconds per seed
and `Default/Optimal` plus `Performance/Optimal` to stay under 5.5 seconds per
seed for these depth-13 stress samples.

## Release Interpretation

The local release candidate is internally consistent: build, test, sanitizer,
install/export, archive, and benchmark gates all passed on the development
desktop.

For `1.0.0`, publish desktop benchmark claims from this document only as
local evidence. Do not publish Raspberry Pi or Jetson latency numbers until the
same benchmark suites have been run on those devices.
