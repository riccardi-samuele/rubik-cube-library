# Architecture

This library is structured around two solver goals:

- `SolveMode::Optimal`: prove the minimum HTM solution when the search finishes.
- `SolveMode::Fast`: return a short non-optimal solution quickly, suitable for
  interactive and embedded use.

The optimal solver remains the correctness reference. The fast solver is allowed
to use non-admissible selection strategies and multiple candidate paths, but it
must still return only physically valid solutions.

## Cube Representations

`rubik::Cube` stores the sticker-level cube. It is the public input and output
format because it maps directly to camera, UI, and user-entered data.

`rubik::CubieCube` stores the cubie-level representation:

- corner permutation
- corner orientation
- edge permutation
- edge orientation

Search code uses `CubieCube` because coordinates and moves are cheaper to update
there than at sticker level.

## Coordinates

The current coordinate layer includes:

- corner orientation: `3^7 = 2187`
- edge orientation: `2^11 = 2048`
- corner permutation: `8! = 40320`
- full edge permutation rank: `12!`, currently for indexing only
- slice edge placement: `C(12, 4) = 495`
- slice edge permutation: `4! = 24`
- U-edge group permutation: `C(12, 4) * 4! = 11880`
- D-edge group permutation: `C(12, 4) * 4! = 11880`

Coordinate move tables update these coordinates without recomputing them from a
full cubie state at every search node.

## Pruning Tables

Pruning tables store lower bounds as byte distances. They are generated lazily
and cached to disk. The cache directory is controlled by
`RUBIK_TABLE_CACHE_DIR`; otherwise the system temporary directory is used.

Current pruning coverage:

- corner orientation
- edge orientation
- slice edge placement
- corner permutation
- U-edge group permutation
- D-edge group permutation
- corner orientation + slice edge placement
- edge orientation + slice edge placement
- corner permutation + slice edge placement
- optional U-edge + D-edge group permutation profile table
- phase-2 corner permutation + slice edge permutation
- phase-2 U-edge group permutation + slice edge permutation
- phase-2 D-edge group permutation + slice edge permutation

The default optimal lower bound is the maximum of several admissible pruning
tables, so it stays admissible for optimal IDA*.

The phase-2 combined tables are built only with the phase-2 move set. They are
used by `solvePhase2`, not by the optimal IDA* lower bound.

## Optimal Search

The optimal solver is IDA* over `SearchNode`.

`SearchNode` carries the cubie state plus cached coordinates. Applying a move
updates the coordinates through move tables. The search uses:

- iterative deepening from the current lower bound;
- same-face pruning;
- opposite-face canonical ordering;
- move ordering by child lower bound;
- timeout-aware recursion.

When it returns `SolveStatus::Optimal`, the move count is proven minimal under
the requested metric.

The next optimal architecture is tracked in
[Optimal Solver Design](optimal-design.md). Temporary implementation names must
remain internal; public users should continue to call `SolveMode::Optimal`.

The local profile contract is tracked in
[Local Optimal Profiles](local-optimal-profiles.md): profiles may change table
selection and latency, but they must not change the meaning of a certified
optimal result.

The first internal symmetry layer now lives in `rubik/detail/symmetry.hpp`. It
enumerates the 24 orientation-preserving cube rotations, applies them to
`Cube` and `CubieCube`, and exposes inverse/composition helpers. This layer is
tested independently before it is used for symmetry-reduced pruning tables.

Coordinate symmetry tables for corner orientation, edge orientation, and slice
edge placement live in `rubik/detail/symmetry_coordinates.hpp`. Slice edge
placement is only closed under symmetries that preserve the U/D slice; the code
exposes `preservesUdSlice` so future pruning tables cannot accidentally use an
invalid coordinate action. The same module also provides canonical
representatives and orbit indices for corner orientation and edge orientation,
which is the first building block for symmetry-reduced pruning tables.
`rubik/detail/symmetry_pruning.hpp` contains the first reduced pruning tables for
corner orientation and edge orientation. These are currently validation
infrastructure, not solver inputs: the reduced tables are much smaller, but
weaker than the full single-coordinate pruning tables.
The first combined reduced table covers corner orientation + edge orientation:
4,478,976 full states collapse to 187,350 orbits, and the reduced pruning table
is 187,350 bytes.
The first phase-1-shaped reduced table covers edge orientation + slice edge
placement using only U/D-slice-preserving symmetries: 1,013,760 full states
collapse to 127,326 orbits, and the reduced pruning table is 127,326 bytes.
The matching corner orientation + slice edge placement reduced table has
1,082,565 full states, 135,576 orbits, and a 135,576-byte pruning table.
The environment flag `RUBIK_EXPERIMENTAL_SYMMETRY_BOUNDS=1` can include these
reduced bounds in the optimal lower bound, but the flag is off by default
because the current deterministic benchmark showed no node reduction.
The optimal solver now enables three-direction phase-1 bounds by default for
`SolveMode::Optimal` with all public profiles, including `Embedded`.
`SolveMode::Fast` stays conservative unless the developer explicitly sets
`RUBIK_EXPERIMENTAL_THREE_PHASE1_BOUNDS=1`. `RUBIK_DISABLE_THREE_PHASE1_BOUNDS=1`
forces the old optimal baseline path for A/B benchmarks. The implementation
stores and updates only the two extra phase-1 directions because the normal U/D
direction already exists in the base node coordinates.

## Fast Search

`SolveMode::Fast` currently tries a two-phase path first:

1. Try a small low-latency two-phase pass.
2. If that fails, try a larger robust two-phase pass.
3. For each pass, solve phase 2 for each selected G1 candidate.
4. Keep the shortest completion found inside that pass.
5. Fall back to beam search if no two-phase completion is found.

Phase 1 target:

- corner orientation solved
- edge orientation solved
- slice edge placement solved

Phase 2 move set:

- `U`, `U2`, `U'`
- `D`, `D2`, `D'`
- `R2`, `F2`, `L2`, `B2`

The fast solver is not optimal. It should eventually become the default path for
interactive or embedded solving, while `SolveMode::Optimal` remains the proof
mode.

## Current Bottlenecks

The optimal solver now uses combined corner/edge orientation pruning and
three-direction phase-1 bounds in the default optimal profile, while keeping
child ordering based on the smaller historical bound. This improves repeated
depth-13 and random depth-12 benchmarks, but stronger admissible pruning is
still needed before depth 14+ is practical within the 30 second target.

The fast solver now reaches deterministic depth 13 cases with multi-candidate
phase 1, phase-2-specific combined pruning, and an adaptive low-latency first
pass. Remaining fast-mode work is less about basic feasibility and more about
candidate scoring, tail latency, and validation on Raspberry Pi hardware.
