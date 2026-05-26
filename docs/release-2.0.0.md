# Release Checklist - 2.0.0

This checklist tracks the remaining work before publishing `2.0.0`. The release
is centered on the local C++ library contract for certified HTM optimal solving.

## Release Scope

Intended release promise:

- C++20 library for 3x3x3 Rubik's Cube solving.
- Public 54-sticker input in `U R F D L B` face order.
- HTM optimal solving through `rubik::Solver`.
- `SolveStatus::Optimal` means the returned solution is proven minimal for the
  requested options.
- V2 API stability contract for the main `Cube -> Solver -> SolveResult` path.
- `SolveProfile::Embedded`, `Default`, `Performance`, and `LargeLocal`
  profiles remain explicit local policies.
- `SolveMode::Fast` remains available as a practical non-optimal mode, not as a
  certified optimal mode.
- Benchmark claims are limited to runs performed on the development desktop.
- Raspberry Pi, Jetson Nano, and Jetson Orin behavior remains unclaimed until
  direct hardware validation is available.

Non-goals for this release:

- No cloud solving.
- No camera recognition.
- No hardware control.
- No application UI.
- No QTM support.
- No Raspberry Pi, Jetson Nano, or Jetson Orin latency claim.

## Current Readiness

| Area | Status | Notes |
| --- | --- | --- |
| Core cubie/sticker model | Ready | Validation covers physical cube constraints. |
| Move parsing/formatting | Ready | HTM moves implemented. |
| Public `Solver` API | Ready for release | V2 stable surface is documented. |
| Optimal correctness | Ready when returning `Optimal` | IDA* proof search with admissible bounds. |
| Fast mode | Available, non-optimal | Valid solutions, not optimality guaranteed. |
| Benchmark gates | Ready for release candidate | Profile-realistic, embedded-multiseed, and optimal-stress gates exist. |
| Large local optimal profile | Ready for local high-memory validation | Depth-15 desktop validation exists for selected seeds. |
| Embedded/Raspberry/Jetson | Not release-claimed | Needs physical hardware benchmark before any latency claim. |
| Packaging | Ready for release candidate | CMake install/export, source archive validation, checksum, and archive rebuild checks exist. |
| Documentation | Ready for release candidate | API, stability, runtime, benchmark, and release documents exist. |
| CI/release automation | Ready for release candidate | Local release script and GitHub Actions CI workflow exist. |
| License | Ready | Apache License 2.0 with repository NOTICE. |

Latest release-candidate validation:

- Command: `scripts/release_check.sh --profile standard`
- Result: `release_check,status,passed`
- Command: `scripts/release_check.sh --profile full --with-benchmarks`
- Result: `release_check,status,passed`
- Command: `scripts/release_check.sh --profile full --with-large-local`
- Result: `release_check,status,passed`
- Archive audit: source tarball regenerated, checked for generated artifacts,
  wrote a SHA-256 checksum, and rebuilt from a fresh extraction with the full
  test suite passing.
- Archive contents: `116` paths.
- Archive checksum:
  `f9f8884269c2c7e4e72d56e9fed94639459f1ff31b5f88fdf8440f0496a65bfd  rubik_cube_library-2.0.0.tar.gz`

## Required Before Tag

- [x] Document the `2.0.0` API stability contract.
- [x] Add changelog entry for `2.0.0`.
- [x] Draft GitHub Release text and asset checklist for `v2.0.0`.
- [x] Set project version to `2.0.0` in CMake package metadata.
- [x] Regenerate and validate the `2.0.0` source archive.
- [x] Run full local release validation with benchmark gates.
- [x] Run large-local validation if the release highlights `LargeLocal`.
- [x] Confirm public docs contain no unverified hardware performance claims.
- [x] Confirm generated artifacts and local process notes are not committed.
- [x] Create the local release tag.
- [ ] Upload archive and checksum to the GitHub Release.

## Validation Gates

Minimum local validation before tagging:

```sh
scripts/release_check.sh --profile standard
```

Full local validation before a release candidate:

```sh
scripts/release_check.sh --profile full --with-benchmarks
```

Large-local validation for high-memory local optimal claims:

```sh
scripts/release_check.sh --profile full --with-large-local
```

The release script regenerates the source archive, verifies versioned release
documents, writes a SHA-256 checksum, and builds/tests from a fresh archive
extraction.

## Benchmark Evidence To Publish

Publish only benchmark claims that were actually run for the release candidate.

Current local evidence on the development desktop:

- CPU: AMD Ryzen 9 8940HX, 16 cores / 32 threads.
- Build preset: `release-native-lto`.
- Cache mode: warm.
- V2 corner-state validation, 2026-05-26: profile-realistic,
  embedded-multiseed, optimal-stress, and optimal tail-case gates passed.
- V2 large-local depth-15 validation, 2026-05-26: 24/24 fixed depth-15 seeds
  solved under the 30s gate with 4 threads; six known tails passed the 18s
  8-thread gate.

These are local desktop measurements only.

## API Stability Decisions

Stable surface:

- `rubik::Cube`
- `rubik::CubeParseResult`
- `rubik::CubieCube`
- `rubik::CubieParseResult`
- `rubik::Move`
- `rubik::Solver`
- `rubik::SolveOptions`
- `rubik::SolveResult`
- `rubik::SolveBoundDiagnostics`
- `rubik::CubeError`
- `rubik::CubeErrorCode`
- CLI sticker order `U R F D L B`

The full release contract is documented in
[API Stability - 2.0.0](api-stability-2.0.0.md).

Experimental surface:

- phase-1 and phase-2 APIs;
- pruning table internals;
- coordinate APIs;
- benchmark random-case generation;
- environment-variable tuning flags;
- large-local optimal table combinations;
- `SolveMode::Fast` internals and solution quality.

## Hardware Matrix

| Target | Status For 2.0 |
| --- | --- |
| Development desktop | Supported for published local benchmark claims. |
| Jetson Orin class | Targeted, but benchmark pending. |
| Jetson Nano | Not claimed until benchmarked. |
| Raspberry Pi 4 GB | Targeted, but benchmark pending. |
| Raspberry Pi 8 GB | Targeted, but benchmark pending. |

## Release Blockers

Hard blockers:

- None for local release readiness.

Soft blockers:

- No Raspberry Pi hardware data.
- No Jetson hardware data.
