# V6 Optimal Latency Pass 6 - 2026-05-28

This document records a V6 local optimal-latency investigation. No solver change
was promoted by this pass.

The solver contract is unchanged: `SolveStatus::Optimal` still means a proven
minimum-length HTM solution for the requested options.

## Candidate

The investigated candidate grouped optimal pruning-table references into a
single `OptimalBoundTables` structure, similar to the existing phase-1 bound
table grouping. The intended benefit was to reduce repeated accessor overhead in
the hot lower-bound path.

## Result

The candidate was removed before benchmarking. During verification,
`rubik_tests` did not complete promptly after the change. The likely cause is
that the grouped structure eagerly initialized many optimal pruning tables even
for tests and paths that did not need the full large-local table set.

Because this introduces startup/test latency risk, the change was not promoted.

## Reading

For V6, table-access consolidation must remain lazy. Any future attempt should
avoid eager construction of the complete optimal table set and should prove that
startup behavior is unchanged before running tail benchmarks.

These are local desktop observations only. No external hardware, GPU, or cloud
latency claims are included.
