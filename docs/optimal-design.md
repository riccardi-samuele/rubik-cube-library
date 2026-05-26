# Optimal Solver Design

`SolveMode::Optimal` is the main product path. The public API must stay stable:
users call `Solver::solve` with `SolveMode::Optimal` and receive a proven minimum
HTM solution when the solver returns `SolveStatus::Optimal`.

Internally, the current IDA* engine remains the correctness reference while the
next optimal engine is developed and benchmarked behind the same public mode.

## Current Engine

The current engine is one-direction IDA* over `SearchNode`.

Current strengths:

- exact cubie-state search;
- admissible pruning tables;
- coordinate move-table updates per node;
- same-face pruning and opposite-face canonical ordering;
- stable child ordering based on the smaller historical lower bound.

Current default non-embedded pruning:

- orientation and slice placement tables;
- corner permutation and U/D edge group tables;
- corner orientation + slice placement;
- edge orientation + slice placement;
- corner permutation + slice placement;
- corner permutation + edge orientation;
- corner orientation + U/D edge group permutation;
- edge orientation + U/D edge group permutation.

Current benchmark status:

```text
warm optimal depth 13 suite: about 1.29 s total, 61,770 expanded nodes
warm optimal depth 14 suite: timeout at 30 s on depth_14, about 2.74M expanded nodes
```

The current engine is correct, but not yet the final architecture for random
18-20 move optimal solving.

## External References

The important published and production designs point to the same conclusion:
optimal solving is won by strong admissible pruning, symmetry reduction, and
careful search ordering.

- Korf-style IDA* uses large pattern databases for lower bounds.
- Cube Explorer's standard optimal solver uses Mike Reid's 1997 triple phase-1
  method.
- Kociemba documents using three phase-1 directions and `max(p1, p2, p3)` as an
  effective lower bound.
- Kociemba's pruning tables use symmetry coordinates to compress otherwise huge
  tables.

The next architecture should therefore not be only "more small tables". It
should add symmetry-aware pruning and direction-aware phase-1 bounds.

## Target Architecture

The final public behavior remains:

```cpp
solver.solve(cube, {.mode = rubik::SolveMode::Optimal});
```

Internally, the optimal engine should evolve toward a Kociemba/Reid-style proof
search:

1. Keep exact IDA* as the proof mechanism.
2. Add cube symmetry infrastructure.
3. Add symmetry-reduced phase-1 pruning coordinates.
4. Compute phase-1 lower bounds in three directions.
5. Use `max(p1, p2, p3)` as an admissible pruning bound.
6. Preserve stable child ordering unless a benchmark proves a new ordering is
   better.
7. Benchmark old and new internal engines side-by-side before replacing the
   default implementation.

## Development Rules

Changes to the optimal engine must follow these rules:

- Keep `SolveMode::Optimal` as the only public optimal mode.
- Do not expose temporary `optimal_v2` naming in the public API.
- Keep the existing engine working until the new engine is faster and equally
  tested.
- Every pruning bound must be admissible.
- Every search-ordering change must be benchmarked separately from pruning.
- If an experiment increases wall time or does not reduce nodes meaningfully,
  revert it.

Recent rejected experiments:

- Exact transposition cache for IDA*: correct but did not reduce nodes on the
  deterministic depth benchmark and added overhead.
- Strong bound as child-ordering tie-break: caused a large depth-13 regression.
- Strong bound as primary child ordering on the depth-14 frontier: still timed
  out and expanded more nodes than the base ordering.
- Phase-2 lower-bound tie-break for optimal DFS
  (`RUBIK_EXPERIMENTAL_PHASE2_OPTIMAL_ORDERING=1`): reduced the sampled
  depth-15 timeout node count slightly but did not solve the case within
  30 seconds.
- Exact goal-table cutoff
  (`RUBIK_EXPERIMENTAL_OPTIMAL_GOAL_TABLE_DEPTH=6`): preserves correctness, but
  the sampled depth-15 case solved slower than corner-state pruning alone.

Promotion candidates:

- Full corner-state pruning table
  (`RUBIK_EXPERIMENTAL_CORNER_STATE_BOUNDS=1`): adds 88,179,840 entries and
  materially improves the current depth-14/depth-15 frontier probes. It passes
  the current depth-13 optimal-stress gates with wide margin and solves all
  sampled depth-14 deep-probe cases. The V2 tail experiment reduced the fixed
  embedded tail-case average from 6,516.20 ms to 1,482.60 ms and max from
  7,575 ms to 2,346 ms, with the embedded optimal payload rising to about
  110 MB. This is the first V2 promotion target.

Promising active experiments:

- Corner permutation + edge-group pruning
  (`RUBIK_EXPERIMENTAL_CORNER_UP_EDGE_BOUNDS=1` or
  `RUBIK_EXPERIMENTAL_CORNER_DOWN_EDGE_BOUNDS=1`): each table adds
  479,001,600 entries. Combined with corner-state, the U-edge variant solved the
  seed `12345` depth-15 frontier case in 28.914 seconds, but seed `42` still
  timed out at 30 seconds. On the V2 tail set, a single edge-group table reduced
  the average to about 830 ms, and both tables reduced the average to about
  650 ms. The memory cost keeps these variants experimental and limited to
  large local profile work.

## Implementation Phases

### Phase 1: Symmetry Foundation

Add a small internal symmetry module:

- enumerate and test the 24 orientation-preserving cube rotations;
- select the 16 Kociemba-style coordinate symmetries only after the geometric
  rotation layer is proven correct;
- support conjugating `CubieCube` by symmetry;
- add tests that conjugation preserves solvability and distances for sampled
  move sequences;
- add coordinate conjugation tables only after cubie-level conjugation is
  correct.

Status:

- `rubik/detail/symmetry.hpp` implements the 24 geometric rotations plus
  inverse/composition helpers.
- `rubik/detail/symmetry_coordinates.hpp` implements conjugation tables for
  corner orientation, edge orientation, and slice edge placement.
- The same module now exposes canonical representatives and orbit indices for
  corner orientation and edge orientation.
- `rubik-bench --report-symmetry` reports table sizes and orbit counts. Current
  counts are 111 corner-orientation orbits and 114 edge-orientation orbits under
  the 24 geometric rotations.
- `rubik/detail/symmetry_pruning.hpp` implements the first reduced pruning
  tables for corner orientation and edge orientation. They are tested as
  admissible lower bounds against the existing full coordinate pruning tables;
  they are intentionally not wired into the optimal solver yet because the
  reduced bounds are smaller than the full single-coordinate bounds.
- The first combined reduced coordinate is corner orientation + edge
  orientation: 4,478,976 full states collapse to 187,350 symmetry orbits
  (about 23.9x compression), with a 187 KB reduced pruning table.
- The first phase-1-shaped reduced coordinate is edge orientation + slice edge
  placement. Because slice placement is only closed under U/D-slice-preserving
  rotations, it uses 8 symmetries: 1,013,760 full states collapse to 127,326
  orbits (about 8.0x compression), with a 127 KB reduced pruning table.
- The matching corner orientation + slice edge placement coordinate uses the
  same 8 valid symmetries: 1,082,565 full states collapse to 135,576 orbits
  (about 8.0x compression), with a 136 KB reduced pruning table.
- `RUBIK_EXPERIMENTAL_SYMMETRY_BOUNDS=1` can include the reduced bounds in the
  optimal lower-bound calculation. The first deterministic depth-13 A/B test
  produced the same node count as baseline, so the flag remains off by default.
- Three-direction phase-1 lower bounds are enabled by default for
  `SolveMode::Optimal` with all public profiles, including `Embedded`. After lazy
  evaluation and lower overhead in the hot bound path, repeated warm-cache A/B
  runs improved deterministic depth-13 from 68,991 to 61,770 nodes and random
  depth-12 x20 from 447,954 to 402,849 nodes, with improved average wall time on
  the development machine. `SolveMode::Fast` remains conservative by default.
  `RUBIK_DISABLE_THREE_PHASE1_BOUNDS=1` forces the old baseline path
  for A/B benchmarks, and `RUBIK_EXPERIMENTAL_THREE_PHASE1_BOUNDS=1` can
  force-enable the bound for developer testing.
- Slice edge placement is intentionally restricted to U/D-slice-preserving
  symmetries when testing composition, because the coordinate is not closed
  under rotations that move the U/D axis to another axis.

### Phase 2: Symmetry-Reduced Coordinates

Add internal coordinates needed for phase-1 optimal pruning:

- flip + UDSlice equivalence class;
- twist conjugation;
- optionally UDSliceSorted for the huge optimal path.

The first milestone is not maximum performance; it is reproducing stable
coordinate identities and table sizes.

### Phase 3: Triple Direction Lower Bound

Compute three direction-specific phase-1 lower bounds:

- `<U,D,R2,L2,F2,B2>`
- `<U2,D2,R,L,F2,B2>`
- `<U2,D2,R2,L2,F,B>`

Use their maximum as an admissible lower bound. Keep existing small-table bounds
available and combine by `max`.

### Phase 4: Replace Default Optimal Internally

Once the new bound improves deterministic depth 14 and random optimal samples,
route `SolveMode::Optimal` through the new internal engine for `Default` and
`Performance` profiles. Keep `Embedded` conservative until Raspberry Pi
benchmarks prove the memory cost is acceptable.

## Benchmark Gates

Before replacing the current optimal engine, the new engine must pass:

- all unit tests;
- deterministic optimal depth 13 faster than current baseline;
- deterministic depth 14 solved or materially closer under the same 30 second
  timeout;
- random optimal validation on fixed seeds and shallow known-optimal scrambles;
- cold-cache and warm-cache reports;
- memory and cache size reports.

Current corner-state promotion status:

- depth-13 random optimal stress: passes all existing public-profile gates;
- depth-14 deep probe: all sampled cases solved;
- depth-15 deep probe: selected seed `12345` and `42` cases still time out at
  30 seconds, so the worst-tail proof search is not yet publication-ready.
- DFS node materialization now reuses the already-built candidate node when
  recursing instead of applying the same move a second time. This is a safe
  engine cleanup, but it does not materially change the depth-15 frontier by
  itself.
- A single 479 MB corner/edge-group table can bring one sampled depth-15 timeout
  under 30 seconds, but the remaining seed `42` timeout shows that depth 15 is
  still not controlled.
- Root-parallel optimal search now uses `SolveOptions::threads`. With
  corner-state, both corner/edge-group tables, the performance profile, and
  four threads, the seed `42` depth-15 frontier case solved in 17.896 seconds.
  This is a large local/desktop configuration, not an embedded default policy.

Current baseline to beat:

```text
depth_13 elapsed_ms: about 700-850
depth_13 nodes_expanded: 68,991 total deterministic-suite nodes
depth_14 status: Timeout at 30,000 ms
depth_14 nodes_expanded: about 2.5M-3.0M depending build and cache state
```
