# API Stability - 4.0.0

V4 keeps the V3 certified HTM optimality and Auto planning contracts.

New stable behavior:

- local large-table optimal solves may use adaptive deep-split root scheduling;
- `SolvePlan::rootOrderingProfile` may include adaptive scheduler diagnostics;
- V4 benchmark comparison tooling reports max elapsed fields in addition to
  average elapsed fields.

The public optimality contract is unchanged: `SolveStatus::Optimal` means the
returned HTM solution is proven minimal for the requested options.

## Adaptive Scheduling

For local `SolveMode::Optimal` solves with multiple threads, the solver may use
adaptive deep-split scheduling when the selected profile is the large local
table profile. This applies to `SolveProfile::Auto` when Auto selects that
local profile, and to explicit `SolveProfile::LargeLocal`.

The policy chooses between:

- the existing root-level parallel scheduler;
- the depth-2 deep-split scheduler for measured tail-latency cases.

The policy is deterministic and uses cheap pre-search signals. It does not
change pruning admissibility, cube validation, move metric semantics, or the
minimum-depth proof.

## Diagnostics

`SolvePlan::rootOrderingProfile` remains informational. V4 may append fields
such as:

- `scheduler=adaptive`
- `adaptive_decision=root`
- `adaptive_decision=deep_split`
- `adaptive_reason=...`
- `deep_root_split=enabled`
- `split_tasks=...`

Consumers should continue treating `rootOrderingProfile` as a compact diagnostic
string rather than a stable schema for business logic.

## Experimental Flags

The following environment variables remain experimental benchmark controls:

- `RUBIK_EXPERIMENTAL_DEEP_ROOT_SPLIT=1`
- `RUBIK_EXPERIMENTAL_ADAPTIVE_DEEP_SPLIT=1`

They are not required for normal V4 Auto or large-local optimal solves.

## Compatibility

Source compatibility with the V3 public API is preserved. Existing solve calls,
input formats, validation behavior, CMake package names, and version metadata
remain stable.

The cache file layout, pruning-table internals, and diagnostic string internals
are not part of the stable API contract.
