# V6 Optimal Latency Pass 5 - 2026-05-28

This document records a V6 local optimal-latency investigation. No solver policy
or implementation change was promoted by this pass.

The solver contract is unchanged: `SolveStatus::Optimal` still means a proven
minimum-length HTM solution for the requested options.

## Candidate

The investigated candidate added a cheap coordinate prefilter before
`cube.isSolved()` in hot search paths. The idea was to avoid checking the full
cube state for nodes whose maintained coordinates already prove the node cannot
be solved.

The candidate was removed after benchmarking because the full corpus did not
support promotion.

## Targeted Tail Check

The target case was run three times:

| Case | Run 1 ms | Run 2 ms | Run 3 ms | Average ms |
| --- | ---: | ---: | ---: | ---: |
| `random_seed_987654321_depth_15_count_1` | 6258 | 6349 | 6432 | 6346 |

This looked promising against pass 4's targeted average of 6391 ms, so the
candidate advanced to the full corpus gate.

## Corpus Result

Benchmark command:

```sh
scripts/run_v6_tail_baseline.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-pass5-optimized-tail-baseline \
  --tail-seeds 987654321,424242,1009,666,555,99,888 \
  --hardening-seeds 12345,20260525,42,314159,271828,987654321,7,99,123456789,424242,8675309,20240525 \
  --timeout-ms 30000 \
  --threads 0 \
  --max-memory-mb 2048 \
  --deep-opt14-count 2 \
  --deep-opt15-count 1 \
  --cache-mode reuse
```

These are local desktop measurements only. No external hardware, GPU, or cloud
latency claims are included.

## Corpus Comparison

Baseline source: `docs/v6-optimal-latency-pass4-2026-05-28.md`.

| Metric | Pass 4 | Pass 5 candidate | Delta |
| --- | ---: | ---: | ---: |
| Cases | 43 | 43 | 0 |
| Solved | 43 | 43 | 0 |
| Failed | 0 | 0 | 0 |
| Total solver ms | 48972 | 50170 | +1198 |
| Total nodes | 169037604 | 169164205 | +126601 |
| p50 solver ms | 523 | 515 | -8 |
| p90 solver ms | 3005 | 2994 | -11 |
| p95 solver ms | 3129 | 3254 | +125 |
| p99 solver ms | 6401 | 6662 | +261 |
| Max solver ms | 6437 | 6709 | +272 |
| Max wall ms | 7078 | 7377 | +299 |

## Result

The candidate was not promoted. Although the targeted repeat looked slightly
better, the corpus gate regressed total time, p95, p99, max solver time, max
wall time, and nodes. The pass 4 implementation remains the current V6 baseline.
