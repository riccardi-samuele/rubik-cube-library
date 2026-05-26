# Rubik Cube Library 2.0 Release Prep Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Prepare the repository for a local `2.0.0` release candidate with a documented V2 API contract, version metadata, release notes, archive validation, and full local gates.

**Architecture:** Keep the stable library surface small and centered on `Cube -> Solver -> SolveResult`. Treat this plan as release preparation, not solver algorithm work: code changes should be limited to version metadata and tests needed to validate release packaging. Public documents must describe only measured library behavior and must not include private planning context.

**Tech Stack:** C++20, CMake, shell release scripts, CTest, Markdown docs, Git.

---

## File Structure

- Modify `CHANGELOG.md`: add a `2.0.0 - 2026-05-26` entry above `1.0.0`.
- Modify `CMakeLists.txt`: bump `project(rubik_cube_library VERSION 1.0.0 ...)` to `2.0.0`.
- Modify `README.md`: update release links and release-validation examples only where version-specific text requires it.
- Modify `docs/api.md`: update version examples and V2 stability references.
- Create `docs/api-stability-2.0.0.md`: define the stable V2 API contract.
- Create `docs/release-2.0.0.md`: release checklist/evidence for V2.
- Create `docs/github-release-v2.0.0.md`: GitHub release body and asset checklist.
- Review `docs/versioning.md`: leave it unchanged if the existing versioned
  release-doc policy already covers `2.0.0`.
- Modify `tests/check_release_archive_versioned_docs.sh` only if the existing test no longer covers version-derived docs and checksum output after the version bump.
- Do not modify solver internals unless a release validation failure proves a real bug.

## Task 1: Add V2 API Stability Document

**Files:**
- Create: `docs/api-stability-2.0.0.md`
- Modify: `docs/api.md`

- [ ] **Step 1: Write the API stability document**

Create `docs/api-stability-2.0.0.md` with this structure:

```markdown
# API Stability - 2.0.0

This document defines the stable public compatibility contract for `2.0.0`.

The main solver path is stable:

- parse or build a `rubik::Cube`;
- configure `rubik::SolveOptions`;
- call `rubik::Solver::solve`;
- inspect `rubik::SolveResult`.

## Stable Input Format

The primary sticker input format is:

- exactly 54 characters;
- face order: `U R F D L B`;
- each face read left-to-right, top-to-bottom;
- solved cube string:
  `UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB`.

Additional input formats may be added later, but they must not change the
meaning of this format.

## Stable Optimality Contract

For `SolveMode::Optimal`, `SolveStatus::Optimal` means the returned HTM
solution is proven minimal under the requested options.

Performance changes may alter pruning tables, move ordering, profile defaults,
or root-level parallel search behavior. They must not weaken the optimality
proof.

## Stable Public Surface

These symbols are stable for the `2.x` release line:

- `rubik::Cube`
- `rubik::CubeParseResult`
- `rubik::CubeError`
- `rubik::CubeErrorCode`
- `rubik::CubieCube`
- `rubik::CubieParseResult`
- `rubik::Corner`
- `rubik::Edge`
- `rubik::Face`
- `rubik::Move`
- `rubik::Metric`
- `rubik::SolveMode`
- `rubik::SolveProfile`
- `rubik::SolveStatus`
- `rubik::SolveOptions`
- `rubik::SolveBoundDiagnostics`
- `rubik::SolveResult`
- `rubik::Solver`
- `rubik::fromStickers`
- `rubik::validateStickers`
- `rubik::faceOf`
- `rubik::quarterTurns`
- `rubik::inverse`
- `rubik::toString`
- `rubik::formatMoves`
- `rubik::parseMove`
- `rubik::parseMoves`
- `rubik::allMoves`
- `rubik::version_major`
- `rubik::version_minor`
- `rubik::version_patch`
- `rubik::version_string`

## Modes And Metrics

`Metric::HTM` is implemented.

`Metric::QTM` is reserved unless a future release fully implements and tests
it.

`SolveMode::Fast` is available as a practical non-optimal mode. It must not be
documented as a certified optimal mode. A successful fast-mode result uses
`SolveStatus::Found`, not `SolveStatus::Optimal`.

`SolveMode::Balanced` is reserved and unsupported until implemented.

Unsupported values must return `SolveStatus::UnsupportedOptions` rather than a
misleading successful result.

## Experimental Surface

The following APIs remain experimental:

- `rubik::Phase1Options`
- `rubik::Phase1Result`
- `rubik::Phase1CandidatesResult`
- `rubik::Phase2Options`
- `rubik::Phase2Result`
- `rubik::isPhase1Solved`
- `rubik::findPhase1Candidates`
- `rubik::solvePhase1`
- `rubik::solvePhase2`
- everything in `rubik::experimental`
- pruning-table internals
- coordinate APIs
- move-table internals
- symmetry internals
- benchmark random-case generation details
- environment-variable tuning flags
- large-local optimal table combinations
- `SolveMode::Fast` internals and solution quality heuristics

Experimental APIs may change in future releases.

## CMake Package Contract

Installed builds export:

- package name: `rubik`
- imported target: `rubik::rubik`

Consumers should use:

```cmake
find_package(rubik CONFIG REQUIRED)
target_link_libraries(my_target PRIVATE rubik::rubik)
```
```

- [ ] **Step 2: Update `docs/api.md` references**

Change the opening compatibility link from `api-stability-1.0.0.md` to
`api-stability-2.0.0.md`. Change the version example to:

```cpp
#include <rubik/version.hpp>

static_assert(rubik::version_major == 2);
std::cout << rubik::version_string << "\n";
```

At the bottom, update the stability paragraph to link to
`API Stability - 2.0.0`.

- [ ] **Step 3: Verify docs contain no unsupported hardware claims**

Run:

```bash
tests/public_docs_no_unverified_hardware_estimates.sh
```

Expected: exit code `0`.

- [ ] **Step 4: Commit**

```bash
git add docs/api.md docs/api-stability-2.0.0.md
git commit -m "Document V2 API stability"
```

## Task 2: Add V2 Release Notes And Changelog

**Files:**
- Modify: `CHANGELOG.md`
- Create: `docs/release-2.0.0.md`
- Create: `docs/github-release-v2.0.0.md`
- Modify: `README.md`

- [ ] **Step 1: Add changelog entry**

Insert this section above `## 1.0.0 - 2026-05-26` in `CHANGELOG.md`:

```markdown
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
```

- [ ] **Step 2: Create `docs/release-2.0.0.md`**

Use `docs/release-1.0.0.md` as the structural model, but set:

```markdown
# Release Checklist - 2.0.0
```

Required content:

- release scope centered on certified HTM optimal solving;
- current readiness table for API contract, optimal correctness, benchmark
  gates, packaging, docs, CI, and license;
- validation command:
  `scripts/release_check.sh --profile full --with-benchmarks`;
- large-local validation command:
  `scripts/release_check.sh --profile full --with-large-local`;
- benchmark evidence limited to the development desktop;
- unchecked final publication items for tag creation and GitHub release upload.

Do not include private goals or planning context.

- [ ] **Step 3: Create `docs/github-release-v2.0.0.md`**

Use this metadata:

```markdown
# GitHub Release Draft - v2.0.0

Use this document as the GitHub Release body for tag `v2.0.0`.

## Release Metadata

- Tag: `v2.0.0`
- Target: second stable release
- Title: `v2.0.0 - Certified optimal solver contract`
- Attachments:
  - `dist/rubik_cube_library-2.0.0.tar.gz`
  - `dist/rubik_cube_library-2.0.0.tar.gz.sha256`
```

The release body must mention:

- `SolveStatus::Optimal` proven-minimal HTM guarantee;
- V2 API stability contract;
- local desktop benchmark validation only;
- no Raspberry Pi, Jetson Nano, or Jetson Orin latency claims;
- full release check command.

- [ ] **Step 4: Update README release links**

In `README.md`, add V2 links near the release links:

```markdown
- [Release Checklist - 2.0.0](docs/release-2.0.0.md)
- [API Stability - 2.0.0](docs/api-stability-2.0.0.md)
```

Keep the existing V1 links if they are historical references.

- [ ] **Step 5: Review versioning policy**

Read `docs/versioning.md`. If it already requires every public release to
update `CHANGELOG.md`, `docs/release-<version>.md`, and
`docs/github-release-v<version>.md`, leave it unchanged. If that requirement is
missing, add this text under the release update list:

```markdown
Every public release should update:

- `CHANGELOG.md`;
- `docs/release-<version>.md`;
- `docs/github-release-v<version>.md`;
- CMake project version metadata.
```

- [ ] **Step 6: Run public-doc hardware gate**

```bash
tests/public_docs_no_unverified_hardware_estimates.sh
```

Expected: exit code `0`.

- [ ] **Step 7: Commit**

```bash
git add CHANGELOG.md README.md docs/versioning.md docs/release-2.0.0.md docs/github-release-v2.0.0.md
git commit -m "Add V2 release documentation"
```

## Task 3: Bump Project Version To 2.0.0

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `docs/api.md` if the version example was not already updated

- [ ] **Step 1: Write a version expectation test by using the existing CLI version tests**

Run the current version checks before the bump:

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release -R "cli_solve_version|cli_bench_version" --output-on-failure
```

Expected before the bump: pass with the current `1.0.0` version.

- [ ] **Step 2: Bump CMake project version**

Change the top of `CMakeLists.txt` from:

```cmake
project(rubik_cube_library VERSION 1.0.0 LANGUAGES CXX)
```

to:

```cmake
project(rubik_cube_library VERSION 2.0.0 LANGUAGES CXX)
```

- [ ] **Step 3: Reconfigure and run version checks**

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release -R "cli_solve_version|cli_bench_version|rubik_tests" --output-on-failure
```

Expected: tests pass and generated `rubik/version.hpp` reports major `2`,
minor `0`, patch `0`.

- [ ] **Step 4: Commit**

```bash
git add CMakeLists.txt docs/api.md
git commit -m "Bump project version to 2.0.0"
```

If `docs/api.md` was unchanged in this task, commit only `CMakeLists.txt`.

## Task 4: Validate Release Archive Versioned Docs

**Files:**
- Modify: `tests/check_release_archive_versioned_docs.sh` only if the test does
  not already cover generated checksum paths for arbitrary versions.
- Generated only: `dist/rubik_cube_library-2.0.0.tar.gz`,
  `dist/rubik_cube_library-2.0.0.tar.gz.sha256`

- [ ] **Step 1: Run archive versioned-doc test**

```bash
tests/check_release_archive_versioned_docs.sh
```

Expected: pass. The test should prove that archive validation derives required
release docs from the CMake version and emits a `.sha256` file.

- [ ] **Step 2: Run archive generation for the real V2 version**

```bash
scripts/check_release_archive.sh --output-dir dist
```

Expected output includes:

```text
release_archive,path,dist/rubik_cube_library-2.0.0.tar.gz
release_archive,sha256_path,dist/rubik_cube_library-2.0.0.tar.gz.sha256
release_archive,status,passed
```

- [ ] **Step 3: Verify archive contents and checksum**

```bash
tar -tzf dist/rubik_cube_library-2.0.0.tar.gz | wc -l
cat dist/rubik_cube_library-2.0.0.tar.gz.sha256
```

Expected:

- archive entry count is non-zero;
- checksum line matches
  `^[0-9a-f]{64}  rubik_cube_library-2\.0\.0\.tar\.gz$`.

- [ ] **Step 4: Commit only if tests/scripts changed**

If `tests/check_release_archive_versioned_docs.sh` changed:

```bash
git add tests/check_release_archive_versioned_docs.sh
git commit -m "Validate V2 archive metadata"
```

If no tracked file changed, do not commit generated `dist` artifacts.

## Task 5: Run Standard Release Validation

**Files:**
- No intended tracked edits.

- [ ] **Step 1: Run standard release check**

```bash
scripts/release_check.sh --profile standard
```

Expected final line:

```text
release_check,status,passed
```

- [ ] **Step 2: Run whitespace check**

```bash
git diff --check
```

Expected: no output and exit code `0`.

- [ ] **Step 3: Inspect working tree**

```bash
git status --short --branch
```

Expected: no tracked modifications. Ignored/generated `dist` output must not be
committed.

## Task 6: Run Full Benchmark Release Gate

**Files:**
- No intended tracked edits.

- [ ] **Step 1: Run full release check with benchmarks**

```bash
scripts/release_check.sh --profile full --with-benchmarks
```

Expected final line:

```text
release_check,status,passed
```

- [ ] **Step 2: Run large-local gate if V2 highlights `LargeLocal`**

```bash
scripts/release_check.sh --profile full --with-large-local
```

Expected final line:

```text
release_check,status,passed
```

If this is too slow for the current session, stop and report that V2 release
cannot be called fully validated until this command passes.

- [ ] **Step 3: Record final artifact facts**

```bash
scripts/check_release_archive.sh --output-dir dist
tar -tzf dist/rubik_cube_library-2.0.0.tar.gz | wc -l
cat dist/rubik_cube_library-2.0.0.tar.gz.sha256
git status --short --branch
```

Expected:

- archive and `.sha256` are generated for `2.0.0`;
- checksum line names `rubik_cube_library-2.0.0.tar.gz`;
- working tree has no tracked changes;
- no push is performed.

## Task 7: Final Local Release Readiness Report

**Files:**
- Modify: `docs/release-2.0.0.md` only if final gate results need to be
  recorded after successful validation.

- [ ] **Step 1: Update final validation section if needed**

If full and large-local gates passed, update `docs/release-2.0.0.md` with:

```markdown
Latest release-candidate validation:

- Command: `scripts/release_check.sh --profile full --with-benchmarks`
- Result: `release_check,status,passed`
- Archive audit: source tarball regenerated, checked for generated artifacts,
  wrote a SHA-256 checksum, and rebuilt from a fresh extraction with the full
  test suite passing.
```

If `--with-large-local` also passed, add:

```markdown
- Large-local validation: `scripts/release_check.sh --profile full --with-large-local`
  passed on the development desktop.
```

- [ ] **Step 2: Run doc hardware gate**

```bash
tests/public_docs_no_unverified_hardware_estimates.sh
```

Expected: exit code `0`.

- [ ] **Step 3: Commit final release-doc update if changed**

```bash
git add docs/release-2.0.0.md
git commit -m "Record V2 release validation"
```

If no tracked file changed, skip this commit.

- [ ] **Step 4: Final report**

Report:

- latest commit hash;
- release version;
- commands that passed;
- archive checksum;
- whether `main` is ahead of `origin/main`;
- that no push was performed.

## Self-Review

- Spec coverage: the plan covers V2 API contract, optimal guarantee, fast-mode
  positioning, experimental surface, local hardware policy, benchmark gates,
  documentation updates, version metadata, archive validation, and release
  readiness.
- Placeholder scan: the plan contains no unresolved placeholder markers or
  intentionally blank implementation steps.
- Type consistency: all referenced public types match the current headers:
  `Cube`, `Solver`, `SolveOptions`, `SolveResult`, `SolveMode`,
  `SolveProfile`, `SolveStatus`, `SolveBoundDiagnostics`, `Metric`.
