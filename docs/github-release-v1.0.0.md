# GitHub Release Draft - v1.0.0

Use this document as the GitHub Release body for tag `v1.0.0`.

## Release Metadata

- Tag: `v1.0.0`
- Target: first public stable release
- Title: `v1.0.0 - First stable release`
- Attachments:
  - `dist/rubik_cube_library-1.0.0.tar.gz`
  - `dist/rubik_cube_library-1.0.0.tar.gz.sha256`

## Release Body

````markdown
# Rubik Cube Library v1.0.0

First public stable release of Rubik Cube Library, a C++20 3x3x3 Rubik's Cube solver
library focused on certified HTM optimal solving.

This is a stable release: the main `rubik::Solver` API is intended to remain
source-compatible throughout the `1.x` line, while explicitly experimental APIs
may still change.

## Highlights

- C++20 library for 3x3x3 Rubik's Cube solving.
- Public 54-sticker input parser in `U R F D L B` face order.
- Physical cube validation: centers, cubies, orientations, and parity.
- HTM move parsing, formatting, and application.
- `rubik::Solver` with `SolveMode::Optimal`.
- `SolveStatus::Optimal` means the returned HTM solution is proven minimal
  under the requested options.
- `SolveProfile::Embedded`, `Default`, `Performance`, and `LargeLocal`.
- Root-level thread parallelism for optimal search through
  `SolveOptions::threads`.
- Public C++ version metadata through `rubik/version.hpp`.
- Experimental `SolveMode::Fast` and two-phase APIs.
- Pruning-table cache support through `RUBIK_TABLE_CACHE_DIR`.
- CLI tools: `rubik-solve` and `rubik-bench`, including `--help` and
  `--version`.
- CMake install/export package support with `rubik::rubik`.
- Apache License 2.0 with repository `NOTICE`.

## What Is Guaranteed In This Release

- Input format: 54 stickers in `U R F D L B` face order, each face read
  left-to-right and top-to-bottom.
- Metric: HTM. `R`, `R2`, and `R'` each count as one move.
- Optimal result: when the solver returns `SolveStatus::Optimal`, the returned
  move sequence is proven minimal for the requested options.
- Invalid sticker strings and physically impossible cubes are rejected before
  solving.

`SolveStatus::Timeout` and `SolveStatus::DepthLimitExceeded` mean no optimal
answer was proven inside the configured limits.

## Experimental / Not Stable Yet

- `SolveMode::Fast` is experimental and does not guarantee optimality.
- Phase-1 and phase-2 APIs are experimental.
- Pruning-table internals and coordinate APIs are experimental.
- Large local optimal table combinations have a high memory footprint and are
  not the embedded default policy.
- Benchmark random-case generation and environment-variable tuning flags are
  experimental.
- QTM is reserved and not implemented.

## Hardware Claims

Published benchmark claims for this release are limited to the development
desktop used for release-candidate validation:

- CPU: AMD Ryzen 9 8940HX, 16 cores / 32 threads.
- Build preset: `release-native-lto`.
- Benchmark cache mode: warm.

Raspberry Pi, Jetson Nano, and Jetson Orin latency claims are not published in
this release. Those targets are planned, but require direct hardware validation
before any performance claim is made.

## Release Candidate Validation

The release candidate passed:

- `release` configure/build/test: 47/47 tests.
- `release-native-lto` configure/build/test: 47/47 tests.
- `asan-ubsan` configure/build/test: 47/47 tests.
- Install/export consumer smoke test.
- Source archive validation.
- Fresh source-archive extraction build/test: 47/47 tests.
- `profile-realistic` benchmark gates.
- `embedded-multiseed` benchmark gates.
- `optimal-stress` benchmark gates.

Validation command:

```sh
scripts/release_check.sh --profile full --with-benchmarks
```

Final status:

```text
release_check,status,passed
```

## Benchmark Snapshot

Desktop release-candidate evidence from `2026-05-26`:

- `Embedded/Fast`, depth-20 x1,200 across 12 deterministic seeds:
  1,200/1,200 solved, worst observed max `381 ms`.
- `Embedded/Optimal`, depth-13 x120 across 12 deterministic seeds:
  120/120 solved, worst observed max `7,430 ms`.
- `Optimal` stress, depth-13 x90 across public profiles:
  90/90 solved; worst embedded seed max `6,756 ms`, worst default seed max
  `3,560 ms`, worst performance seed max `3,542 ms`.

These are local desktop numbers, not Raspberry Pi or Jetson numbers.

## Quick Start

```cpp
#include <rubik/cube.hpp>
#include <rubik/move.hpp>
#include <rubik/solver.hpp>

#include <chrono>
#include <iostream>

int main()
{
    auto parsed = rubik::Cube::fromStickers(
        "UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB");
    if (!parsed) {
        std::cerr << parsed.error.message << "\n";
        return 1;
    }

    rubik::Solver solver;
    rubik::SolveResult result = solver.solve(parsed.cube, {
        .mode = rubik::SolveMode::Optimal,
        .metric = rubik::Metric::HTM,
        .maxDepth = 20,
        .timeout = std::chrono::seconds(30),
        .maxMemoryBytes = 1024ull * 1024 * 1024,
        .threads = 1,
        .profile = rubik::SolveProfile::Default,
    });

    if (result.status == rubik::SolveStatus::Optimal ||
        result.status == rubik::SolveStatus::Solved) {
        std::cout << rubik::formatMoves(result.moves) << "\n";
    }
}
```

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Install and consume from CMake:

```sh
cmake --install build --prefix /path/to/install
```

```cmake
find_package(rubik CONFIG REQUIRED)
target_link_libraries(my_solver PRIVATE rubik::rubik)
```

## Documentation

- `README.md`: build, install, quick start, CLI usage.
- `CHANGELOG.md`: stable scope and known limits.
- `docs/api.md`: public API overview.
- `docs/api-stability-1.0.0.md`: stable API stability contract.
- `docs/release-candidate-2026-05-26.md`: release-candidate validation report.
- `docs/benchmarks.md`: benchmark suites and gate commands.
````

## Pre-Publish Checklist

- [ ] Confirm the tag points at the same source used for the final archive.
- [ ] Run `scripts/release_check.sh --profile full --with-benchmarks` one last
      time after all release-note edits are complete.
- [ ] Regenerate `dist/rubik_cube_library-1.0.0.tar.gz` and its `.sha256`
      checksum.
- [ ] Attach `dist/rubik_cube_library-1.0.0.tar.gz` and its `.sha256`
      checksum to the GitHub Release.
- [x] Confirm the release body does not claim Raspberry Pi or Jetson latency.
