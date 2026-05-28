# Rubik Cube Library

C++20 Rubik's Cube solver library, currently at `5.0.0`.

The library is designed as an optimal 3x3x3 solver for desktop and embedded
targets. The public API is centered on certified optimal solving first, with
room for faster non-optimal modes later.

Release status:

- `SolveMode::Optimal` returns a proven-minimal HTM solution when the result
  status is `Optimal`.
- The main `rubik::Solver` API is intended to be stable.
- `SolveMode::Fast`, phase-1/phase-2 APIs, large local table combinations, and
  environment-variable tuning flags are experimental.
- Raspberry Pi and Jetson latency claims are not published yet; those require
  real hardware validation on the target devices.
- QTM is reserved and not implemented.

## What Is Guaranteed

- Input: 54 stickers in `U R F D L B` face order, each face read left-to-right
  and top-to-bottom.
- Metric: HTM. `R`, `R2`, and `R'` each count as one move.
- Optimal result: `SolveStatus::Optimal` means the returned solution is proven
  minimal under the requested options.
- Validation: sticker format and physical cube constraints are checked before
  solving.

`SolveStatus::Timeout` and `SolveStatus::DepthLimitExceeded` mean no optimal
answer was proven inside the configured limits.

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
        .profile = rubik::SolveProfile::Auto,
        .cachePolicy = rubik::CachePolicy::Auto,
        .threads = 0,
        .maxMemoryBytes = 0,
        .timeout = std::chrono::seconds(30),
    });

    if (result.status == rubik::SolveStatus::Optimal ||
        result.status == rubik::SolveStatus::Solved) {
        std::cout << rubik::formatMoves(result.moves) << "\n";
    }
}
```

`SolveProfile::Auto` is the recommended profile for adaptive certified HTM
optimal solving. Explicit profiles remain available when an application needs a
fixed memory/performance policy.

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The test suite covers the core library, CLI smoke tests, benchmark smoke tests,
and example programs.

Repeatable build presets are available for benchmark and release work:

```sh
cmake --preset release
cmake --build --preset release
ctest --preset release
```

Available presets:

- `debug`: normal debug build.
- `release`: portable optimized build.
- `release-native`: optimized for the current CPU with `-march=native`.
- `release-lto`: portable release build with link-time optimization.
- `release-native-lto`: fastest local benchmark build for the current machine.
- `asan-ubsan`: AddressSanitizer and UndefinedBehaviorSanitizer validation.

Run the local release validation script:

```sh
scripts/release_check.sh --profile standard
```

This validates the configured build presets, standalone install consumption,
source archive contents, and a fresh build/test from the generated archive.
The archive version is read from the CMake project version.

For a release candidate with desktop benchmark gates:

```sh
scripts/release_check.sh --profile full --with-benchmarks
```

The GitHub Actions workflow in `.github/workflows/ci.yml` runs the standard
release validation and sanitizer tests on a clean Ubuntu runner.

Create and validate only the source release archive contents:

```sh
scripts/check_release_archive.sh
```

The archive check reads the default archive version from the CMake project
version and requires the matching versioned release documents, for example
`docs/release-5.0.0.md` and `docs/github-release-v5.0.0.md`. It also writes a
matching `.sha256` checksum file next to the archive. Pass `--version` only when
deliberately validating a different archive label.

## Install

Install the library, headers, CMake package files, and CLI tools:

```sh
cmake --install build --prefix /path/to/install
```

Use it from another CMake project:

```cmake
find_package(rubik CONFIG REQUIRED)

add_executable(my_solver main.cpp)
target_link_libraries(my_solver PRIVATE rubik::rubik)
```

Configure the consumer with:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/install
```

Verify install/export packaging with the standalone consumer smoke test:

```sh
cmake --build build --target rubik-check-install-consumer
```

Disable CLI installation with:

```sh
cmake -S . -B build -DRUBIK_INSTALL_CLI=OFF
```

## CLI

Show command usage:

```sh
./build/rubik-solve --help
./build/rubik-bench --help
./build/rubik-cache-setup --help
```

Show CLI versions:

```sh
./build/rubik-solve --version
./build/rubik-bench --version
./build/rubik-cache-setup --version
```

Read the library version from C++:

```cpp
#include <rubik/version.hpp>

static_assert(rubik::version_major == 5);
std::cout << rubik::version_string << "\n";
```

Solve one cube from a 54-sticker string:

```sh
./build/rubik-solve UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB
```

Useful options:

```sh
./build/rubik-solve <54-stickers> --mode optimal --timeout-ms 30000 --max-depth 20 --profile auto --threads 0
```

Prepare pruning-table cache data before latency-sensitive optimal solving:

```sh
./build/rubik-cache-setup --profile auto
```

Emit machine-readable cache setup rows for scripts:

```sh
./build/rubik-cache-setup --profile auto --dry-run --format csv
```

Use the high-memory local optimal profile explicitly:

```sh
./build/rubik-solve <54-stickers> --mode optimal --profile large-local --threads 4 --max-memory-mb 2048
```

Run the experimental fast mode:

```sh
./build/rubik-solve UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB --mode fast
```

CLI options are parsed strictly. Unknown enum values, missing option values,
and invalid numeric values return a non-zero exit code with a specific error
message.

## Benchmarks

Benchmark tooling is included for reproducible local validation. A small smoke
run:

```sh
./build/rubik-bench --max-depth 7 --timeout-ms 30000
```

Inspect the active optimal profile policy without running benchmark cases:

```sh
./build/rubik-bench --mode optimal --profile large-local --report-policy
```

Release-candidate benchmark gates:

```sh
scripts/release_check.sh --profile full --with-benchmarks
```

Legacy V3 Auto gates remain available for historical adaptive-profile
regression checks:

```sh
scripts/release_check.sh --profile quick --with-v3-auto
```

Large local and adaptive Auto hardening optimal gates are intentionally separate
because they need large tables and can take a long time on a cold cache:

```sh
scripts/release_check.sh --profile full --with-large-local
```

Detailed benchmark suites and output formats are documented in
[Benchmarks](docs/benchmarks.md).

## Examples

Example programs are built by default:

```sh
./build/example-validate-input
./build/example-solve-optimal
./build/example-cache-setup
./build/example-solve-fast
```

`example-solve-optimal` shows the recommended certified solver path:
`SolveMode::Optimal`, `SolveProfile::Auto`, `CachePolicy::Auto`, and automatic
thread planning with `threads = 0`. `example-cache-setup` uses the public
`prepareCache()` API. `example-solve-fast` demonstrates the experimental
non-optimal mode and prints `non_optimal: true`.

Disable them with:

```sh
cmake -S . -B build -DRUBIK_BUILD_EXAMPLES=OFF
```

## Documentation

Additional technical notes live in `docs/`:

- [Architecture](docs/architecture.md)
- [API Notes](docs/api.md)
- [Benchmarks](docs/benchmarks.md)
- [Runtime Behavior](docs/runtime.md)
- [Versioning And API Stability](docs/versioning.md)
- [Embedded Fast Tail-Case Diagnostics - 2026-05-25](docs/embedded-fast-tail-cases-2026-05-25.md)
- [Embedded Multiseed Benchmark - 2026-05-25](docs/benchmark-embedded-multiseed-2026-05-25.md)
- [Benchmark Gate Calibration - 2026-05-25](docs/benchmark-gate-calibration-2026-05-25.md)
- [Profile Realistic Large Benchmark - 2026-05-25](docs/benchmark-profile-realistic-large-2026-05-25.md)
- [Profile Realistic Benchmark - 2026-05-25](docs/benchmark-profile-realistic-2026-05-25.md)
- [Embedded Fast Failure Diagnostics - 2026-05-25](docs/embedded-fast-failures-2026-05-25.md)
- [Local Optimal Profiles](docs/local-optimal-profiles.md)
- [Random Fast Benchmark - 2026-05-25](docs/benchmark-random-fast-2026-05-25.md)
- [Roadmap](docs/roadmap.md)

Current versioned release documents:

- [API Stability - 5.0.0](docs/api-stability-5.0.0.md)
- [Release Checklist - 5.0.0](docs/release-5.0.0.md)

Historical versioned release documents:

- [API Stability - 4.0.0](docs/api-stability-4.0.0.md)
- [API Stability - 3.0.0](docs/api-stability-3.0.0.md)
- [API Stability - 2.0.0](docs/api-stability-2.0.0.md)
- [API Stability - 1.0.0](docs/api-stability-1.0.0.md)
- [Release Checklist - 4.0.0](docs/release-4.0.0.md)
- [Release Checklist - 3.0.0](docs/release-3.0.0.md)
- [Release Checklist - 2.0.0](docs/release-2.0.0.md)
- [Release Checklist - 1.0.0](docs/release-1.0.0.md)

Release notes are tracked in [CHANGELOG.md](CHANGELOG.md).

## License

Apache License 2.0. See [LICENSE](LICENSE) and [NOTICE](NOTICE).

## Table Cache

Pruning tables are cached as binary files. By default the cache lives under the
system temporary directory. Set `RUBIK_TABLE_CACHE_DIR` to control the location:

```sh
RUBIK_TABLE_CACHE_DIR=/path/to/cache ./your_solver
```

The active cache directory is available at runtime:

```cpp
std::string path = rubik::pruning_tables::cacheDirectory();
```

Thread-safety expectations and cache compatibility rules are documented in
[Runtime Behavior](docs/runtime.md).

## API

```cpp
#include <rubik/cube.hpp>
#include <rubik/move.hpp>
#include <rubik/solver.hpp>

auto cubeResult = rubik::Cube::fromStickers(
    "UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB");

if (!cubeResult) {
    // cubeResult.error.code and cubeResult.error.message explain the issue.
    return;
}

rubik::Solver solver;
rubik::SolveResult result = solver.solve(cubeResult.cube, {
    .mode = rubik::SolveMode::Optimal,
    .metric = rubik::Metric::HTM,
    .maxDepth = 20,
    .timeout = std::chrono::seconds(10),
    .maxMemoryBytes = 1024ull * 1024 * 1024,
    .threads = 1,
    .profile = rubik::SolveProfile::Default,
});

if (result.status == rubik::SolveStatus::Optimal ||
    result.status == rubik::SolveStatus::Solved) {
    auto text = rubik::formatMoves(result.moves);
}
```

The two-phase building blocks are also exposed for experimentation:

```cpp
#include <rubik/experimental/phase1.hpp>
#include <rubik/experimental/phase2.hpp>

rubik::experimental::Phase1Result phase1 = rubik::experimental::solvePhase1(cube);
cube.apply(phase1.moves);

if (rubik::experimental::isPhase1Solved(cube)) {
    rubik::experimental::Phase2Result phase2 = rubik::experimental::solvePhase2(cube);
}
```

`solvePhase1` moves a cube into the G1 subgroup where corner orientation, edge
orientation, and slice-edge placement are solved. `solvePhase2` expects a cube
already in that subgroup and uses only the phase-2 move set.

For fast-mode experimentation, phase 1 can also enumerate several G1 candidates:

```cpp
rubik::experimental::Phase1CandidatesResult candidates =
    rubik::experimental::findPhase1Candidates(cube, {
    .maxDepth = 12,
    .maxCandidates = 16,
});
```

The phase APIs are experimental. The stable high-level entry point is
`rubik::Solver`.

`SolveStatus::Optimal` and `SolveStatus::Solved` mean the returned solution is
proven minimal under the requested metric within the configured depth and
timeout.

The primary sticker input order is `U R F D L B`, with each face read left to
right and top to bottom. The default metric is HTM, where `R`, `R2`, and `R'`
each count as one move.

`Cube::fromStickers` validates both the sticker format and the physical cube
constraints: centers, corner cubies, edge cubies, orientation sums, and
permutation parity.

Implementation details and solver direction are documented in
[Architecture](docs/architecture.md), [Optimal Solver Design](docs/optimal-design.md),
and [Roadmap](docs/roadmap.md).
