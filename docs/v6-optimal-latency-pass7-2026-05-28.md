# V6 Optimal Latency Pass 7 - Candidate Bound Cutoff

Date: 2026-05-28

## Goal

Reduce optimal-solver tail latency without changing optimality, public API behavior, or the selected move ordering for surviving candidates.

## Change

`nodeLowerBoundWithoutThreePhase1()` now accepts an optional pruning threshold. Candidate generation passes the remaining-depth threshold used by the existing cheap prune:

```text
lower_bound > limit - depth - 1
```

If an intermediate lower-bound table already exceeds that threshold, the solver returns immediately because the candidate will be pruned either way. If the candidate survives, the function still evaluates the complete lower bound, so ordering and correctness stay unchanged for searched branches.

The same pass also keeps pair-table index calculations local to the table checks that actually use them. This avoids computing unused indices when an earlier table has already proven the candidate impossible under the current depth limit.

## Benchmark Command

```sh
scripts/run_v6_tail_baseline.sh \
  --build-dir out/release-native-lto \
  --output-dir out/release-native-lto/benchmark-results/v6-tail-pass7 \
  --cache-dir /tmp/rubik_cube_library_v6_tail_cache \
  --threads 0 \
  --max-memory-mb 4096 \
  --cache-mode reuse
```

## Results

Corpus: V6 tail baseline, 43 optimal cases, warm table cache.

| Metric | Pass 4 | Pass 7 |
| --- | ---: | ---: |
| Solved | 43 / 43 | 43 / 43 |
| Total solver elapsed | 48972 ms | 35697 ms |
| Total nodes expanded | 169037604 | 169933525 |
| p50 solver elapsed | 523 ms | 383 ms |
| p90 solver elapsed | 3005 ms | 2199 ms |
| p95 solver elapsed | 3129 ms | 2313 ms |
| p99 solver elapsed | 6401 ms | 4464 ms |
| Max solver elapsed | 6437 ms | 4603 ms |

This is a wall-clock and CPU-efficiency improvement rather than a node-count improvement. The solver expands roughly the same number of nodes, but avoids expensive table reads for candidates that are already proven too deep.

## Verification

- `cmake --build --preset release-native-lto --target rubik_tests`
- `ctest --test-dir out/release-native-lto -R rubik_tests --output-on-failure`

Additional release checks are run before committing this pass.
