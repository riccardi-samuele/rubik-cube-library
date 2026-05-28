# V6 Optimal Latency Pass 8 - Three-Phase Bound Cutoff

Date: 2026-05-28

## Goal

Reduce residual optimal-solver latency after pass 7 by avoiding unnecessary three-phase phase1 table reads on candidates that are already impossible under the current depth limit.

## Change

`phase1CoordinateLowerBound()` and `nodeThreePhase1LowerBound()` now accept an optional pruning threshold. Candidate generation passes the same remaining-depth threshold used by the existing pruning condition.

If a three-phase phase1 bound exceeds the threshold, the solver can return immediately because that candidate will be pruned. If the candidate survives, the full bound is still evaluated, so strong ordering and optimality remain unchanged for searched candidates.

## Benchmark Command

```sh
scripts/run_v6_tail_baseline.sh \
  --build-dir out/release-native-lto \
  --output-dir out/release-native-lto/benchmark-results/v6-tail-pass8 \
  --cache-dir /tmp/rubik_cube_library_v6_tail_cache \
  --threads 0 \
  --max-memory-mb 4096 \
  --cache-mode reuse
```

## Results

Corpus: V6 tail baseline, 43 optimal cases, warm table cache.

| Metric | Pass 7 | Pass 8 |
| --- | ---: | ---: |
| Solved | 43 / 43 | 43 / 43 |
| Total solver elapsed | 35697 ms | 35426 ms |
| Total nodes expanded | 169933525 | 169754968 |
| p50 solver elapsed | 383 ms | 388 ms |
| p90 solver elapsed | 2199 ms | 2201 ms |
| p95 solver elapsed | 2313 ms | 2263 ms |
| p99 solver elapsed | 4464 ms | 4471 ms |
| Max solver elapsed | 4603 ms | 4546 ms |

The gain is intentionally modest. The pass 7 diagnostics showed many three-phase checks but comparatively few three-phase-only candidate prunes, so this pass mainly trims residual work without changing the search shape.

## Verification

- `cmake --build --preset release-native-lto --target rubik_tests`
- `ctest --test-dir out/release-native-lto -R rubik_tests --output-on-failure`

Additional release checks are run before committing this pass.
