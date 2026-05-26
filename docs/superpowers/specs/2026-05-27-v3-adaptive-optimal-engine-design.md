# V3 Adaptive Optimal Engine Design

## Purpose

Rubik Cube Library V3 will focus on the optimal solver engine.

The release goal is not to add unrelated features. V3 should keep the V2
certified HTM optimality contract and make optimal solving more adaptive,
faster on desktop-class local machines, easier to prepare with cache, and more
transparent through diagnostics.

The guiding product idea is a chameleon-like optimal solver: the library should
adapt to the current memory budget, thread budget, cache state, and requested
limits while preserving correctness.

## Release Promise

When `SolveMode::Optimal` returns `SolveStatus::Optimal`, the returned solution
is still proven minimal in HTM for the requested options.

V3 adds an adaptive planning layer around that existing guarantee. The planner
may choose different bounds, table sets, thread counts, and cache behavior, but
it must not weaken the proof.

## Non-Goals

- No QTM implementation in this V3 scope.
- No GPU acceleration in this V3 scope.
- No cloud or remote solving.
- No camera recognition.
- No robot hardware control.
- No Raspberry Pi, Jetson Nano, or Jetson Orin performance claims until direct
  hardware measurements exist.
- No persistent learning or self-tuning database in V3.
- No change to the meaning of the V2 54-sticker input format.

## Public API Additions

### `SolveProfile::Auto`

`SolveProfile::Auto` is the recommended V3 profile for certified optimal
solving.

It is valid only with:

- `SolveMode::Optimal`
- `Metric::HTM`

If `Auto` is requested with `SolveMode::Fast`, the solver should return
`SolveStatus::UnsupportedOptions` rather than silently selecting a different
policy.

Manual profiles remain available:

- `Embedded`
- `Default`
- `Performance`
- `LargeLocal`

These profiles remain explicit policies for users that need predictable
resource behavior.

### `CachePolicy`

V3 should expose a small public cache policy enum:

```cpp
enum class CachePolicy {
    Auto,
    RequireWarm,
    AllowBuild,
    Disabled,
};
```

Semantics:

- `Auto`: choose cache behavior based on timeout, memory budget, cache
  availability, and selected plan.
- `RequireWarm`: use an existing compatible cache, but do not build heavy cache
  data during `solve`.
- `AllowBuild`: building compatible cache data during `solve` is allowed.
- `Disabled`: do not use disk cache.

The default for `SolveProfile::Auto` is `CachePolicy::Auto`.

### `SolvePlan`

V3 should expose a minimal stable plan summary in `SolveResult`. The purpose is
to make adaptive behavior explainable without exposing unstable table internals.

Proposed shape:

```cpp
struct SolvePlan {
    SolveProfile requestedProfile;
    SolveProfile effectiveProfile;
    SolveMode mode;
    Metric metric;

    std::uint64_t requestedMaxMemoryBytes;
    std::uint64_t effectiveMaxMemoryBytes;
    std::uint64_t estimatedTablePayloadBytes;

    unsigned requestedThreads;
    unsigned effectiveThreads;

    CachePolicy cachePolicy;
    bool diskCacheEnabled;
    bool diskCacheWarm;
    bool builtCacheDuringSolve;

    std::vector<std::string> boundsUsed;
    std::string strategyName;
};
```

The field names may be refined during implementation, but the public concept is
stable: users must be able to inspect the effective profile, memory budget,
thread count, cache behavior, and broad bound family used by `Auto`.

## Adaptive Policy

### Memory Policy

If the user sets `maxMemoryBytes`, `Auto` must respect it as a hard planning
limit.

If the user does not set `maxMemoryBytes`, `Auto` should use an aggressive
desktop-oriented default for V3:

- initial default budget: about 2 GB;
- no unbounded memory use;
- users can explicitly request larger budgets such as 4 GB or 8 GB.

This keeps V3 faster on desktop while leaving `Embedded` as the low-memory
manual profile.

### Thread Policy

If the user sets `threads`, `Auto` must respect it.

If `threads == 0`, `Auto` chooses an effective thread count:

- at least 1;
- never above `std::thread::hardware_concurrency()` when available;
- desktop-oriented cap suitable for the memory plan;
- more aggressive when the memory budget and timeout justify it;
- reported in `SolvePlan`.

The initial implementation may cap automatic desktop use around 8 threads and
use higher counts only when benchmarks justify it.

### Cache Policy

The adaptive cache behavior should choose the fastest safe option for the
situation:

- if compatible cache is warm, use it;
- if cache is missing and `CachePolicy::AllowBuild` is active, build it when
  the timeout/setup context makes that worthwhile;
- if cache is missing and the solve timeout is tight, avoid spending the whole
  solve building heavy cache data;
- if the cache directory is unavailable or unwritable, fall back to the best
  in-process plan allowed by memory;
- if `CachePolicy::RequireWarm` is active, do not build heavy cache data during
  `solve`;
- if `CachePolicy::Disabled` is active, use no disk cache.

## Cache Setup

V3 should add a dedicated CLI:

```sh
rubik-cache-setup --profile auto --max-memory-mb 2048
```

Useful options:

```sh
rubik-cache-setup --profile auto
rubik-cache-setup --profile large-local --max-memory-mb 4096
rubik-cache-setup --profile embedded --cache-dir /path/to/cache
rubik-cache-setup --dry-run
```

The tool should:

- use the same planner as `SolveProfile::Auto`;
- prepare compatible cache data for the selected plan;
- verify cache compatibility with the library version and table configuration;
- support `--dry-run` to show the selected plan without building tables;
- avoid hardware performance claims.

V3 should also expose a minimal C++ API:

```cpp
struct CacheSetupOptions {
    SolveProfile profile = SolveProfile::Auto;
    CachePolicy cachePolicy = CachePolicy::AllowBuild;
    std::uint64_t maxMemoryBytes = 0;
    unsigned threads = 0;
    std::filesystem::path cacheDirectory;
    bool dryRun = false;
};

struct CacheSetupResult {
    bool ready;
    SolvePlan plan;
    std::uint64_t bytesPrepared;
    std::chrono::milliseconds elapsed;
    std::string message;
};

CacheSetupResult prepareCache(const CacheSetupOptions& options);
```

The CLI is the primary user workflow. The C++ API exists for applications,
robots, and setup tools that should not shell out to a separate process.

## Internal Architecture

### Optimal Planner

Add an internal planner that translates public options into an execution plan.

Inputs:

- solve mode and metric;
- requested profile;
- memory budget;
- thread budget;
- timeout;
- max depth;
- cache policy;
- cache directory state;
- build/runtime capability information.

Outputs:

- effective profile;
- table/bound families;
- effective thread count;
- effective memory budget;
- cache usage decision;
- strategy name;
- diagnostics plan summary.

The planner should be deterministic for the same inputs and environment.

### Bounds Selection

V3 should start from V2's promoted corner-state bound and study heavier
corner/edge-group bounds under memory tiers.

Initial tiers:

- low memory: V2-like default/performance policy;
- around 2 GB: large-local-like plan using the best measured subset of heavy
  bounds;
- explicit high memory: more aggressive tail plan if benchmarks prove it.

Heavy bounds are promoted only if they improve tail latency under the release
benchmark gates. Reducing average time is not sufficient if worst-case latency
gets worse.

### Search Policy

V3 should prefer changes that reduce tail latency on certified optimal search.

Candidate areas:

- better distribution of root-parallel work;
- early stop and shared incumbent handling;
- tail-aware child ordering only if benchmarked as a win;
- memory-tiered pruning selection;
- cache-aware table loading.

Previously weak V2 experiments such as phase-2 ordering, strong ordering, and
goal-depth-6 tables should not be promoted without new evidence.

## Benchmark Strategy

V3 desktop validation should expand the V2 benchmark matrix.

Required local desktop suites:

- existing profile-realistic gates;
- existing embedded-multiseed gates;
- existing optimal-stress gates;
- existing large-local depth-15 gates;
- 8-thread tail replay gates;
- new Auto profile gates;
- memory-tier comparison at 1 GB, 2 GB, and explicit larger budgets;
- thread scaling at 1, 2, 4, 8, and available higher counts when useful;
- a larger slow-case corpus extracted from stress and depth-frontier runs.

The primary V3 performance metric is tail latency for `SolveMode::Optimal`.

Release claims must be limited to hardware actually tested. Until embedded
hardware is available, Raspberry Pi, Jetson Nano, and Jetson Orin remain target
classes without published latency numbers.

## Documentation Changes

V3 documentation should make this the recommended optimal path:

```cpp
rubik::SolveResult result = solver.solve(cube, {
    .mode = rubik::SolveMode::Optimal,
    .metric = rubik::Metric::HTM,
    .profile = rubik::SolveProfile::Auto,
    .cachePolicy = rubik::CachePolicy::Auto,
    .threads = 0,
    .maxMemoryBytes = 0,
    .timeout = std::chrono::seconds(30),
});
```

Manual profiles should remain documented for users with strict resource
requirements.

Docs must clearly say:

- `Auto` is recommended for adaptive certified HTM optimal solving;
- `Fast` is still non-optimal;
- cache setup can improve warm-run behavior;
- unsupported combinations return `UnsupportedOptions`;
- hardware claims require direct measurements.

## Compatibility

V3 may add new enum values and fields, but it should not change the meaning of
existing V2 public input formats or the certified HTM optimality contract.

Existing users that select explicit V2 profiles should keep predictable
behavior. The adaptive behavior is opt-in through `SolveProfile::Auto`.

## Open Implementation Questions

- Exact automatic thread cap for desktop plans.
- Exact table/bound set for the first 2 GB Auto plan.
- Cache metadata format and compatibility key.
- Whether `SolvePlan::boundsUsed` should use strings or stable enum values.
- Whether `prepareCache()` belongs in `rubik/solver.hpp` or a new cache header.
- How much setup work `CachePolicy::Auto` may perform during a solve with a
  short timeout.

These questions should be resolved in the implementation plan after benchmark
and code inspection.
