# V6 optimal latency pass 73-74

## Goal

Validate the selective rollback flag for promoted positive high-bound root
ordering:

```bash
RUBIK_DISABLE_POSITIVE_HIGH_BOUND_ROOT_ORDERING=1
```

The flag disables only the promoted positive high-bound buckets. It leaves the
older adaptive `high_bound_first` rule enabled.

## Broad rollback A/B command

```bash
scripts/run_v6_conservative_root_probe.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-positive-rollback-ab-pass73 \
  --corpus-file out/release-native-lto/benchmark-results/v6-conservative-root-broader-all-profile-discovery-pass70/targeted_corpus.csv \
  --threads 0 \
  --max-memory-mb 2048 \
  --cache-mode require-warm \
  --candidate-env RUBIK_DISABLE_POSITIVE_HIGH_BOUND_ROOT_ORDERING=1
```

Comparison:

```bash
scripts/compare_v6_latency.py \
  --baseline-dir out/release-native-lto/benchmark-results/v6-conservative-root-positive-rollback-ab-pass73/default \
  --candidate-dir out/release-native-lto/benchmark-results/v6-conservative-root-positive-rollback-ab-pass73/candidate \
  --output out/release-native-lto/benchmark-results/v6-conservative-root-positive-rollback-ab-pass73/comparison.csv
```

The broad A/B run is useful as a safety check, but the aggregate is not a clean
promotion measurement because unrelated `default -> default` cases varied
substantially between the two replay halves.

Transition summary:

| Transition | Cases | Auto ms | Rollback ms | Delta ms | Node delta |
| --- | ---: | ---: | ---: | ---: | ---: |
| `high_bound_first -> default` | 7 | 17,969 | 22,170 | 4,201 | 21,100,916 |
| `default -> default` | 61 | 123,376 | 101,280 | -22,096 | -1,012,968 |

## Touched-case replay

The touched-case replay isolates only the 7 cases whose ordering changes under
the rollback flag.

```bash
scripts/run_v6_conservative_root_probe.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-positive-rollback-touched-pass74 \
  --corpus-file out/release-native-lto/benchmark-results/v6-conservative-root-positive-rollback-ab-pass73/touched_corpus.csv \
  --threads 0 \
  --max-memory-mb 2048 \
  --cache-mode require-warm \
  --candidate-env RUBIK_DISABLE_POSITIVE_HIGH_BOUND_ROOT_ORDERING=1
```

Comparison:

```bash
scripts/compare_v6_latency.py \
  --baseline-dir out/release-native-lto/benchmark-results/v6-conservative-root-positive-rollback-touched-pass74/default \
  --candidate-dir out/release-native-lto/benchmark-results/v6-conservative-root-positive-rollback-touched-pass74/candidate \
  --output out/release-native-lto/benchmark-results/v6-conservative-root-positive-rollback-touched-pass74/comparison.csv
```

Result:

| Corpus | Cases | Auto ms | Rollback ms | Delta ms | Delta | Auto max ms | Rollback max ms | Max delta ms | Auto nodes | Rollback nodes | Node delta | Winner |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| Promoted touched cases | 7 | 14,438 | 22,210 | 7,772 | 53.83% | 6,814 | 10,314 | 3,500 | 75,034,351 | 95,401,388 | 20,367,037 | auto |

Per-case result:

| Case | Auto ms | Rollback ms | Delta ms | Node delta |
| --- | ---: | ---: | ---: | ---: |
| `hardening:depth15:seed12345:random_12345_2` | 406 | 401 | -5 | 11,576 |
| `hardening:depth15:seed12345:random_12345_5` | 6,814 | 10,314 | 3,500 | 4,717,495 |
| `hardening:depth15:seed271828:random_271828_1` | 142 | 135 | -7 | 5,917 |
| `hardening:depth15:seed424242:random_424242_5` | 3,707 | 6,836 | 3,129 | 9,422,378 |
| `hardening:depth15:seed424242:random_424242_6` | 3,200 | 4,355 | 1,155 | 6,216,487 |
| `hardening:depth15:seed99:random_99_5` | 102 | 104 | 2 | 331 |
| `hardening:depth15:seed99:random_99_6` | 67 | 65 | -2 | -7,147 |

## Decision

The selective rollback flag works as a benchmark and safety control. The
touched-case replay supports keeping the promoted positive high-bound policy
enabled by default.
