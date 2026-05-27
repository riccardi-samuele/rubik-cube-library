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

## Candidate Order Bound Probe

A targeted optimization skipped the duplicate `nodeBaseLowerBound()` call used
only for base ordering when strong move ordering is active. In `auto_strong_bound`
cases the actual ordering bound is the already-computed candidate lower bound,
so the base ordering value is not needed.

Command:

```sh
scripts/benchmark_root_ordering_experiments.sh \
  --cache-dir /tmp/rubik_cube_library_v3_tail_probe_cache \
  --output-dir out/release-native-lto/benchmark-results/skip-unused-order-bound-tail \
  --seeds 987654321,424242,1009,2016,666,555,99,888 \
  --variants default \
  --timeout-ms 30000 \
  --max-depth 15 \
  --random-depth 15
```

| Seed | Baseline ms | Optimized ms | Delta ms |
| ---: | ---: | ---: | ---: |
| 1009 | 11694 | 10956 | -738 |
| 2016 | 6582 | 6334 | -248 |
| 424242 | 3554 | 3398 | -156 |
| 555 | 3525 | 3409 | -116 |
| 666 | 1176 | 1102 | -74 |
| 987654321 | 8868 | 8885 | +17 |
| 99 | 2351 | 2353 | +2 |
| 888 | 2171 | 2203 | +32 |

This probe is a net improvement on the current tail set and keeps the hot path
linear. It avoids one redundant lower-bound computation when strong move
ordering is active.

## Base Bound Max Probe

The base lower bound was changed from an initializer-list `std::max` to explicit
linear max updates. This keeps the same table lookups and result while avoiding
initializer-list construction in a hot function.

Command:

```sh
scripts/benchmark_root_ordering_experiments.sh \
  --cache-dir /tmp/rubik_cube_library_v3_tail_probe_cache \
  --output-dir out/release-native-lto/benchmark-results/manual-base-bound-max-tail \
  --seeds 987654321,424242,1009,2016,666,555,99,888 \
  --variants default \
  --timeout-ms 30000 \
  --max-depth 15 \
  --random-depth 15
```

| Seed | Previous ms | Manual max ms | Delta ms |
| ---: | ---: | ---: | ---: |
| 987654321 | 8885 | 8688 | -197 |
| 2016 | 6334 | 6120 | -214 |
| 555 | 3409 | 3341 | -68 |
| 1009 | 10956 | 10933 | -23 |
| 99 | 2353 | 2346 | -7 |
| 888 | 2203 | 2189 | -14 |
| 424242 | 3398 | 3420 | +22 |
| 666 | 1102 | 1113 | +11 |

This probe is a small net improvement on the current tail set and preserves the
same lower-bound values.

## Phase2 Ordering Max Probe

The phase2 ordering lower bound was also changed from initializer-list
`std::max` to explicit linear max updates. This affects the phase2 ordering
diagnostic path and keeps the same lower-bound values.

Commands:

```sh
scripts/benchmark_root_ordering_experiments.sh \
  --cache-dir /tmp/rubik_cube_library_v3_tail_probe_cache \
  --output-dir out/release-native-lto/benchmark-results/phase2-ordering-listmax-baseline \
  --seeds 987654321,424242,1009,2016,666,555,99,888 \
  --variants phase2_tiebreak \
  --timeout-ms 30000 \
  --max-depth 15 \
  --random-depth 15

scripts/benchmark_root_ordering_experiments.sh \
  --cache-dir /tmp/rubik_cube_library_v3_tail_probe_cache \
  --output-dir out/release-native-lto/benchmark-results/phase2-ordering-linear-max-probe \
  --seeds 987654321,424242,1009,2016,666,555,99,888 \
  --variants phase2_tiebreak \
  --timeout-ms 30000 \
  --max-depth 15 \
  --random-depth 15
```

| Seed | List max ms | Linear max ms | Delta ms |
| ---: | ---: | ---: | ---: |
| 987654321 | 9971 | 8717 | -1254 |
| 424242 | 4014 | 3431 | -583 |
| 1009 | 13654 | 10881 | -2773 |
| 2016 | 10476 | 6232 | -4244 |
| 666 | 2059 | 1096 | -963 |
| 555 | 4753 | 3329 | -1424 |
| 99 | 6568 | 2363 | -4205 |
| 888 | 5208 | 2172 | -3036 |

This probe is strongly favorable for the phase2 ordering path.
