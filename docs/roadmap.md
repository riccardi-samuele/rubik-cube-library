# Roadmap

The project goal is a C++20 Rubik's Cube solver library that is readable,
embeddable, and strong enough to become a serious default choice for
applications that need 3x3x3 solving.

The `1.0.0` release established the stable public base: sticker input,
physical validation, HTM moves, certified optimal solving, benchmark tooling,
CMake packaging, and release validation. The next major release is focused on
making the optimal path faster, easier to integrate, and easier to validate
across local hardware profiles.

`2.0.0` added stronger local optimal profiles, benchmark gates, and release
validation. `3.0.0` made the adaptive Auto optimal path the recommended local
profile and added cache setup workflows for applications that need predictable
startup behavior. The current V4 workstream focuses on CPU-only optimal
tail-latency reduction.

## Current Status

Implemented:

- sticker input and validation in `U R F D L B` order;
- cubie representation;
- move parsing and formatting;
- coordinate move tables;
- disk-cached pruning tables;
- optimal HTM IDA* solver with certified minimal solutions when the result
  status is `SolveStatus::Optimal`;
- embedded, default, and performance solving profiles;
- adaptive `Auto` optimal profile planning;
- public solve-plan diagnostics;
- cache setup API and `rubik-cache-setup` CLI;
- experimental two-phase fast solver;
- root-level thread parallelism for optimal search;
- CLI solver;
- CLI benchmark runner;
- V4 CPU tail discovery, replay, and A/B comparison tooling;
- profile-smoke, profile-realistic, Auto, embedded-multiseed, optimal-stress,
  Auto tail, and large local benchmark suites;
- example programs;
- install/export packaging for downstream CMake projects;
- optimized CMake presets for release, native, LTO, and sanitizer builds;
- public API stability documentation for `1.0.0`, `2.0.0`, and `3.0.0`;
- unit tests and release validation scripts.

Experimental or hardware-dependent:

- `SolveMode::Fast`;
- phase-1 and phase-2 APIs;
- large local optimal profile table combinations;
- environment-variable tuning flags;
- Raspberry Pi, Jetson Nano, and Jetson Orin latency claims.

Hardware-specific performance claims stay unpublished until they are measured
on the real target devices.

## Road To 4.0

The `4.0` target is a CPU-only optimal tail-latency release. It should reduce
the slowest local `SolveMode::Optimal` cases without weakening the certified
minimum-move contract.

Primary goals:

1. Record a clean V3 baseline before solver changes.
2. Discover a broader deterministic set of difficult depth-15 optimal cases.
3. Promote slow cases into a replayable V4 tail corpus.
4. Compare candidate optimizations against the baseline with A/B tooling.
5. Improve CPU root scheduling, root ordering, or Auto policy only when
   benchmark data supports the change.

Out of scope for `4.0`:

- GPU acceleration;
- cloud solving;
- QTM;
- camera recognition;
- hardware control;
- unmeasured hardware performance claims.

## Road To 3.0

The `3.0` target is an adaptive optimal-engine release. It should make the
recommended optimal path easier to use without weakening the certified
minimum-move contract.

Primary goals:

1. Make `SolveProfile::Auto` the recommended optimal profile.
2. Keep explicit profiles available for fixed memory/performance policies.
3. Report the effective plan through `SolvePlan`.
4. Provide `prepareCache()` and `rubik-cache-setup` for cache preparation.
5. Keep benchmark gates local and reproducible without unmeasured hardware
   claims.

Out of scope for `3.0`:

- QTM;
- cloud solving;
- camera recognition;
- hardware control;
- GPU backends;
- unmeasured hardware performance claims.

## Road To 2.0

The `2.0` target is a stronger local solver release, not a product expansion.
The scope should stay centered on the library itself.

Primary goals:

1. Improve optimal-mode throughput and tail latency while preserving the
   certified minimum-move guarantee.
2. Make benchmark results easier to reproduce and compare between releases.
3. Strengthen public API ergonomics without breaking the stable `1.0` core
   unnecessarily.
4. Keep embedded and performance profiles explicit, measurable, and local.
5. Promote only proven experimental pieces into stable API or stable defaults.

Out of scope for `2.0`:

- cloud solving;
- camera recognition;
- hardware control;
- application UI;
- unmeasured hardware performance claims.

## 2.0 Workstreams

### 1. Optimal Solver

Planned:

- reduce node expansion in hard optimal cases;
- improve move ordering for deeper frontiers;
- make root-parallel search behavior deterministic and documented;
- evaluate which large pruning combinations should become stable profiles;
- keep all pruning admissible for `SolveMode::Optimal`;
- add regression cases for known slow optimal tails.

Success criteria:

- `SolveStatus::Optimal` still means proven-minimal HTM solution;
- no performance improvement may weaken optimality;
- benchmark gates cover normal, stress, and selected tail cases.

### 2. Benchmark And Regression Framework

Planned:

- keep benchmark suites reproducible by seed and profile;
- add version-to-version comparison output;
- keep CSV output stable for automation;
- separate smoke, release, stress, and long local runs;
- document cache state, memory profile, thread count, and table profile in
  benchmark reports;
- avoid publishing numbers for hardware that has not been tested directly.

Success criteria:

- a release candidate can be validated locally with one documented command;
- slow cases can be replayed deterministically;
- benchmark reports are useful as reproducible library evidence.

### 3. Public API Ergonomics

Planned:

- review `SolveResult` for richer timing, depth, profile, and diagnostic
  fields that are useful to library users;
- keep error reporting stable and explicit;
- document thread-safety expectations;
- document table-cache compatibility and invalidation rules;
- decide which experimental aliases remain public;
- add or refine examples around common integration patterns.

Success criteria:

- the common solve path remains small and readable;
- advanced options are available without making basic use noisy;
- source compatibility with `1.0` is preserved unless a major-version break is
  justified and documented.

### 4. Local Hardware Profiles

Planned:

- maintain `Embedded`, `Default`, and `Performance` profiles;
- keep memory limits explicit through `SolveOptions::maxMemoryBytes`;
- make table sizes and cache warm-up behavior visible;
- tune profile defaults for local execution first;
- defer Raspberry Pi, Jetson Nano, and Jetson Orin claims until real hardware
  measurements are available.

Success criteria:

- users can choose a profile based on memory and latency needs;
- profile behavior is documented and testable;
- unsupported claims do not appear in public release notes.

### 5. Fast Mode

Planned:

- keep `SolveMode::Fast` experimental until quality and latency are stable;
- strengthen random-depth regression coverage;
- expose quality/speed tradeoffs only when they are well defined;
- ensure fast mode never gets confused with certified optimal mode.

Success criteria:

- fast mode is useful as a practical non-optimal option;
- documentation clearly separates fast and optimal guarantees.

## Candidate 2.x Follow-Ups

After `2.0`, likely candidates are:

- `2.1`: CPU performance and profile tuning;
- `2.2`: C API and Python bindings;
- `2.3`: experimental GPU or accelerator backend if benchmarks justify it;
- `2.4`: additional metrics such as QTM if there is clear demand.

Bindings and GPU work should not block `2.0` unless they become necessary for
the core library contract.

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
- `optimal-large-local`: public large-local depth-15 probe for high-throughput
  local configurations with large pruning tables and root-parallel search;
- `embedded-fast-tail-cases`: replay the current slowest embedded fast cases
  with phase diagnostics;
- `fast-100`: reproducible fast random 100 depth-20 benchmark;
- `fast-1000`: longer fast random 1000 depth-20 benchmark;
- `optimal-depth`: deterministic optimal benchmark through depth 13;
- `tail-diagnostics`: diagnostic runs for the current slowest random cases;
- `all`: smoke, profile-smoke, profile-realistic, Auto tail,
  embedded-multiseed, optimal-stress, optimal-tail-cases, optimal-deep-probe,
  embedded-fast-tail-cases, embedded-fast-failures, fast-100, optimal-depth,
  and tail-diagnostics.

## Release Discipline

Before publishing a new version:

1. Run the appropriate release validation profile.
2. Run benchmark gates relevant to the release scope.
3. Verify the source archive excludes generated local artifacts.
4. Verify public documentation only describes the library and measured facts.
5. Update changelog, version references, and release notes.
6. Create and validate the source archive.
