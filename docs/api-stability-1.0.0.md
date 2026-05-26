# API Stability - 1.0.0

This document freezes the public compatibility contract for `1.0.0`.

Within the `1.x` release line, the symbols and formats listed here should
remain source-compatible unless a bug makes the contract impossible to preserve.

## Frozen Input Format

The primary sticker input format is frozen for `1.x`:

- exactly 54 characters;
- face order: `U R F D L B`;
- each face read left-to-right, top-to-bottom;
- solved cube string:
  `UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB`.

Additional input formats may be added later, but they must not change the
meaning of this format.

## Frozen Error Codes

The `rubik::CubeErrorCode` enumerator names are frozen for `1.x`:

- `None`
- `InvalidStickerCount`
- `InvalidColor`
- `InvalidColorCount`
- `InvalidCenterConfiguration`
- `InvalidCornerPermutation`
- `InvalidCornerOrientation`
- `InvalidEdgePermutation`
- `InvalidEdgeOrientation`
- `InvalidParity`
- `UnsolvableCube`

Error message text is diagnostic and may change. Consumers should branch on
`CubeErrorCode`, not on `CubeError::message`.

## Stable Public Surface

These types and functions are treated as stable for `1.x`:

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

`SolveStatus::Optimal` is part of the semantic contract: when returned from
`SolveMode::Optimal`, the result is a proven-minimal HTM solution under the
requested options.

## Reserved Or Unsupported Values

Some public enum values are reserved for future use:

- `rubik::Metric::QTM` is not implemented in `1.0.0`.
- `rubik::SolveMode::Balanced` is not implemented in `1.0.0`.

Unsupported values must return `SolveStatus::UnsupportedOptions` rather than a
misleading successful result.

## Experimental Surface

The following APIs remain experimental in `1.0.0`:

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
- pruning-table internals;
- coordinate APIs;
- move-table internals;
- symmetry internals;
- benchmark random-case generation details;
- environment-variable tuning flags;
- large-local optimal table combinations;
- `SolveMode::Fast` internals and solution quality.

Experimental APIs may change in future releases.

## Phase Header Decision

For `1.0.0`, both legacy phase headers remain installed:

- `#include <rubik/phase1.hpp>`
- `#include <rubik/phase2.hpp>`

The recommended include paths for new code are:

- `#include <rubik/experimental/phase1.hpp>`
- `#include <rubik/experimental/phase2.hpp>`

The legacy headers are compatibility aliases for the release. They should not be
removed during the `1.x` line, but they may be deprecated in a future release.

## CMake Package Contract

Installed builds export:

- package name: `rubik`
- imported target: `rubik::rubik`

Consumers should use:

```cmake
find_package(rubik CONFIG REQUIRED)
target_link_libraries(my_target PRIVATE rubik::rubik)
```
