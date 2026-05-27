# API Stability - 3.0.0

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

The exact internal table set selected by `Auto` is not stable. The public
contract is that the selected plan respects user limits and is reported through
`SolvePlan`.

## Cache Policy

Stable `CachePolicy` values:

- `Auto`: use compatible warm cache data when available and choose a local plan
  that respects the requested options.
- `RequireWarm`: require all cache data needed by the selected plan to already
  exist. When required cache files are missing, `Solver::solve()` returns
  `SolveStatus::CacheNotReady`.
- `AllowBuild`: allow cache preparation to build missing local cache data.
- `Disabled`: do not use disk cache data for the solve.

The cache file layout, filenames, and table internals are not part of the stable
API contract.

## Solve Plan

`SolvePlan` is the stable public report for the selected solve strategy.
Stable fields:

- `requestedProfile`
- `effectiveProfile`
- `mode`
- `metric`
- `requestedMaxMemoryBytes`
- `effectiveMaxMemoryBytes`
- `estimatedTablePayloadBytes`
- `requestedThreads`
- `effectiveThreads`
- `cachePolicy`
- `diskCacheEnabled`
- `diskCacheWarm`
- `builtCacheDuringSolve`
- `boundsUsed`
- `strategyName`
- `optimalMoveOrdering`
- `rootOrderingProfile`

`SolvePlan::optimalMoveOrdering` reports the actual optimal DFS child ordering
used by the solve. Stable values are `base_bound`, `auto_strong_bound`,
`forced_strong_bound`, and `phase2_tiebreak`.

`SolvePlan::rootOrderingProfile` reports compact root child-ordering diagnostic
data for benchmark analysis. It is informational; consumers should treat it as a
semicolon-separated key-value string rather than a fixed schema. Parallel
optimal searches may include a `root_search` field with per-root outcome and
expanded-node counts, and diagnostic runs may include `root_bound_diagnostics`
with per-root pruning counters.

## Cache Preparation

`prepareCache()` accepts `CacheSetupOptions` and returns `CacheSetupResult`.
Stable `CacheSetupOptions` fields:

- `profile`
- `cachePolicy`
- `maxMemoryBytes`
- `threads`
- `cacheDirectory`
- `dryRun`

Stable `CacheSetupResult` fields:

- `ready`
- `cacheWarm`
- `plan`
- `bytesPrepared`
- `bytesMissing`
- `elapsed`
- `message`

`rubik-cache-setup` is the CLI entry point for the same cache-preparation
workflow. Its human-readable messages may change, while exit status and the
documented command-line options are the supported integration surface.
