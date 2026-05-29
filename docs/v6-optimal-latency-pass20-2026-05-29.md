# V6 optimal latency pass 20 - depth-14 conservative deep split

Date: 2026-05-29

Status: accepted.

## Goal

Reduce local optimal latency on the remaining depth-14 conservative-root cases
without applying deep root splitting to the slower depth-15 conservative-root
tail.

## Change

The adaptive scheduler now selects deep root splitting for depth-14 cases with:

- `maxDepth == 14`
- `initialLowerBound` 8 or 9
- `strongMinCount >= 2`

The policy reason is `depth14_conservative_root`.

Depth-15 conservative-root cases remain on root scheduling. Earlier pass 20
probes showed forced deep splitting is harmful there.

## TDD check

Added `testAdaptiveSchedulerSelectsV6Depth14ConservativeCase`.

Red phase:

```text
rubik_tests
test expectation failed
```

Green phase:

```text
rubik tests passed
```

## Full V6 corpus

Command:

```bash
scripts/run_v6_tail_baseline.sh \
  --output-dir out/release-native-lto/benchmark-results/v6-tail-pass20 \
  --cache-mode reuse
```

Baseline pass 12:

- cases: 43
- solved: 43
- total solver elapsed: 28861 ms
- total nodes: 169155502
- p50/p90/p95/p99/max solver: 307/1742/1835/3645/3749 ms
- max wall elapsed: 4506 ms

Pass 20:

- cases: 43
- solved: 43
- total solver elapsed: 28037 ms
- total nodes: 165163047
- p50/p90/p95/p99/max solver: 252/1784/1829/3545/3593 ms
- max wall elapsed: 4237 ms

## Group impact

The targeted group changed from root scheduling to deep split:

| Group | Count | Pass 12 solver ms | Pass 20 solver ms | Pass 12 nodes | Pass 20 nodes |
| --- | ---: | ---: | ---: | ---: | ---: |
| depth-14 conservative root | 13 | 2797 | 1967 | 12689554 | 8359979 |

Depth-15 conservative-root cases were not scheduled by the new rule:

| Group | Count | Pass 12 solver ms | Pass 20 solver ms | Pass 12 nodes | Pass 20 nodes |
| --- | ---: | ---: | ---: | ---: | ---: |
| depth-15 conservative root | 10 | 10208 | 10371 | 40947416 | 41398746 |

## Decision

Accept the depth-14 conservative deep-split policy.

It reduces total corpus latency by 824 ms, cuts p50 by 55 ms, improves p95,
p99, max solver, max wall, and reduces total nodes by 3992455. The p90 regresses
by 42 ms, so this is a measured tradeoff rather than a universal improvement.
The improvement is still worthwhile because it reduces total CPU work and the
worst local optimal latency.
