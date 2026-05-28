# API Stability - 5.0.0

V5 keeps the V4 certified HTM optimality and adaptive local planning contracts.

New stable public-usability behavior:

- the primary optimal example uses `SolveMode::Optimal`, `SolveProfile::Auto`,
  `CachePolicy::Auto`, and `threads = 0`;
- the cache setup example uses the public `prepareCache()` API;
- public release validation includes a gate that rejects stale public examples;
- public docs distinguish current release documents from historical release
  documents.

The public optimality contract is unchanged: `SolveStatus::Optimal` means the
returned HTM solution is proven minimal for the requested options.

## Compatibility

Source compatibility with the V4 public API is preserved. Existing solve calls,
input formats, validation behavior, CMake package names, cache setup APIs, and
version metadata remain stable.

`SolveMode::Fast`, phase-1/phase-2 APIs, pruning-table internals, large local
table combinations, environment-variable tuning flags, and diagnostic string
internals remain outside the stable API contract unless documented elsewhere as
stable.
