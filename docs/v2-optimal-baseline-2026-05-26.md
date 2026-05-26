# V2 Optimal Baseline - 2026-05-26

This report records the first V2 optimal-mode baseline after the `1.0.0`
release. The run is intended as the comparison point for future optimal solver
changes.

The measurements below are local desktop measurements only. They are not
Raspberry Pi, Jetson Nano, or Jetson Orin claims.

## Configuration

- Command:

```sh
scripts/run_v2_optimal_baseline.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v2_optimal_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v2-optimal-baseline
```

- Git revision at benchmark time: `9544c60`
- Build preset: `release-native-lto`
- Seeds: `12345`, `20260525`, `42`
- Timeout: 30,000 ms per case
- Depth-13 stress cases: 10 per seed/profile
- Deep probe: not included
- Threads: 1
- Memory budget: 1,024 MB
- Table cache mode: warm

Generated local outputs:

- `out/release-native-lto/benchmark-results/v2-optimal-baseline/manifest.csv`
- `out/release-native-lto/benchmark-results/v2-optimal-baseline/slowest-cases.csv`
- `out/release-native-lto/benchmark-results/v2-optimal-baseline/optimal-stress/warm_optimal_stress_summary.csv`
- `out/release-native-lto/benchmark-results/v2-optimal-baseline/optimal-tail-cases/warm_optimal_tail_cases_summary.csv`

## Optimal Stress Summary

The stress suite ran depth-13 random optimal cases across the `Embedded`,
`Default`, and `Performance` profiles.

| Profile | Seed | Solved | Failed | Total solve ms | p95 ms | Max ms | Nodes expanded |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Embedded | `12345` | 10 | 0 | 21,805 | 7,261 | 7,261 | 14,207,363 |
| Embedded | `20260525` | 10 | 0 | 11,222 | 6,753 | 6,753 | 7,386,908 |
| Embedded | `42` | 10 | 0 | 19,221 | 6,955 | 6,955 | 12,750,730 |
| Default | `12345` | 10 | 0 | 10,726 | 4,244 | 4,244 | 3,080,077 |
| Default | `20260525` | 10 | 0 | 6,706 | 3,217 | 3,217 | 2,011,214 |
| Default | `42` | 10 | 0 | 8,843 | 3,035 | 3,035 | 2,602,342 |
| Performance | `12345` | 10 | 0 | 9,913 | 3,855 | 3,855 | 2,993,467 |
| Performance | `20260525` | 10 | 0 | 6,287 | 3,014 | 3,014 | 1,908,327 |
| Performance | `42` | 10 | 0 | 8,985 | 2,912 | 2,912 | 2,519,940 |

Aggregate stress result: 90 solved, 0 failed.

## Tail-Case Summary

The tail-case suite replayed the known slow embedded depth-13 cases with
optimal diagnostics enabled.

| Case | Status | Moves | Elapsed ms | Nodes expanded |
| --- | --- | ---: | ---: | ---: |
| `random_42_1` | Optimal | 13 | 9,150 | 4,668,997 |
| `random_12345_4` | Optimal | 13 | 8,633 | 4,731,097 |
| `random_20260525_7` | Optimal | 13 | 7,936 | 4,373,576 |
| `random_42_2` | Optimal | 13 | 6,157 | 3,174,144 |
| `random_12345_2` | Optimal | 13 | 5,233 | 2,864,260 |

Aggregate tail result: 5 solved, 0 failed.

## Initial Read

- The current depth-13 optimal path is stable under the benchmarked local
  profiles.
- `Embedded` expands substantially more nodes than `Default` and
  `Performance`, which makes it the clearest target for pruning improvement.
- `Performance` does not dominate `Default` on every wall-clock row, despite
  reducing nodes in several cases. Future profile changes should compare both
  elapsed time and node expansion.
- The slowest replay target for the next optimal-engine iteration is
  `random_42_1` at 9,150 ms and 4,668,997 expanded nodes.

## Next Optimization Targets

1. Replay the top tail cases with experimental admissible pruning combinations.
2. Compare node expansion and elapsed time independently for `Default` and
   `Performance`.
3. Keep `Embedded` within the 1,024 MB memory contract unless a new profile is
   explicitly introduced.
4. Promote no experimental pruning table unless it improves tail latency without
   weakening optimality or memory reporting.
