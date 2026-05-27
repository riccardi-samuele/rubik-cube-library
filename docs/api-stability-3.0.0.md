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
