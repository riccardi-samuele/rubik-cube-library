# Embedded Multiseed Benchmark - 2026-05-25

Host:

- CPU: AMD Ryzen 9 8940HX, 16 cores / 32 threads
- Build preset: `release-native-lto`
- Cache: warm, `/tmp/rubik_cube_library_embedded_multiseed_cache`
- Suite: `embedded-multiseed`
- Seeds: `12345`, `20260525`, `42`
- Counts per seed: fast depth-20 `100`, optimal depth-13 `10`

Command:

```sh
cmake --build out/release-native-lto --target rubik-benchmark-embedded-multiseed
```

Output summary:

```text
out/release-native-lto/benchmark-results/embedded-multiseed/warm_embedded_multiseed_summary.csv
```

## Summary

| Seed | Mode | Benchmark | Cases | Solved | Failed | Avg ms | P50 | P90 | P95 | P99 | Max | Nodes |
| ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 12345 | Fast | depth-20 x100 | 100 | 100 | 0 | 105.10 | 100 | 190 | 208 | 290 | 324 | 28,808,094 |
| 12345 | Optimal | depth-13 x10 | 10 | 10 | 0 | 2,699.10 | 1,051 | 5,481 | 9,260 | 9,260 | 9,260 | 14,207,363 |
| 20260525 | Fast | depth-20 x100 | 100 | 100 | 0 | 121.48 | 111 | 234 | 252 | 320 | 338 | 29,583,234 |
| 20260525 | Optimal | depth-13 x10 | 10 | 10 | 0 | 1,368.90 | 52 | 4,448 | 8,036 | 8,036 | 8,036 | 7,386,908 |
| 42 | Fast | depth-20 x100 | 100 | 100 | 0 | 108.84 | 96 | 203 | 244 | 301 | 385 | 27,712,486 |
| 42 | Optimal | depth-13 x10 | 10 | 10 | 0 | 2,507.80 | 396 | 6,611 | 8,529 | 8,529 | 8,529 | 12,750,730 |

## Interpretation

All 330 embedded cases solved successfully:

- `Embedded/Fast`: 300/300 solved, worst observed case `385 ms`.
- `Embedded/Optimal`: 30/30 solved, worst observed case `9.260 s`.

The previous multiseed run found a heavier fast tail on seed `20260525`
(`p99=751 ms`, `max=852 ms`). The candidate-ordering change reduced the
multiseed fast worst case to `385 ms`, while keeping all cases solved.

For `Embedded/Optimal`, enabling the three-direction phase-1 lower bound by
default reduced the multiseed worst case from `16.815 s` to `9.260 s` without
increasing the embedded pruning-table payload.

## Active Gates

The desktop multiseed gate target checks every current seed:

```sh
cmake --build out/release-native-lto --target rubik-benchmark-embedded-multiseed-gates
```

Current thresholds:

- `Embedded/Fast`: `solved=100`, `p95<=350 ms`, `p99<=500 ms`, `max<=700 ms`
  per seed.
- `Embedded/Optimal`: `solved=10`, `p95/p99/max<=12000 ms` per seed.

These are still desktop numbers. Raspberry Pi and Jetson-class claims require
the same suite on target hardware.

The current slow tail replay is documented in
[Embedded Fast Tail-Case Diagnostics - 2026-05-25](embedded-fast-tail-cases-2026-05-25.md).
