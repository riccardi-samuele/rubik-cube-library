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
