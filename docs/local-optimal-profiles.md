# Local Optimal Profiles

The solver is designed as a local-first library. Remote compute can be added by
applications around the library, but the core correctness contract must not
depend on a network service.

## Correctness Contract

`SolveMode::Optimal` has one meaning across every profile:

- `SolveStatus::Optimal`: the returned HTM solution is proven minimal within the
  configured depth limit.
- `SolveStatus::Solved`: the input was already solved, so the minimal solution
  length is zero.
- `SolveStatus::Found`: not used for certified optimal results.
- `SolveStatus::Timeout` or `SolveStatus::DepthLimitExceeded`: no optimal
  answer was proven under the configured limits.

Profiles may change memory use, table selection, move ordering, and latency.
They must not weaken the meaning of `Optimal`.

## Public Profiles

### `SolveProfile::Embedded`

Target devices:

- Raspberry Pi 4/5 class boards;
- embedded Linux devices with limited memory bandwidth;
- local applications where bounded latency is acceptable if it keeps the result
  optimal.

Current table payload: about 110 MB for optimal tables.

Design target: stay comfortably below a 1 GB working memory budget unless the
caller explicitly opts into a larger configuration. This profile enables the
three-direction phase-1 lower bound and corner-state pruning by default for
optimal solving. The corner-state table is admissible and materially reduces
the current tail-case latency while staying well below the embedded memory
budget.

### `SolveProfile::Default`

Target devices:

- normal desktops and laptops;
- server processes that want good optimal performance without very large tables;
- the default public API behavior.

Current table payload: about 294 MB for optimal tables.

This profile enables the three-direction phase-1 lower bound and corner-state
pruning by default for optimal solving. It is the main correctness and
compatibility baseline.

### `SolveProfile::Performance`

Target devices:

- high-end desktops;
- mini PCs and compact workstations;
- high-memory local devices after direct validation;
- workstations where extra memory is acceptable.

Current table payload: about 435 MB for optimal tables.

This profile may use larger admissible tables and more expensive search helpers
when benchmarks show a real wall-clock win. It is the right place for local
high-performance runs.

### `SolveProfile::LargeLocal`

Target devices:

- desktops and workstations with enough RAM for large optimal tables;
- high-memory local compute after direct validation;
- validation runs where certified optimality is more important than memory
  footprint.

Current optimal table payload: about 1.39 GB.

This profile maps to the performance optimal table set and, in
`SolveMode::Optimal`, also enables the corner-state bound plus both
corner/edge-group admissible bounds. It does not require
`RUBIK_EXPERIMENTAL_CORNER_UP_EDGE_BOUNDS` or
`RUBIK_EXPERIMENTAL_CORNER_DOWN_EDGE_BOUNDS`.

`LargeLocal` is not the embedded default policy. It is intended for local
high-memory optimal solving and for repeatable depth-15 validation on
the directly benchmarked host.

### `SolveProfile::Auto`

`Auto` keeps the optimal correctness contract while selecting a local strategy
from the request shape. For optimal HTM searches through depth 13, it resolves
to `Performance` to avoid preparing the larger `LargeLocal` tables for shallow
work. For deeper optimal searches, it resolves to `LargeLocal` and enables the
larger admissible bounds.

`SolveResult::plan` reports the requested profile, effective profile, selected
thread count, memory budget, estimated table payload, and strategy name. Use
those fields when logging or benchmarking adaptive runs.

## Non-Goals For Public Profiles

Do not add a new public profile until there is a measured reason. Experimental
engines, giant pruning tables, and symmetry-reduced table packs can live behind
internal flags while they are being validated.

A future `Extreme` or `Research` profile is acceptable only if it has:

- a stable memory contract;
- benchmark evidence against `Performance`;
- documented cache size and warm-up costs;
- a clear reason to expose it in the public API.

## Accelerator Policy

GPU support is optional and must never be required for correctness.

The required CPU path is:

- complete;
- deterministic;
- available on Raspberry Pi class hardware;
- able to return certified optimal results without an accelerator.

Good first targets for GPU or other accelerators:

- pruning-table generation;
- large batch lower-bound evaluation;
- frontier expansion experiments;
- benchmark tooling for table construction.

Poor first targets:

- replacing the whole recursive IDA* core before the CPU solver has stronger
  admissible pruning;
- making optimal correctness depend on CUDA, OpenCL, or vendor-specific APIs.

## Current Performance Reality

The current optimized desktop report is in
[Realistic Benchmark Report - 2026-05-25](benchmark-realistic-2026-05-25.md).
The important status is:

- random depth-12 optimal cases are fast on the current desktop;
- random depth-13 optimal cases already have multi-second tail latency;
- random depth-20 optimal cases are not yet production-ready under short
  timeouts.

This means the next solver work must focus on stronger admissible pruning and
table architecture, not only compiler flags.

## Release Claims

Before publishing Raspberry Pi, Jetson, or embedded performance claims, every
claim needs a matching benchmark report with:

- hardware model;
- OS and compiler;
- build preset;
- thermal/governor settings;
- cache state;
- profile;
- timeout and depth limits;
- random seed and case count;
- average, percentile, maximum, timeout, and failure data.

Do not publish desktop-derived Raspberry Pi or Jetson latency numbers.

## Implementation Rules

For every public release:

- `maxMemoryBytes` is enforced before search using the active profile's
  estimated pruning-table payload plus small solver overhead;
- `rubik-bench --report-memory` must remain the source of truth for profile
  table payloads;
- cache compatibility must be versioned or invalidated safely as documented in
  [Runtime Behavior](runtime.md);
- profile-specific benchmark suites must cover `Embedded`, `Default`, and
  `Performance`;
- no public profile may return a non-minimal solution with
  `SolveStatus::Optimal`.
