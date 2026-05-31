# V6 optimal latency pass 72

## Goal

Validate the automatic adaptive-policy promotion of the measured
`positive_high_bound` buckets.

The promoted policy uses `high_bound_first` automatically for:

- `lb8_s13-16_fd1`
- `lb9_s0-3_fd0`
- `lb10_s13-16_fd1`

The `lb9` rule is intentionally limited to `strongMinCount <= 3`. The measured
positive bucket used the broader `lb9_s0-4_fd0` label, but `strongMinCount == 4`
overlaps the existing `reverse_tie` adaptive rule. The replayed positive cases
in that bucket had `strongMinCount` values of 2 and 3, so the promoted rule keeps
the existing `strongMinCount == 4` path unchanged.

`RUBIK_DISABLE_POSITIVE_HIGH_BOUND_ROOT_ORDERING=1` disables only these promoted
positive high-bound buckets. It does not disable the older adaptive
`high_bound_first` rule for `lb8_s6-8_fd0`.

## Replay command

```bash
scripts/run_v6_conservative_root_probe.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-auto-promotion-replay-pass72 \
  --corpus-file out/release-native-lto/benchmark-results/v6-conservative-root-broader-all-profile-discovery-pass70/targeted_corpus.csv \
  --threads 0 \
  --max-memory-mb 2048 \
  --cache-mode require-warm
```

Comparison command:

```bash
scripts/compare_v6_latency.py \
  --baseline-dir out/release-native-lto/benchmark-results/v6-conservative-root-broader-gated-sweep-pass71/positive_high_bound/default \
  --candidate-dir out/release-native-lto/benchmark-results/v6-conservative-root-auto-promotion-replay-pass72/default \
  --output out/release-native-lto/benchmark-results/v6-conservative-root-auto-promotion-replay-pass72/comparison_vs_pass71_default.csv
```

## Result

| Corpus | Cases | Baseline ms | Promoted auto ms | Delta ms | Delta | Baseline max ms | Promoted max ms | Max delta ms | Baseline nodes | Promoted nodes | Node delta | Winner |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| Pass70 broader conservative-root corpus | 68 | 123,404 | 113,207 | -10,197 | -8.26% | 10,505 | 8,630 | -1,875 | 592,486,258 | 572,947,843 | -19,538,415 | promoted auto |

Percentiles:

| Percentile | Baseline ms | Promoted auto ms | Delta ms |
| --- | ---: | ---: | ---: |
| p50 | 1,059 | 1,041 | -18 |
| p90 | 4,442 | 3,669 | -773 |
| p95 | 5,448 | 5,067 | -381 |
| p99 | 8,911 | 6,690 | -2,221 |

Ordering transitions:

| Transition | Cases | Baseline ms | Promoted auto ms | Delta ms | Node delta |
| --- | ---: | ---: | ---: | ---: | ---: |
| `default -> high_bound_first` | 7 | 22,446 | 14,285 | -8,161 | -20,916,963 |
| `default -> default` | 61 | 100,958 | 98,922 | -2,036 | 1,378,548 |

Largest wins:

| Case | Ordering | Baseline ms | Promoted auto ms | Delta ms | Node delta |
| --- | --- | ---: | ---: | ---: | ---: |
| `hardening:depth15:seed12345:random_12345_5` | `default -> high_bound_first` | 10,505 | 6,690 | -3,815 | -5,025,480 |
| `hardening:depth15:seed424242:random_424242_5` | `default -> high_bound_first` | 6,768 | 3,650 | -3,118 | -9,474,151 |
| `hardening:depth15:seed424242:random_424242_6` | `default -> high_bound_first` | 4,442 | 3,222 | -1,220 | -6,416,508 |

Largest regressions:

| Case | Ordering | Baseline ms | Promoted auto ms | Delta ms | Node delta |
| --- | --- | ---: | ---: | ---: | ---: |
| `hardening:depth15:seed12345:random_12345_10` | `default -> default` | 1,542 | 1,573 | 31 | 13,133 |
| `hardening:depth15:seed987654321:random_987654321_2` | `default -> default` | 62 | 66 | 4 | -76 |
| `hardening:depth15:seed271828:random_271828_1` | `default -> high_bound_first` | 139 | 142 | 3 | 8,133 |

## Decision

The promotion is valid on the broader replay corpus. It improves total elapsed
time, p90, p95, p99, max elapsed time, and total nodes. The largest observed
regression is 31 ms and did not come from a promoted `high_bound_first` case.
