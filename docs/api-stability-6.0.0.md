# API Stability - 6.0.0

V6 keeps the V5 public-usability contract and promotes measured local
optimal-latency behavior without changing the certified HTM optimality contract.

`SolveStatus::Optimal` still means the returned HTM solution is proven minimal
for the requested options.

## Stable Public Surface

- `rubik::Cube`
- `rubik::CubieCube`
- `rubik::Move`
- `rubik::Solver`
- `rubik::SolveOptions`
- `rubik::SolveResult`
- `rubik::SolvePlan`
- `rubik::CacheSetupOptions`
- `rubik::CacheSetupResult`
- `rubik::SolveBoundDiagnostics`
- `rubik::CubeError`
- `rubik/version.hpp` version constants
- CLI sticker input order `U R F D L B`
- CLI tools `rubik-solve`, `rubik-bench`, and `rubik-cache-setup`
- CMake imported target `rubik::rubik`

## V6 Behavior

The local optimal path keeps the V6 conservative root-ordering policy that was
accepted by measured rollback replays. The policy changes search order and
latency characteristics only; it does not weaken admissible bounds or the
minimum-move proof.

V6 also keeps public benchmark tooling for transition-corpus extraction and
replay aggregation so future optimal-policy changes can be accepted or rejected
from measured data.

## Compatibility

Source compatibility with the V5 public API is preserved. Existing solve calls,
input formats, validation behavior, CMake package names, cache setup APIs,
examples, and version metadata remain stable.

Patch releases in the `6.x` line must preserve source compatibility for the
stable public surface unless a documented correctness bug requires a narrow
behavior change.

## Non-Stable Surface

Headers under `rubik/detail/` and `rubik/experimental/` are not part of the
source-compatible API contract.

`SolveMode::Fast`, phase-1/phase-2 APIs, pruning-table internals, large local
table combinations, environment-variable tuning flags, and diagnostic string
internals remain outside the stable API contract unless documented elsewhere as
stable.
