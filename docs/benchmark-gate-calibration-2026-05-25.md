# Benchmark Gate Calibration - 2026-05-25

Host:

- CPU: AMD Ryzen 9 8940HX, 16 cores / 32 threads
- Build preset: `release-native-lto`
- Cache: warm
- Seed: `12345`
- Suite: `profile-realistic`
- Counts: fast depth-20 `100`, optimal depth-12 `20`, optimal depth-13 `10`

This calibration repeated the realistic profile suite three times on the same
desktop. The purpose is to set initial regression gates that catch meaningful
slowdowns without failing on normal run-to-run variance. `Embedded/Fast` was
tightened later after the candidate-ordering improvement, using the refreshed
profile-realistic run and the embedded multiseed suite as guardrails.

## Inputs

Run 1:

```sh
scripts/run_benchmark_suite.sh --suite profile-realistic \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_profile_realistic_large_percentiles_20260525_cache \
  --output-dir out/release-native-lto/benchmark-results/profile-realistic-large-percentiles-2026-05-25 \
  --fast-timeout-ms 5000 \
  --optimal-timeout-ms 30000 \
  --realistic-fast-count 100 \
  --realistic-opt12-count 20 \
  --realistic-opt13-count 10
```

Runs 2 and 3 reused the calibrated counts with
`/tmp/rubik_cube_library_profile_realistic_gate_calibration_20260525_cache` and
separate output directories:

- `profile-realistic-gate-calibration-run2-2026-05-25`
- `profile-realistic-gate-calibration-run3-2026-05-25`

## Key Results

| Profile | Mode | Benchmark | Metric | Run 1 | Run 2 | Run 3 | Gate |
| --- | --- | --- | --- | ---: | ---: | ---: | ---: |
| Embedded | Fast | depth-20 x100 | solved | 100 | 100 | 100 | 100 |
| Embedded | Fast | depth-20 x100 | p95 ms | 300 | 300 | 297 | 400 |
| Embedded | Fast | depth-20 x100 | p99 ms | 572 | 560 | 561 | 750 |
| Embedded | Fast | depth-20 x100 | max ms | 655 | 659 | 648 | 900 |
| Embedded | Optimal | depth-13 x10 | solved | 10 | 10 | 10 | 10 |
| Embedded | Optimal | depth-13 x10 | p95 ms | 14,667 | 16,249 | 14,563 | 22,000 |
| Embedded | Optimal | depth-13 x10 | p99 ms | 14,667 | 16,249 | 14,563 | 22,000 |
| Embedded | Optimal | depth-13 x10 | max ms | 14,667 | 16,249 | 14,563 | 22,000 |
| Default | Optimal | depth-13 x10 | p95/p99/max ms | 3,838 | 3,574 | 3,785 | 5,500 |
| Performance | Optimal | depth-13 x10 | p95/p99/max ms | 3,889 | 3,871 | 3,750 | 5,500 |

After the embedded fast candidate-ordering improvement, the refreshed
profile-realistic run measured `Embedded/Fast` at p95 `218 ms`, p99 `280 ms`,
and max `294 ms`. The active `Embedded/Fast` gate is therefore tighter than the
original calibration row: p95 `350 ms`, p99 `500 ms`, max `700 ms`.

After enabling three-direction phase-1 lower bounds for `Embedded/Optimal`, the
refreshed profile-realistic run measured `Embedded/Optimal` depth-13 x10 at p95
`9,776 ms`, p99 `9,776 ms`, and max `9,776 ms`. The active embedded optimal gate
is therefore tighter than the original calibration row: p95/p99/max
`12,000 ms`.

## Active Gates

The calibrated CMake target now runs the profile-realistic suite with
`100/20/10` counts and checks:

```sh
scripts/check_benchmark_gates.sh \
  --summary-file out/release-native-lto/benchmark-results/profile-realistic/warm_profile_realistic_summary.csv \
  --gate embedded,fast,random_depth_20_count_100,100,350,500,700 \
  --gate embedded,optimal,random_depth_13_count_10,10,12000,12000,12000 \
  --gate default,optimal,random_depth_13_count_10,10,5500,5500,5500 \
  --gate performance,optimal,random_depth_13_count_10,10,5500,5500,5500
```

Run with:

```sh
cmake --build out/release-native-lto --target rubik-benchmark-profile-realistic
cmake --build out/release-native-lto --target rubik-benchmark-profile-realistic-gates
```

These are desktop regression gates, not Raspberry Pi claims. Before publishing
embedded guarantees, repeat this calibration on the target hardware.
