# Root Bound Tail Baseline - 2026-05-27

Command:

```sh
scripts/benchmark_root_ordering_experiments.sh \
  --cache-dir /tmp/rubik_cube_library_v3_tail_probe_cache \
  --output-dir out/release-native-lto/benchmark-results/root-bound-tail-baseline \
  --seeds 987654321,424242,1009,2016,666,555,99,888 \
  --variants default \
  --timeout-ms 30000 \
  --max-depth 15 \
  --random-depth 15
```

Root-search summary:

```sh
scripts/analyze_root_search_profile.py \
  --input-dir out/release-native-lto/benchmark-results/root-bound-tail-baseline \
  --summary
```

## Results

| Seed | Elapsed ms | Nodes | Solution root rank | Root nodes/ms | Cheap prunes/node ppm | Three-phase prune ppm |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1009 | 11694 | 33627384 | 12 | 196 | 12324616 | 3708 |
| 987654321 | 8868 | 26465061 | 14 | 196 | 12339162 | 3151 |
| 2016 | 6582 | 19718009 | 11 | 196 | 12352813 | 2534 |
| 424242 | 3554 | 10214557 | 14 | 194 | 12344700 | 2903 |
| 555 | 3525 | 9549801 | 7 | 194 | 12331283 | 3802 |
| 99 | 2351 | 6177585 | 13 | 194 | 12333514 | 2356 |
| 888 | 2171 | 5558539 | 16 | 191 | 12351433 | 2392 |
| 666 | 1176 | 2804590 | 13 | 187 | 12304530 | 2885 |

## Interpretation

The root throughput is stable across these depth-15 tail cases: roughly
187-196 root nodes per millisecond in this benchmark run. The cheap candidate
prune density is also stable at about 12.3M parts per million relative to
expanded root nodes.

The slow cases are therefore not isolated root anomalies. They are larger
instances of the same candidate-generation and candidate-pruning workload.

Two local optimization probes were tested and rejected because they worsened
the `1009` tail case:

- deferring order-bound calculation until after cheap candidate pruning;
- lazily computing extra phase-1 direction coordinates only after cheap
  candidate pruning.

The next optimization work should focus on reducing the cost of the candidate
loop itself or increasing pruning strength without disrupting the current
linear hot path.
