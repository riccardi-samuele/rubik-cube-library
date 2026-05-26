# Release Checklist - 1.0.0

This checklist tracks the remaining work before publishing the first stable
release. The release is allowed to expose experimental internals, but it must be
honest about guarantees, memory cost, and hardware coverage.

## Release Scope

Intended release promise:

- C++20 library for 3x3x3 Rubik's Cube solving.
- Public 54-sticker input in `U R F D L B` face order.
- HTM optimal solving through `rubik::Solver`.
- Experimental fast/two-phase APIs available but not stable.
- Large-local optimal profile documented as high-memory local policy, not as an
  embedded default.
- Public C++ and CLI version metadata are exposed from the CMake project
  version.
- Raspberry Pi behavior documented as unverified until real hardware is tested.

Non-goals for this release:

- No claim of full God's-algorithm latency for all random states.
- No Raspberry Pi benchmark claim without hardware.
- No stable commitment for phase-1/phase-2 experimental APIs.
- No QTM support.

## Current Readiness

| Area | Status | Notes |
| --- | --- | --- |
| Core cubie/sticker model | Ready | Validation covers physical cube constraints. |
| Move parsing/formatting | Ready | HTM moves implemented. |
| Public `Solver` API | Ready for release | Public stable surface is documented. |
| Optimal correctness | Ready when returning `Optimal` | IDA* proof search with admissible bounds. |
| Fast mode | Experimental | Valid solutions, not quality/stability guaranteed. |
| Large local optimal profile | Experimental but strong | 24 sampled depth-15 seeds under 30s with 4 threads; six known tails under 18s with 8 threads. |
| Embedded/Raspberry | Not release-claimed | Needs physical Raspberry Pi benchmark. |
| Packaging | Ready for release | CMake install/export, package version metadata, standalone consumer smoke target, source archive validation, and fresh archive build/test exist. |
| Documentation | Ready for release | API notes, README release positioning, changelog, versioning policy, runtime thread-safety, cache policy, and release-candidate validation exist. |
| CI/release automation | Ready for release | Local release script and GitHub Actions CI workflow exist. |
| License | Ready | Apache License 2.0 with repository NOTICE. |

Latest release-candidate validation:

- [Release Candidate Validation - 2026-05-26](release-candidate-2026-05-26.md)
- [GitHub Release Draft - v1.0.0](github-release-v1.0.0.md)
- Command: `scripts/release_check.sh --profile full --with-benchmarks`
- Result: `release_check,status,passed`
- Archive audit: source tarball regenerated, checked for generated artifacts,
  and rebuilt from a fresh extraction with 45/45 tests passing.

## Required Before Tag

- [x] Add a repository license file.
- [x] Choose and document semantic versioning policy.
- [x] Set project version to `1.0.0` in CMake package metadata.
- [x] Expose public version metadata through `rubik/version.hpp` and CLI
      `--version`.
- [x] Update README with release status, supported guarantees, and known limits.
- [x] Freeze public sticker order and error-code names for the release.
- [x] Decide whether legacy `rubik/phase1.hpp` and `rubik/phase2.hpp` aliases
      remain in the release or move fully under `rubik/experimental`.
- [x] Document table-cache compatibility and invalidation policy.
- [x] Document thread-safety expectations for `Solver`, table initialization,
      and shared cache directories.
- [x] Add release build instructions and install verification.
- [x] Add a changelog/release notes file.
- [x] Add CI or a documented local release script.
- [x] Draft GitHub Release text and asset checklist.

## Validation Gates

Minimum local validation before tagging:

```sh
scripts/release_check.sh --profile standard
```

This command also regenerates the source archive and verifies that a fresh
extraction configures, builds, and passes the local test suite. The archive
version is read from the CMake project version.

Full local validation before a release candidate:

```sh
scripts/release_check.sh --profile full --with-benchmarks
```

Benchmark gates to run on the development desktop:

```sh
cmake --build out/release-native-lto --target rubik-benchmark-profile-realistic
cmake --build out/release-native-lto --target rubik-benchmark-profile-realistic-gates

cmake --build out/release-native-lto --target rubik-benchmark-embedded-multiseed
cmake --build out/release-native-lto --target rubik-benchmark-embedded-multiseed-gates

cmake --build out/release-native-lto --target rubik-benchmark-optimal-stress
cmake --build out/release-native-lto --target rubik-benchmark-optimal-stress-gates
```

Large local experimental gates:

```sh
scripts/release_check.sh --profile full --with-large-local
```

Sanitizer validation:

```sh
cmake --preset asan-ubsan
cmake --build --preset asan-ubsan
ctest --preset asan-ubsan --output-on-failure
```

Install/package validation:

```sh
cmake --install out/release --prefix /tmp/rubik-1.0.0
cmake -S tests/consumer_smoke -B /tmp/rubik-consumer-build \
  -DCMAKE_PREFIX_PATH=/tmp/rubik-1.0.0
cmake --build /tmp/rubik-consumer-build
/tmp/rubik-consumer-build/rubik_consumer_smoke
```

The same validation is available from a configured build tree:

```sh
cmake --build out/release --target rubik-check-install-consumer
```

## Benchmark Evidence To Publish

Publish only benchmark claims that were actually run for the release candidate.

Current local evidence on the development desktop:

- `1.0.0` release candidate, 2026-05-26: full release check with
  benchmark gates passed.
- `Optimal/large-local`, 4 threads, depth-15 fixed seeds: 24/24 under 30s.
- `Optimal/large-local`, 8 threads, six known depth-15 tails: all under 18s.
- `Optimal/corner-state`, depth-13 stress gates: all pass with wide margin.
- `Optimal/corner-state`, depth-14 deep probe: sampled cases solved.

Do not publish Raspberry Pi latency claims yet. Mark Raspberry Pi support as a
target with pending hardware validation.

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

The full release freeze is documented in
[API Stability - 1.0.0](api-stability-1.0.0.md).

Experimental surface:

- phase-1 and phase-2 APIs;
- pruning table internals;
- coordinate APIs;
- benchmark random-case generation;
- environment-variable tuning flags;
- large-local optimal table combinations;
- `SolveMode::Fast` internals and solution quality.

## Hardware Matrix

| Target | Status For 1.0 |
| --- | --- |
| Development desktop | Supported for published local benchmark claims. |
| Jetson Orin class | Targeted, but benchmark pending. |
| Jetson Nano | Not claimed until benchmarked. |
| Raspberry Pi 4 GB | Targeted, but benchmark pending. |
| Raspberry Pi 8 GB | Targeted, but benchmark pending. |

## Release Blockers

Hard blockers:

None currently tracked in this checklist.

Soft blockers:

- No Raspberry Pi hardware data.
- No Jetson hardware data.
- Some 4-thread depth-15 cases are close to the 30s gate.
