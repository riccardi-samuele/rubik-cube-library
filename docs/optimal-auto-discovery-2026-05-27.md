# Auto Optimal Discovery - 2026-05-27

This run extends the `SolveProfile::Auto` optimal depth-15 tail search with
additional random seeds. It is a discovery run, not a release gate by itself.

Command:

```sh
scripts/run_benchmark_suite.sh \
  --suite optimal-auto-discovery \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_optimal_auto_discovery_smoke_cache \
  --output-dir out/release-native-lto/benchmark-results/optimal-auto-discovery-wide \
  --seeds 1001,1002,1003,1004,1005,1006,1007,1008,1009,1010,1011,1012 \
  --deep-opt15-count 2 \
  --threads 0 \
  --max-memory-mb 2048
```

Configuration:

- Build preset: `release-native-lto`
- Mode: `optimal`
- Requested profile: `auto`
- Effective profile: `large-local`
- Threads: `16`
- Cache mode: warm
- Depth: `15`
- Cases: `24`

Result:

- Solved: `24/24`
- Failed: `0`
- Slowest case: seed `1009`, case `random_1009_1`
- Slowest elapsed: `10477 ms`
- Slowest nodes expanded: `34677060`
- Slowest solution length: `15`

Slowest discovered cases:

| Rank | Seed | Case | Elapsed ms | Nodes | Solution length |
| --- | --- | --- | ---: | ---: | ---: |
| 1 | 1009 | random_1009_1 | 10477 | 34677060 | 15 |
| 2 | 1004 | random_1004_1 | 8209 | 27142785 | 15 |
| 3 | 1011 | random_1011_1 | 7776 | 25357799 | 15 |
| 4 | 1012 | random_1012_2 | 6851 | 24039455 | 15 |
| 5 | 1005 | random_1005_2 | 5952 | 20746201 | 15 |

Decision:

The seed `1009` case is slower than the previous known Auto tail cases while
remaining under the `12000 ms` gate. It is promoted into the repeatable
`rubik-benchmark-optimal-auto-tail` suite and its gate so future changes cannot
regress this tail silently.

## Follow-up Sweep

Command:

```sh
scripts/run_benchmark_suite.sh \
  --suite optimal-auto-discovery \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_optimal_auto_discovery_smoke_cache \
  --output-dir out/release-native-lto/benchmark-results/optimal-auto-discovery-wide-2 \
  --seeds 2001,2002,2003,2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014,2015,2016 \
  --deep-opt15-count 2 \
  --threads 0 \
  --max-memory-mb 2048
```

Result:

- Solved: `32/32`
- Failed: `0`
- Slowest case: seed `2016`, case `random_2016_1`
- Slowest elapsed: `9800 ms`
- Slowest nodes expanded: `24617212`
- Slowest solution length: `15`

Slowest follow-up cases:

| Rank | Seed | Case | Elapsed ms | Nodes | Solution length |
| --- | --- | --- | ---: | ---: | ---: |
| 1 | 2016 | random_2016_1 | 9800 | 24617212 | 15 |
| 2 | 2016 | random_2016_2 | 9088 | 27127250 | 15 |
| 3 | 2014 | random_2014_1 | 7329 | 23836571 | 15 |
| 4 | 2002 | random_2002_2 | 7282 | 24503462 | 15 |
| 5 | 2011 | random_2011_2 | 7123 | 23694722 | 15 |

Decision:

The follow-up sweep did not exceed the promoted seed `1009` tail. No new gate
case is added from this run. Seed `2016` is a useful candidate for future
manual A/B experiments because both sampled cases landed above `9000 ms`.

## Tail A/B Ordering Experiment

The next experiment compared existing ordering variants on the promoted seed
`1009` and the follow-up seed `2016`.

Command shape:

```sh
RUBIK_TABLE_CACHE_DIR=/tmp/rubik_cube_library_optimal_auto_tail_cache \
  out/release-native-lto/rubik-bench \
  --mode optimal \
  --profile auto \
  --threads 0 \
  --max-memory-mb 2048 \
  --timeout-ms 30000 \
  --max-depth 15 \
  --case-set random \
  --random-count 1 \
  --random-depth 15 \
  --random-seed SEED \
  --slowest-count 1 \
  --diagnose-optimal
```

Targeted result:

| Seed | Variant | Elapsed ms | Nodes |
| --- | --- | ---: | ---: |
| 1009 | baseline | 10357 | 34676678 |
| 1009 | strong ordering | 10094 | 33130102 |
| 1009 | phase-2 ordering | 10421 | 34705363 |
| 1009 | goal depth 6 | 13732 | 34713387 |
| 2016 | baseline | 9791 | 24650997 |
| 2016 | strong ordering | 5587 | 19641485 |
| 2016 | phase-2 ordering | 9676 | 24612315 |
| 2016 | goal depth 6 | 12882 | 24667363 |

`strong ordering` is the only promising variant from this targeted run. It
reduced seed `2016` substantially and slightly improved seed `1009`.
`phase-2 ordering` was neutral to worse, and `goal depth 6` increased elapsed
time on both cases.

Full Auto tail check with `RUBIK_EXPERIMENTAL_STRONG_OPTIMAL_ORDERING=1`:

| Seed | Strong elapsed ms | Strong nodes |
| --- | ---: | ---: |
| 987654321 | 7703 | 26219475 |
| 424242 | 3134 | 10263374 |
| 1009 | 10377 | 33183627 |
| 666 | 1061 | 2804441 |
| 555 | 3099 | 9501170 |
| 99 | 3571 | 11404899 |
| 888 | 4328 | 14244652 |

Decision:

Do not promote `strong ordering` globally yet. It improved several heavy tails,
but it also made some previously fast tail cases slower. The next useful step is
to design a selective policy, or gather more evidence about when strong ordering
wins, before changing the default optimal search order.
