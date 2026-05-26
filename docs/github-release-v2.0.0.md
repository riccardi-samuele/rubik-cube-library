# GitHub Release Draft - v2.0.0

Use this document as the GitHub Release body for tag `v2.0.0`.

## Release Metadata

- Tag: `v2.0.0`
- Target: second stable release
- Title: `v2.0.0 - Certified optimal solver contract`
- Attachments:
  - `dist/rubik_cube_library-2.0.0.tar.gz`
  - `dist/rubik_cube_library-2.0.0.tar.gz.sha256`

## Release Body

````markdown
# Rubik Cube Library v2.0.0

Second stable release of Rubik Cube Library, a C++20 3x3x3 Rubik's Cube solver
library focused on certified HTM optimal solving.

The V2 public contract centers on the main `Cube -> Solver -> SolveResult`
path. `SolveStatus::Optimal` means the returned HTM solution is proven minimal
under the requested options.

## Highlights

- Documented `2.0.0` API stability contract.
- Certified HTM optimal solving remains the primary library guarantee.
- Corner-state admissible pruning enabled in default optimal profiles after
  benchmark validation.
- Tighter profile-realistic, embedded-multiseed, and optimal-stress gates.
- Large-local depth-15 validation for high-memory local optimal solving.
- `SolveProfile::Embedded`, `Default`, `Performance`, and `LargeLocal`.
- `SolveMode::Fast` remains available as a practical non-optimal mode.
- CMake install/export package support with `rubik::rubik`.
- Public C++ version metadata through `rubik/version.hpp`.
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

- `SolveMode::Fast` does not guarantee optimality.
- Phase-1 and phase-2 APIs are experimental.
- Pruning-table internals and coordinate APIs are experimental.
- Large-local optimal table combinations have a high memory footprint and are
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
this release. Those targets require direct hardware validation before any
performance claim is made.

## Release Candidate Validation

The release candidate must pass:

- `release` configure/build/test.
- `release-native-lto` configure/build/test.
- `asan-ubsan` configure/build/test.
- Install/export consumer smoke test.
- Source archive validation.
- Fresh source-archive extraction build/test.
- `profile-realistic` benchmark gates.
- `embedded-multiseed` benchmark gates.
- `optimal-stress` benchmark gates.

Validation command:

```sh
scripts/release_check.sh --profile full --with-benchmarks
```

Large-local validation command:

```sh
scripts/release_check.sh --profile full --with-large-local
```

## Benchmark Snapshot

Desktop V2 evidence from `2026-05-26`:

- V2 corner-state validation: profile-realistic, embedded-multiseed,
  optimal-stress, and optimal tail-case gates passed.
- Embedded fast depth-20 multiseed gate: all sampled cases solved.
- Embedded optimal depth-13 multiseed gate: all sampled cases solved.
- Large-local depth-15 validation: 24/24 fixed seeds solved under the 30s
  4-thread gate.
- Large-local 8-thread tail validation: six known tails passed the 18s gate.

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
- `docs/api-stability-2.0.0.md`: stable API contract.
- `docs/release-2.0.0.md`: release checklist.
- `docs/benchmarks.md`: benchmark suites and gate commands.
````

## Pre-Publish Checklist

- [ ] Confirm the tag points at the same source used for the final archive.
- [ ] Run `scripts/release_check.sh --profile full --with-benchmarks` one last
      time after all release-note edits are complete.
- [ ] Run `scripts/release_check.sh --profile full --with-large-local` if
      large-local claims are included in the release body.
- [ ] Regenerate `dist/rubik_cube_library-2.0.0.tar.gz` and its `.sha256`
      checksum.
- [ ] Attach `dist/rubik_cube_library-2.0.0.tar.gz` and its `.sha256`
      checksum to the GitHub Release.
- [x] Confirm the release body does not claim Raspberry Pi or Jetson latency.
