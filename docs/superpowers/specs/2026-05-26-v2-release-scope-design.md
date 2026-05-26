# Rubik Cube Library 2.0 Release Scope Design

## Purpose

Version `2.0.0` is the first long-term public contract for the library after
the initial stable release. The release should strengthen the local optimal
solver, benchmark reproducibility, and public API clarity without expanding the
project into unrelated product areas.

The library remains a local C++ Rubik's Cube solving library. Public
documentation, release notes, and code comments must describe the library and
measured behavior only.

## Scope

The `2.0.0` scope is:

- center the public contract on certified optimal solving;
- preserve or improve the simple `Cube -> Solver -> SolveResult` path;
- keep `SolveMode::Optimal` and `SolveStatus::Optimal` as the strongest public
  guarantee;
- keep local profiles explicit: `Embedded`, `Default`, `Performance`, and
  `LargeLocal`;
- document benchmark and release gates that can be reproduced locally;
- keep unproven hardware claims out of public release material;
- isolate experimental APIs and implementation details.

The `2.0.0` scope is not:

- cloud solving;
- camera recognition;
- robot or hardware control;
- application UI;
- Raspberry Pi, Jetson Nano, or Jetson Orin performance claims before direct
  measurement on those devices;
- GPU or accelerator work unless it becomes necessary for the local solver
  contract.

## API Contract

The stable V2 surface should remain small:

- `rubik::Cube`
- `rubik::CubeParseResult`
- `rubik::CubeError`
- `rubik::CubeErrorCode`
- `rubik::Move` and move parsing/formatting helpers
- `rubik::Solver`
- `rubik::SolveOptions`
- `rubik::SolveResult`
- `rubik::SolveBoundDiagnostics`
- `rubik::SolveMode`
- `rubik::SolveProfile`
- `rubik::SolveStatus`
- `rubik/version.hpp`
- the CMake package target `rubik::rubik`
- the 54-sticker input format in `U R F D L B` face order

Small breaking changes are allowed only if they improve the long-term contract.
The release should not rename or redesign stable types merely for cosmetic
cleanup.

## Optimal Solving Contract

`SolveMode::Optimal` is the primary library mode.

When `Solver::solve` returns `SolveStatus::Optimal`, the solution is a
proven-minimal HTM solution under the requested options. Performance work may
change pruning tables, move ordering, profiles, or parallel search strategy, but
it must not weaken the optimality proof.

`Metric::HTM` remains the implemented metric for `2.0.0`. `Metric::QTM` remains
reserved unless fully implemented and tested.

## Fast Mode Contract

`SolveMode::Fast` remains available as a practical non-optimal mode, but it is
not the center of the V2 public contract.

Documentation and examples must clearly separate:

- `Optimal`: certified minimum-move result when status is `Optimal`;
- `Fast`: useful non-optimal result when status is `Found`.

Fast mode internals and quality heuristics remain experimental unless a future
release defines a stronger quality contract.

## Experimental Surface

The following remain experimental or internal for `2.0.0`:

- phase-1 and phase-2 APIs;
- everything under `rubik::experimental`;
- coordinate, move-table, pruning-table, and symmetry internals;
- environment-variable tuning flags;
- benchmark random-case generation details;
- large-local table combinations as implementation details.

Experimental APIs may be refined after V2 without the same compatibility
expectations as the stable solver surface.

## Profiles And Local Hardware Policy

V2 keeps these public profiles:

- `Embedded`: conservative local profile for smaller memory budgets;
- `Default`: normal desktop/server profile;
- `Performance`: local profile for stronger throughput where available;
- `LargeLocal`: high-memory local profile for deeper optimal work.

Profile behavior must be documented in terms of memory budget, thread count,
cache state, and measured local benchmark data. Public docs must not estimate
Raspberry Pi, Jetson Nano, or Jetson Orin performance until those devices are
tested directly.

## Benchmark And Release Gates

Before publishing `2.0.0`, the release must pass:

- the full release validation profile;
- profile-realistic benchmark gates;
- embedded-multiseed benchmark gates;
- optimal-stress benchmark gates;
- large-local gates if `LargeLocal` is part of the highlighted release value;
- source archive creation and archive rebuild validation;
- checksum generation for release artifacts;
- public documentation checks for unverified hardware claims.

Benchmark reports must state:

- build preset;
- cache mode;
- profile;
- mode;
- thread count;
- memory budget when relevant;
- host hardware when the report includes timing data.

## Documentation Updates

The implementation plan should update or add:

- `CHANGELOG.md` entry for `2.0.0`;
- `docs/api.md` for the V2 public contract;
- a V2 API stability document;
- `docs/release-2.0.0.md`;
- `docs/github-release-v2.0.0.md`;
- any version references required by archive validation;
- release notes that describe measured library behavior only.

The V2 docs should avoid internal planning language and personal context.

## Success Criteria

V2 is ready to publish when:

- `SolveStatus::Optimal` remains a proven-minimal HTM guarantee;
- the public API contract is documented and intentionally scoped;
- benchmark gates pass on the local development machine;
- release validation passes from the source archive;
- public docs contain no unverified hardware claims;
- generated or temporary artifacts are not committed;
- the project version, changelog, release notes, and archive metadata agree.
