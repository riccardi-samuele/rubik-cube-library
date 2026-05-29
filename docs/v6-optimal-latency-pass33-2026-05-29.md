# V6 optimal latency pass 33

## Goal

Verify the Pass 32 lb8 `high_bound_first` decision with a same-binary A/B test.
Pass 32 compared two commits, so this pass adds a diagnostic switch that can
disable only the new adaptive lb8 high-bound root ordering while leaving the
rest of the solver unchanged.

## Change

`RUBIK_DISABLE_ADAPTIVE_HIGH_BOUND_ROOT_ORDERING=1` now forces
`chooseAdaptiveRootOrdering()` to keep the default ordering for the
`lb8_stable_mid_strong_min` high-bound path.

The switch is intentionally narrow:

- it affects only adaptive root ordering
- it does not disable adaptive deep root split
- values `unset`, empty, or `0` keep the V6 behavior enabled

## Validation

Commands:

```bash
cmake --build out/release-native-lto --target rubik_tests -j2
out/release-native-lto/rubik_tests

ctest --test-dir out/release-native-lto -R '^rubik_tests$' --output-on-failure

scripts/run_v6_tail_baseline.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --cache-mode reuse \
  --output-dir out/release-native-lto/benchmark-results/v6-pass33-ab-enabled \
  --tail-seeds 987654321 \
  --hardening-seeds 987654321,314159 \
  --deep-opt14-count 2 \
  --deep-opt15-count 1 \
  --threads 0 \
  --max-memory-mb 2048 \
  --timeout-ms 30000

RUBIK_DISABLE_ADAPTIVE_HIGH_BOUND_ROOT_ORDERING=1 scripts/run_v6_tail_baseline.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --cache-mode reuse \
  --output-dir out/release-native-lto/benchmark-results/v6-pass33-ab-disabled \
  --tail-seeds 987654321 \
  --hardening-seeds 987654321,314159 \
  --deep-opt14-count 2 \
  --deep-opt15-count 1 \
  --threads 0 \
  --max-memory-mb 2048 \
  --timeout-ms 30000

scripts/compare_v6_latency.py \
  --baseline-dir out/release-native-lto/benchmark-results/v6-pass33-ab-disabled \
  --candidate-dir out/release-native-lto/benchmark-results/v6-pass33-ab-enabled \
  --sort-by elapsed_delta_ms \
  --sort-desc \
  --output out/release-native-lto/benchmark-results/v6-pass33-compare-disabled-enabled.csv

scripts/compare_v6_latency.py \
  --baseline-dir out/release-native-lto/benchmark-results/v6-pass33-ab-disabled \
  --candidate-dir out/release-native-lto/benchmark-results/v6-pass33-ab-enabled \
  --group-by-reason \
  --sort-by elapsed_delta_ms \
  --sort-desc \
  --output out/release-native-lto/benchmark-results/v6-pass33-compare-disabled-enabled-reason-groups.csv
```

## Same-binary A/B

Disabled means `RUBIK_DISABLE_ADAPTIVE_HIGH_BOUND_ROOT_ORDERING=1`.
Enabled means the normal V6 adaptive policy.

| Metric | Disabled | Enabled | Delta | Winner |
| --- | ---: | ---: | ---: | --- |
| Common cases | 7 | 7 | 0 | - |
| Total solver ms | 10010 | 4564 | -5446 | Enabled |
| p50 ms | 268 | 409 | +141 | Disabled |
| p90 ms | 4234 | 1802 | -2432 | Enabled |
| p95 ms | 4234 | 1802 | -2432 | Enabled |
| p99 ms | 4234 | 1802 | -2432 | Enabled |
| Max solver ms | 4968 | 1810 | -3158 | Enabled |
| Total nodes | 51193701 | 21485963 | -29707738 | Enabled |
| Max wall ms | 5777 | 2619 | -3158 | Enabled |

Grouped by adaptive reason:

| Reason | Disabled ms | Enabled ms | Delta ms | Winner |
| --- | ---: | ---: | ---: | --- |
| `lb9_mid_strong_min` | 2 | 3 | +1 | Disabled |
| `depth14_conservative_root` | 456 | 451 | -5 | Enabled |
| `lb8_stable_mid_strong_min` | 9552 | 4110 | -5442 | Enabled |

Rows where the high-bound path changed the ordering:

| Case | Disabled ms | Enabled ms | Delta ms | Ordering change |
| --- | ---: | ---: | ---: | --- |
| tail depth 15 seed 987654321 | 4968 | 1802 | -3166 | default -> high_bound_first |
| hardening depth 15 seed 987654321 | 4234 | 1810 | -2424 | default -> high_bound_first |
| hardening depth 15 seed 314159 | 268 | 409 | +141 | default -> high_bound_first |
| hardening depth 14 seed 314159 | 82 | 89 | +7 | default -> high_bound_first |

## Decision

Keep the Pass 32 policy enabled. The same-binary comparison confirms that the
lb8 high-bound ordering is responsible for the large tail improvement on the
measured target subset. The tradeoff remains visible: two seed 314159 rows get
slower, but the total, p90/p95/p99, max latency, wall latency, and node count
all improve strongly in this targeted run.

The next pass should keep the diagnostic switch available and continue reducing
the remaining non-lb8 tail cases without broadening `high_bound_first` globally.
