# Release Checklist - 3.0.0 Draft

This checklist tracks the remaining work before publishing `3.0.0`. The release
is centered on adaptive local optimal solving while preserving the certified HTM
minimum-move contract.

## Release Scope

Intended release promise:

- C++20 library for 3x3x3 Rubik's Cube solving.
- Public 54-sticker input in `U R F D L B` face order.
- HTM optimal solving through `rubik::Solver`.
- `SolveStatus::Optimal` means the returned solution is proven minimal for the
  requested options.
- `SolveProfile::Auto` is the recommended optimal profile for local solving.
- Explicit `Embedded`, `Default`, `Performance`, and `LargeLocal` profiles
  remain available for fixed policy selection.
- `SolvePlan` reports the effective profile, cache policy, thread count,
  memory budget, and optimal move-ordering policy used by the solve.
- `prepareCache()` and `rubik-cache-setup` are available for preparing local
  pruning-table caches before latency-sensitive solves.
- Benchmark claims are limited to runs performed on the development desktop.
- Raspberry Pi, Jetson Nano, and Jetson Orin behavior remains unclaimed until
  direct hardware validation is available.

Non-goals for this release:

- No cloud solving.
- No camera recognition.
- No hardware control.
- No application UI.
- No QTM support.
- No GPU backend.
- No Raspberry Pi, Jetson Nano, or Jetson Orin latency claim.

## Current Readiness

| Area | Status | Notes |
| --- | --- | --- |
| Core cubie/sticker model | Ready | Validation covers physical cube constraints. |
| Move parsing/formatting | Ready | HTM moves implemented. |
| Public `Solver` API | Ready for candidate | V3 stability surface is documented. |
| Optimal correctness | Ready when returning `Optimal` | Certified HTM contract is unchanged from V2. |
| `SolveProfile::Auto` | Ready for candidate | Auto selects a local optimal policy and reports it through `SolvePlan`. |
| Explicit profiles | Ready | Existing fixed profiles remain available. |
| Cache preparation | Ready for candidate | `prepareCache()` and `rubik-cache-setup` exist. |
| Benchmark gates | Ready for local candidate validation | V3 Auto gates, embedded multiseed, and optimal stress gates exist. |
| Embedded/Raspberry/Jetson | Not release-claimed | Needs physical hardware benchmark before any latency claim. |
| Packaging | Ready for final candidate run | Source archive and checksum were regenerated for `3.0.0`. |
| Documentation | In progress | V3 API stability, benchmarks, and local verification are documented. |
| CI/release automation | Ready for candidate | Local release script and GitHub Actions CI workflow exist. |
| License | Ready | Apache License 2.0 with repository NOTICE. |

Latest local V3 verification:

- Command: `cmake --build out/release-native-lto --target rubik-benchmark-embedded-multiseed`
- Result: passed.
- Command: `cmake --build out/release-native-lto --target rubik-benchmark-embedded-multiseed-gates`
- Result: passed.
- Command: `cmake --build out/release-native-lto --target rubik-benchmark-optimal-stress`
- Result: passed.
- Command: `cmake --build out/release-native-lto --target rubik-benchmark-optimal-stress-gates`
- Result: passed.
- Command: `cmake --build out/release-native-lto --target rubik-benchmark-v3-auto-gates`
- Result: passed.
- Archive audit: source tarball regenerated, checked for generated artifacts,
  wrote a SHA-256 checksum, and verified required versioned release documents.
- Archive contents: `149` paths.
- Archive checksum:
  `9f2aad917145d587cc5dbb8fe4af4c2fa6d55b3b4340e6e8aa1d1458f8ecd732  rubik_cube_library-3.0.0.tar.gz`

See [V3 Local Verification - 2026-05-27](v3-local-verification-2026-05-27.md).

## Required Before Tag

- [x] Document the `3.0.0` API stability contract.
- [x] Make `SolveProfile::Auto` the recommended optimal profile in public
      documentation.
- [x] Add `SolvePlan` reporting for the effective Auto plan.
- [x] Add cache preparation APIs and CLI support.
- [x] Add repeatable V3 Auto benchmark gates.
- [x] Record local V3 verification without unmeasured hardware claims.
- [x] Finalize the `3.0.0` API stability document by removing draft wording.
- [x] Add changelog entry for `3.0.0`.
- [x] Draft GitHub Release text and asset checklist for `v3.0.0`.
- [x] Set project version to `3.0.0` in CMake package metadata.
- [x] Regenerate and validate the `3.0.0` source archive.
- [ ] Run final release validation with V3 Auto gates.
- [ ] Confirm public docs contain no unverified hardware performance claims.
- [ ] Confirm generated artifacts are not committed.
- [ ] Create the local release tag.
- [ ] Upload archive and checksum to the GitHub Release.

## Validation Gates

Minimum local validation before tagging:

```sh
scripts/release_check.sh --profile standard
```

V3 Auto release-candidate validation:

```sh
scripts/release_check.sh --profile quick --with-v3-auto
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
- V3 local verification, 2026-05-27: embedded multiseed, optimal stress, and
  V3 Auto gates passed.
- V3 Auto tail gate: `7/7` fixed depth-15 cases solved under the `12000 ms`
  gate.
- V3 Auto hardening gate: `36/36` fixed cases solved under the configured
  depth-14 and depth-15 gates.

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
- `rubik::SolvePlan`
- `rubik::SolveProfile::Auto`
- `rubik::CachePolicy`
- `rubik::CacheSetupOptions`
- `rubik::CacheSetupResult`
- `rubik::prepareCache()`
- `rubik::CubeError`
- `rubik::CubeErrorCode`
- CLI sticker order `U R F D L B`
- CLI `rubik-cache-setup`

The full release contract is documented in
[API Stability - 3.0.0](api-stability-3.0.0.md).

Experimental surface:

- phase-1 and phase-2 APIs;
- pruning table internals;
- coordinate APIs;
- benchmark random-case generation;
- environment-variable tuning flags;
- large-local optimal table combinations;
- root-ordering diagnostic strings;
- `SolveMode::Fast` internals and solution quality.

## Hardware Matrix

| Target | Status For 3.0 |
| --- | --- |
| Development desktop | Supported for published local benchmark claims. |
| Jetson Orin class | Targeted, but benchmark pending. |
| Jetson Nano | Not claimed until benchmarked. |
| Raspberry Pi 4 GB | Targeted, but benchmark pending. |
| Raspberry Pi 8 GB | Targeted, but benchmark pending. |

## Release Blockers

Hard blockers:

- Final release-candidate validation has not been run after setting the final
  `3.0.0` version.

Soft blockers:

- No Raspberry Pi hardware data.
- No Jetson hardware data.
