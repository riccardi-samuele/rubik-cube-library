# API Stability - 2.0.0

This document defines the stable public compatibility contract for `2.0.0`.
V3 additions have their own draft contract in
[API Stability - 3.0.0 Draft](api-stability-3.0.0.md); this document remains
the `2.x` compatibility reference.

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
