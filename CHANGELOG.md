# Changelog

All notable user-facing changes are tracked here.

This project uses semantic versioning for public releases. The main
`rubik::Solver` API is treated as stable, while explicitly experimental APIs
can still change between minor releases.

## 3.0.0 - 2026-05-27

Third stable release scope:

- Make `SolveProfile::Auto` the recommended optimal profile for adaptive local
  HTM optimal solving.
- Keep the V2 certified optimality contract: `SolveStatus::Optimal` means the
  returned HTM solution is proven minimal for the requested options.
- Add `SolvePlan` reporting so callers can inspect the effective profile,
  cache policy, thread count, memory budget, and optimal move-ordering policy
  selected for a solve.
- Add cache preparation support through `prepareCache()` and the
  `rubik-cache-setup` CLI.
- Add repeatable V3 Auto benchmark gates for shallow Auto solves, fixed
  depth-15 Auto tail cases, and Auto hardening cases.
- Promote selective Auto optimal move ordering for measured tail cases while
  keeping environment-variable overrides available for experiments.
- Document V3 local verification results from the development desktop.
- Keep Raspberry Pi, Jetson Nano, and Jetson Orin performance claims out of
  public release notes until direct hardware measurements are available.

Known 3.0 limits:

- QTM is not implemented.
- `SolveMode::Fast` does not guarantee optimality.
- Raspberry Pi, Jetson Nano, and Jetson Orin latency claims are pending real
  hardware tests.
- `SolveProfile::LargeLocal` and Auto large-local selections have a high memory
  footprint and are not guaranteed to fit every local target.
- GPU acceleration is not implemented.
- Experimental APIs and environment-variable tuning flags may change in future
  releases.

## 2.0.0 - 2026-05-26

Second stable release scope:

- Strengthen the public contract around certified HTM optimal solving.
- Document the `2.0.0` API stability contract.
- Promote the corner-state admissible pruning bound into default optimal
  profiles after benchmark validation.
- Tighten optimal benchmark gates for profile-realistic, embedded-multiseed,
  and optimal-stress suites.
- Add V2 optimal baseline and corner-state validation reports.
- Add large-local depth-15 validation for high-memory local optimal solving.
- Keep `SolveMode::Fast` available as a practical non-optimal mode while
  documenting that it is not a certified optimal mode.
- Keep Raspberry Pi, Jetson Nano, and Jetson Orin performance claims out of
  public release notes until direct hardware measurements are available.

Known 2.0 limits:

- QTM is not implemented.
- `SolveMode::Fast` does not guarantee optimality.
- Raspberry Pi, Jetson Nano, and Jetson Orin latency claims are pending real
  hardware tests.
- `SolveProfile::LargeLocal` has a high memory footprint and is not the
  embedded default policy.
- Experimental APIs may change in future releases.

## 1.0.0 - 2026-05-26

First stable release scope:

- Add a C++20 3x3x3 Rubik's Cube solving library.
- Add 54-sticker input parsing in `U R F D L B` order.
- Validate physical cube constraints: centers, cubies, orientations, and parity.
- Add HTM move parsing, formatting, and application.
- Add `rubik::Solver` with `SolveMode::Optimal` and certified minimal results
  when the solver returns `SolveStatus::Optimal`.
- Add `SolveProfile::Embedded`, `Default`, `Performance`, and `LargeLocal`
  profiles.
- Add root-level thread parallelism for optimal search through
  `SolveOptions::threads`.
- Add public C++ version metadata through `rubik/version.hpp`.
- Add experimental two-phase and fast solving APIs.
- Add pruning-table cache support through `RUBIK_TABLE_CACHE_DIR`.
- Add CLI tools: `rubik-solve` and `rubik-bench`, including `--help` and
  `--version`.
- Add CMake install/export package support with `rubik::rubik`.
- Add reproducible benchmark suites and benchmark gates.
- Add benchmark metadata rows, including solver profile and binary version.
- Document the large local optimal profile for high-memory local testing.
- Document the `1.0.0` public API stability contract.
- License the project under Apache License 2.0 with a repository `NOTICE` file.

Known 1.0 limits:

- QTM is not implemented.
- `SolveMode::Fast` is experimental and does not guarantee optimality.
- Raspberry Pi and Jetson performance claims are pending real hardware tests.
- Large local optimal acceleration has a high memory footprint and is not the
  embedded default policy.
- Experimental APIs may change in future releases.
