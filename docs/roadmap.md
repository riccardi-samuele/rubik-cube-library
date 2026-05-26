# Roadmap

The project goal is a C++20 Rubik's Cube solver library that is readable,
embeddable, and strong enough to become a serious default choice for applications
that need 3x3x3 solving.

## Current Status

Implemented:

- sticker input and validation;
- cubie representation;
- move parsing and formatting;
- coordinate move tables;
- disk-cached pruning tables;
- optimal HTM IDA* solver with default corner/edge orientation combined
  pruning;
- experimental two-phase fast solver;
- CLI solver;
- CLI benchmark runner;
- profile-smoke, profile-realistic, embedded-multiseed, and optimal-stress
  benchmark suites;
- example programs;
- install/export packaging for downstream CMake projects;
- three-direction phase-1 optimal lower bound for all optimal profiles,
  including embedded;
- experimental full corner-state optimal pruning table with passing depth-13
  stress gates and improved depth-14 frontier behavior;
- experimental large local optimal profile using corner/edge-group pruning and
  root-parallel search to solve twenty-four sampled depth-15 frontier cases
  under 30s on the development desktop;
- experimental 8-thread large local tail replay that keeps the six slowest
  sampled depth-15 cases under 18s on the development desktop;
- optimized CMake presets for release, native, LTO, and sanitizer builds;
- local optimal profile contract for embedded, default, and performance targets;
- unit tests.

## Near-Term Engineering Steps

Release checklist:

- [Release Checklist - 1.0.0](release-1.0.0.md)

1. Continue stabilizing benchmark suites for fast, optimal, cold-cache,
   warm-cache, and profile-specific runs.
2. Keep `maxMemoryBytes` enforcement aligned with `rubik-bench --report-memory`
   as table profiles evolve.
3. Strengthen optimal-mode admissible pruning and track the maximum solved
   deterministic depth within the target timeout.
4. Improve optimal depth-15 tail behavior before promoting the corner-state
   table into a default profile policy.
5. Keep the stable public API source-compatible while hardening experimental
   APIs.
6. Add large random solve verification and regression tests for known slow
   cases.
7. Keep memory, cache size, cache warm-up, and table compatibility reports
   profile-specific.
8. Broaden `Embedded/Fast` random depth-20 regression coverage beyond the first
   two targeted tail cases.
9. Run Raspberry Pi and Jetson-class benchmarks when hardware is available.
10. Complete user-facing documentation for input format, API usage, and
    downstream CMake integration.
11. Prepare packaging, semantic versioning, and release archives.
12. Run pre-release validation before publishing `1.0.0`.

## Active Benchmark Suites

Current script entry point:

```sh
scripts/run_benchmark_suite.sh --suite smoke
```

Available suites:

- `smoke`: fast random 5 plus optimal deterministic depth 7;
- `profile-smoke`: short embedded/default/performance profile sweep;
- `profile-realistic`: calibrated embedded/default/performance profile sweep;
- `embedded-multiseed`: embedded fast and optimal robustness sweep across
  multiple random seeds;
- `optimal-stress`: optimal depth-13 stress sweep across all public profiles
  and multiple random seeds;
- `optimal-tail-cases`: replay the current slowest embedded optimal stress
  cases with lower-bound diagnostics;
- `optimal-deep-probe`: non-gated optimal depth-14/depth-15 frontier probe with
  lower-bound diagnostics;
- `optimal-large-local`: performance-profile depth-15 probe for desktop/Orin
  class configurations with large pruning tables and root-parallel search;
- `embedded-fast-tail-cases`: replay the current slowest embedded fast cases
  with phase diagnostics;
- `fast-100`: reproducible fast random 100 depth-20 benchmark;
- `fast-1000`: longer fast random 1000 depth-20 benchmark;
- `optimal-depth`: deterministic optimal benchmark through depth 13;
- `tail-diagnostics`: diagnostic runs for the current slowest random cases;
- `all`: smoke, profile-smoke, profile-realistic, embedded-multiseed,
  optimal-stress, optimal-tail-cases, optimal-deep-probe,
  embedded-fast-tail-cases,
  embedded-fast-failures, fast-100, optimal-depth, and tail-diagnostics.

## API Release Decisions

Before the first stable release:

1. Decide whether compatibility aliases for phase APIs stay after the first
   stable release.
2. Use benchmark `slowest` and `diagnostic_phase*` reports to target
   tail-latency optimizations.
3. Add more compact phase-2-specific coordinates if random benchmarks need
   stronger pruning.

## Phase-2 Pruning Candidates

Implemented candidates:

- corner permutation + slice edge permutation;
- U-edge permutation + slice edge permutation;
- D-edge permutation + slice edge permutation;

Remaining candidates:

- compact phase-2-only edge permutation coordinates.

The goal is to reduce phase-2 node expansion enough that trying several phase-1
candidates is cheap.

## Embedded Work

The Raspberry Pi 4 target has 4 GB of RAM, but the working memory budget for the
library should stay around 1 GB unless explicitly configured otherwise.

Needed:

- `Embedded` profile benchmark on actual Raspberry Pi hardware;
- `Performance` profile benchmark on Jetson Orin class hardware when available;
- table size report and per-profile logical RAM report;
- cache warm-up report;
- memory peak measurement;
- separate default limits for optimal and fast mode.

The current local profile contract is documented in
[Local Optimal Profiles](local-optimal-profiles.md).

## Public API Hardening

Before treating the library as stable:

- freeze sticker order and error codes;
- document thread-safety expectations;
- document table cache compatibility/versioning;
- decide whether `phase1` and `phase2` compatibility aliases remain public;
- add semantic versioning rules;
- add examples and integration tests.

## Long-Term Solver Goals

Optimal mode:

- implement the symmetry foundation described in
  [Optimal Solver Design](optimal-design.md);
- strengthen admissible pruning beyond the current triple-direction phase-1
  bound for depth 14+;
- better memory-profile selection;
- eventual parallel search support;
- QTM support if needed.

Fast mode:

- robust two-phase solver;
- bounded-latency profile for embedded devices;
- quality/speed tuning knobs;
- large random benchmark validation.
