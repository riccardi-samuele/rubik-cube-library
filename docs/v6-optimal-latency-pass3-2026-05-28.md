# V6 Optimal Latency Pass 3 - 2026-05-28

This document records a V6 local optimal-latency investigation for the remaining
tail case. No solver policy change was promoted by this pass.

The solver contract is unchanged: `SolveStatus::Optimal` still means a proven
minimum-length HTM solution for the requested options.

## Target

After pass 2, the remaining measured tail target is:

| Benchmark | Solver ms | Wall ms | Nodes |
| --- | ---: | ---: | ---: |
| `random_seed_987654321_depth_15_count_1` | 6963 | 7638 | 23893344 |

The case has this profile:

- `initial_lower_bound=8`;
- `strong_min_count=7`;
- `first_diff=0`;
- adaptive deep-root split enabled;
- depth-2 split with 243 tasks.

## Candidate A: Deeper Root Split

A temporary experimental split-depth probe compared the current depth-2 split
against depth 3 on `random_seed_987654321_depth_15_count_1`.

| Variant | Solver ms | Nodes | Split tasks |
| --- | ---: | ---: | ---: |
| split depth 2 | 6480 | 23881494 | 243 |
| split depth 3 | 7063 | 26452702 | 3321 |

Reading: depth 3 increases both latency and expanded nodes. It was not promoted.

## Candidate B: Forced Strong-Bound Ordering

The same case was run three times with base-bound ordering and three times with
forced strong-bound ordering.

| Variant | Run 1 ms | Run 2 ms | Run 3 ms | Average ms | Average nodes |
| --- | ---: | ---: | ---: | ---: | ---: |
| base-bound | 6459 | 6603 | 6680 | 6581 | 23891244 |
| strong-bound | 6231 | 6247 | 7337 | 6605 | 24038056 |

Reading: forced strong-bound ordering has a similar average, expands more nodes,
and has a worse tail run. It was not promoted.

## Result

Pass 3 deliberately keeps the pass 2 solver policy unchanged. The investigated
candidates did not satisfy the V6 latency goal for the remaining tail case.

The next useful direction is not another root ordering toggle. The evidence
points toward reducing duplicate work or bound-evaluation cost inside the
depth-15 search for the stable lower-bound 8 profile.

These are local desktop measurements only. No external hardware, GPU, or cloud
latency claims are included.
