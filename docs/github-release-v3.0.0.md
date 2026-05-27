# GitHub Release Draft - v3.0.0

Use this document as the GitHub Release body for tag `v3.0.0` after final
release-candidate validation has passed.

## Release Metadata

- Tag: `v3.0.0`
- Target: third stable release
- Title: `v3.0.0 - Adaptive local optimal solving`
- Attachments:
  - `dist/rubik_cube_library-3.0.0.tar.gz`
  - `dist/rubik_cube_library-3.0.0.tar.gz.sha256`

## Release Body

````markdown
# Rubik Cube Library v3.0.0

Third stable release of Rubik Cube Library, a C++20 3x3x3 Rubik's Cube solver
library focused on certified HTM optimal solving.

The V3 public contract keeps the V2 optimality guarantee and makes
`SolveProfile::Auto` the recommended local optimal profile. When the solver
returns `SolveStatus::Optimal`, the returned HTM move sequence is proven
minimal under the requested options.

## Highlights

- `SolveProfile::Auto` is the recommended adaptive profile for local HTM
  optimal solving.
- `SolvePlan` reports the effective profile, cache policy, thread count,
  memory budget, and optimal move-ordering policy used by the solve.
- `prepareCache()` and the `rubik-cache-setup` CLI prepare pruning-table caches
  before latency-sensitive solves.
- Repeatable V3 Auto benchmark gates cover shallow Auto solves, fixed depth-15
  Auto tail cases, and Auto hardening cases.
- Selective Auto optimal move ordering is promoted for measured tail cases.
- Explicit `Embedded`, `Default`, `Performance`, and `LargeLocal` profiles
  remain available for fixed policy selection.
- CMake install/export package support remains available through `rubik::rubik`.
- Public C++ version metadata remains available through `rubik/version.hpp`.
- Apache License 2.0 with repository `NOTICE`.

## What Is Guaranteed In This Release

- Input format: 54 stickers in `U R F D L B` face order, each face read
  left-to-right and top-to-bottom.
- Metric: HTM. `R`, `R2`, and `R'` each count as one move.
- Optimal result: when the solver returns `SolveStatus::Optimal`, the returned
  move sequence is proven minimal for the requested options.
- `SolveProfile::Auto` respects the requested local resource limits and reports
  the selected plan through `SolvePlan`.
- Invalid sticker strings and physically impossible cubes are rejected before
  solving.

`SolveStatus::Timeout`, `SolveStatus::DepthLimitExceeded`, and
`SolveStatus::CacheNotReady` mean no optimal answer was proven for that call.

## Experimental / Not Stable Yet

- `SolveMode::Fast` does not guarantee optimality.
- Phase-1 and phase-2 APIs are experimental.
- Pruning-table internals and coordinate APIs are experimental.
- Root-ordering diagnostic strings are informational and may evolve.
- Benchmark random-case generation and environment-variable tuning flags are
  experimental.
- QTM is reserved and not implemented.
- GPU acceleration is not implemented.

## Hardware Claims

Published benchmark claims for this release are limited to the development
desktop used for local validation:

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
- V3 Auto release-candidate gates.
- Public documentation hardware-claim audit.

Validation command:

```sh
scripts/release_check.sh --profile quick --with-v3-auto
```

Full desktop benchmark validation command:

```sh
scripts/release_check.sh --profile full --with-benchmarks
```

Large-local validation command:

```sh
scripts/release_check.sh --profile full --with-large-local
```

## Benchmark Snapshot

Desktop V3 evidence from `2026-05-27`:

- V3 local verification: embedded multiseed, optimal stress, and V3 Auto gates
  passed.
- V3 Auto tail gate: `7/7` fixed depth-15 cases solved under the `12000 ms`
  gate.
- V3 Auto hardening gate: `36/36` fixed cases solved under the configured
  depth-14 and depth-15 gates.
- Highest solver elapsed time observed by the V3 local gates: `9384 ms`.
- Highest wall elapsed time observed in the same Auto tail suite: `10033 ms`.

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
        .maxMemoryBytes = 2ull * 1024 * 1024 * 1024,
        .threads = 0,
        .profile = rubik::SolveProfile::Auto,
        .cachePolicy = rubik::CachePolicy::Auto,
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
- `docs/api-stability-3.0.0.md`: stable API contract.
- `docs/release-3.0.0.md`: release checklist.
- `docs/benchmarks.md`: benchmark suites and gate commands.
````

## Pre-Publish Checklist

- [ ] Confirm the tag points at the same source used for the final archive.
- [ ] Run `scripts/release_check.sh --profile quick --with-v3-auto` one last
      time after all release-note edits are complete.
- [ ] Run `scripts/release_check.sh --profile full --with-benchmarks` if the
      release body keeps full desktop benchmark claims.
- [ ] Run `scripts/release_check.sh --profile full --with-large-local` if
      large-local claims are included in the release body.
- [ ] Regenerate `dist/rubik_cube_library-3.0.0.tar.gz` and its `.sha256`
      checksum.
- [ ] Attach `dist/rubik_cube_library-3.0.0.tar.gz` and its `.sha256`
      checksum to the GitHub Release.
- [x] Confirm the release body does not claim Raspberry Pi or Jetson latency.
