# Changelog

All notable user-facing changes are tracked here.

This project uses semantic versioning for public releases. The main
`rubik::Solver` API is treated as stable, while explicitly experimental APIs
can still change between minor releases.

## Unreleased

- Added experimental blocked-face support to fast solving. Library callers can
  pass one blocked face or one opposite blocked pair through
  `SolveOptions::blockedFaces`.
- Added the same blocked-face restriction support to the experimental phase-1
  and phase-2 APIs.
- Extended `rubik-solve` with `--blocked-faces FACE[,FACE]` for fast mode and
  tightened CLI coverage for accepted and rejected blocked-face combinations.
- Updated README and API notes for fast blocked-face solving.

## 6.0.0 - 2026-06-01

- Promoted measured V6 conservative root-ordering policy for local optimal
  solving.
- Kept the certified optimality contract unchanged:
  `SolveStatus::Optimal` means the returned HTM solution is proven minimal for
  the requested options.
- Added V6 transition-corpus extraction and replay aggregation tooling for
  benchmark-driven policy validation.
- Documented V6 local benchmark evidence and release notes.
- Kept Raspberry Pi, Jetson, Orin, GPU, and cloud performance claims out of
  public release notes until direct measurements are available.

## 5.0.0 - 2026-05-28

- Modernized public examples around the recommended certified optimal path.
- Moved the cache setup example to the public `prepareCache()` API.
- Clarified the experimental non-optimal fast-mode example.
- Added release validation for stale public examples.
- Updated public documentation and release notes for V5.

## 4.0.0 - 2026-05-28

Fourth stable release scope:

- Promote adaptive deep-split scheduling for local large-table optimal solves
  after V4 corpus validation.
- Keep the certified optimality contract: `SolveStatus::Optimal` means the
  returned HTM solution is proven minimal for the requested options.
- Add V4 tail discovery, corpus replay, and three-way comparison tooling for
  baseline, unconditional deep split, and adaptive deep split.
- Extend V4 comparison output with max elapsed fields.
- Add adaptive scheduler diagnostics to `SolvePlan::rootOrderingProfile`.
- Document V4 local benchmark results from the development desktop.
- Keep Raspberry Pi, Jetson, Orin, GPU, cloud, and other external hardware
  performance claims out of public release notes until direct measurements are
  available.

Known 4.0 limits:

- QTM is not implemented.
- `SolveMode::Fast` does not guarantee optimality.
- Unconditional deep splitting remains rejected because it regressed average
  latency in the measured corpus.
- Raspberry Pi, Jetson, Orin, GPU, and cloud latency claims require direct
  measurements before publication.
- Large local optimal acceleration has a high memory footprint and is not the
  embedded default policy.
- Experimental environment-variable tuning flags may change in future releases.

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
