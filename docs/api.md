# API Notes

The API is intentionally small while the solver internals are still evolving.
The `1.0.0` compatibility contract is documented in
[API Stability - 1.0.0](api-stability-1.0.0.md).

## Sticker Input

The primary input format is a 54-character string in face order:

```text
U R F D L B
```

Each face is read left-to-right, top-to-bottom.

Example solved cube:

```text
UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB
```

Parsing:

```cpp
auto parsed = rubik::Cube::fromStickers(stickers);
if (!parsed) {
    // parsed.error.code and parsed.error.message explain the failure.
}
```

Validation includes:

- sticker count
- sticker symbols
- color counts
- centers
- corner cubies
- edge cubies
- corner orientation sum
- edge orientation sum
- permutation parity

## Main Solver

```cpp
rubik::Solver solver;
rubik::SolveResult result = solver.solve(cube, {
    .mode = rubik::SolveMode::Optimal,
    .metric = rubik::Metric::HTM,
    .maxDepth = 20,
    .timeout = std::chrono::seconds(30),
    .maxMemoryBytes = 1024ull * 1024 * 1024,
    .threads = 1,
    .profile = rubik::SolveProfile::Default,
});
```

Important statuses:

- `Solved`: input cube is already solved.
- `Optimal`: returned solution is proven minimal.
- `Found`: returned solution is valid but not proven minimal.
- `Timeout`: no result before the timeout.
- `MemoryLimitExceeded`: estimated table payload and solver overhead exceed
  `maxMemoryBytes`.
- `DepthLimitExceeded`: no result within `maxDepth`.
- `InvalidCube`: input is not a valid physical cube.
- `UnsupportedOptions`: requested mode or metric is not implemented.

## Modes

`SolveMode::Optimal` uses admissible IDA* search and returns `Optimal` only when
the solution length is proven minimal.

`SolveMode::Fast` uses an experimental two-phase path followed by a beam-search
fallback. It returns `Found` for valid non-optimal solutions.

`SolveMode::Balanced` is reserved and currently unsupported.

## Profiles

Profiles tune memory use, table selection, and latency. They do not change the
meaning of `SolveStatus::Optimal`: an optimal result is still a proven-minimal
HTM solution for every profile.

`SolveProfile::Default` is the normal desktop/server profile.
`SolveProfile::Performance` may use larger or more expensive internal search
helpers when they are available. `SolveProfile::LargeLocal` is an optimal-only
desktop/Orin-class profile that enables the largest local admissible pruning
bounds without environment flags. `SolveProfile::Embedded` keeps the solver
more conservative for smaller devices.

For `SolveMode::Optimal`, all public profiles enable the three-direction
phase-1 lower bound by default, including `Embedded`. For benchmark A/B work,
`RUBIK_DISABLE_THREE_PHASE1_BOUNDS=1` forces the older optimal baseline.

The local hardware and profile contract is documented in
[Local Optimal Profiles](local-optimal-profiles.md).

`maxMemoryBytes` is checked before search starts. The check uses the active
profile's pruning-table payload estimate plus small solver overhead. It is a
conservative logical budget, not a full process RSS measurement.

`threads` controls root-level parallelism in `SolveMode::Optimal`. Values above
1 can reduce wall time on desktop/Orin-class CPUs while preserving the same
optimality proof. Small embedded targets should usually keep `threads = 1`
unless benchmarked on the actual hardware.

Runtime thread-safety expectations are documented in
[Runtime Behavior](runtime.md).

## Metrics

`Metric::HTM` is implemented. Under HTM, each face turn counts as one move:

- `R`: 1
- `R2`: 1
- `R'`: 1

`Metric::QTM` is reserved and currently unsupported.

## Two-Phase Experimental API

The two-phase API is experimental. Prefer these explicit headers for new code:

```cpp
#include <rubik/experimental/phase1.hpp>
#include <rubik/experimental/phase2.hpp>
```

The same types and functions are currently also available through
`rubik/phase1.hpp` and `rubik/phase2.hpp` for compatibility. The preferred
headers for new code remain under `rubik/experimental/`.

Phase 1:

```cpp
rubik::experimental::Phase1Result phase1 = rubik::experimental::solvePhase1(cube, {
    .maxDepth = 12,
    .timeout = std::chrono::seconds(2),
    .profile = rubik::SolveProfile::Default,
});
```

Multiple phase-1 candidates:

```cpp
rubik::experimental::Phase1CandidatesResult candidates =
    rubik::experimental::findPhase1Candidates(cube, {
    .maxDepth = 12,
    .timeout = std::chrono::seconds(2),
    .profile = rubik::SolveProfile::Default,
    .maxCandidates = 16,
});
```

Phase 2:

```cpp
if (rubik::experimental::isPhase1Solved(cube)) {
    rubik::experimental::Phase2Result phase2 = rubik::experimental::solvePhase2(cube);
}
```

The phase APIs are exposed for experimentation. They may change faster than the
main `Solver` API.

## Stability Policy

The detailed `1.0.0` freeze is documented in
[API Stability - 1.0.0](api-stability-1.0.0.md).

Currently intended stable surface:

- `rubik::Cube`
- `rubik::CubieCube`
- `rubik::Move`
- `rubik::Solver`
- `rubik::SolveOptions`
- `rubik::SolveResult`
- `rubik::SolveBoundDiagnostics`
- `rubik::CubeError`
- CLI input order `U R F D L B`

Currently experimental surface:

- phase-1 and phase-2 APIs;
- pruning table internals;
- coordinate APIs;
- benchmark case generation details;
- environment-variable tuning flags;
- large-local optimal table combinations;
- `SolveMode::Fast` internals and solution quality.

## Examples

The repository includes small compileable examples:

- `examples/validate_input.cpp`
- `examples/solve_fast.cpp`
- `examples/solve_optimal.cpp`
- `examples/cache_setup.cpp`

They are built by default when `RUBIK_BUILD_EXAMPLES` is enabled.

The examples are also registered as CTest smoke tests. The CLI applications have
CTest coverage for solved input, invalid input, `--help`, strict option parsing,
deterministic benchmark output, random benchmark generation, symmetry reports,
lower-bound benchmark reports, cache/memory reports, and optimal-mode bound
diagnostics.

## CMake Package

Installed builds export a CMake package named `rubik`.

Consumer example:

```cmake
find_package(rubik CONFIG REQUIRED)

add_executable(my_solver main.cpp)
target_link_libraries(my_solver PRIVATE rubik::rubik)
```

The install tree contains:

- `include/rubik/...`
- `lib/librubik.a`
- `lib/cmake/rubik/rubikConfig.cmake`
- `lib/cmake/rubik/rubikConfigVersion.cmake`
- `lib/cmake/rubik/rubikTargets.cmake`
- `bin/rubik-solve` and `bin/rubik-bench` when `RUBIK_INSTALL_CLI=ON`
