# API Stability - 3.0.0 Draft

V3 keeps the V2 certified HTM optimality contract.

New stable public concepts:

- `SolveProfile::Auto`
- `CachePolicy`
- `SolvePlan`
- `CacheSetupOptions`
- `CacheSetupResult`
- `prepareCache()`
- CLI `rubik-cache-setup`

`SolveProfile::Auto` is valid for `SolveMode::Optimal` with `Metric::HTM`.
Unsupported combinations return `SolveStatus::UnsupportedOptions`.
When `CachePolicy::RequireWarm` is selected and required cache files are
missing, `Solver::solve()` returns `SolveStatus::CacheNotReady`.

The exact internal table set selected by `Auto` is not stable. The public
contract is that the selected plan respects user limits and is reported through
`SolvePlan`.

`SolvePlan::optimalMoveOrdering` reports the actual optimal DFS child ordering
used by the solve. Stable values are `base_bound`, `auto_strong_bound`,
`forced_strong_bound`, and `phase2_tiebreak`.

`SolvePlan::rootOrderingProfile` reports compact root child-ordering diagnostic
data for benchmark analysis. It is informational; consumers should treat it as a
semicolon-separated key-value string rather than a fixed schema.
