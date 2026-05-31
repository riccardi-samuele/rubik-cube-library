# V6 optimal latency pass 65-66

## Goal

Replay `high_bound_first` only on the positive bucket shapes found in pass61,
after pass63-64 showed that the nearby `fd0` regression family is unsafe as a
broad policy.

Target buckets:

```text
lb8_s13-16_fd1
lb9_s0-4_fd0
lb10_s13-16_fd1
```

## Discovery command

```bash
scripts/run_v6_conservative_root_targeted_corpus.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-high-bound-positive-discovery-pass65 \
  --seeds 42,99,424242,12345,20260525,314159,271828,987654321 \
  --random-count 2 \
  --random-start-indices 1,3,5,7,9,11,13,15 \
  --target-buckets lb8_s13-16_fd1,lb9_s0-4_fd0,lb10_s13-16_fd1 \
  --min-target-cases 10 \
  --threads 0 \
  --max-memory-mb 2048 \
  --discovery-only
```

The discovery pass found 10 matching cases.

| Profile | Cases | Discovery ms | Discovery nodes |
| --- | ---: | ---: | ---: |
| `10:14:1` | 1 | 4,531 | 24,083,616 |
| `10:16:1` | 1 | 8 | 34,171 |
| `8:13:1` | 1 | 3 | 11,746 |
| `8:14:1` | 2 | 12,919 | 51,976,255 |
| `8:16:1` | 2 | 459 | 2,016,310 |
| `9:2:0` | 2 | 6,988 | 30,585,724 |
| `9:3:0` | 1 | 137 | 365,238 |

## Replay command

```bash
scripts/run_v6_conservative_root_ordering_sweep.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-high-bound-positive-sweep-pass66 \
  --corpus-file out/release-native-lto/benchmark-results/v6-conservative-root-high-bound-positive-discovery-pass65/targeted_corpus.csv \
  --candidates high_bound_first \
  --threads 0 \
  --max-memory-mb 2048
```

Feature join:

```bash
scripts/analyze_v6_conservative_root_features.py \
  --run-dir out/release-native-lto/benchmark-results/v6-conservative-root-high-bound-positive-discovery-pass65 \
  --discovery-only \
  --case-output out/release-native-lto/benchmark-results/v6-conservative-root-high-bound-positive-discovery-pass65/discovery_case_features.csv \
  --feature-output out/release-native-lto/benchmark-results/v6-conservative-root-high-bound-positive-discovery-pass65/discovery_feature_summary.csv

scripts/analyze_v6_ordering_candidate_features.py \
  --comparison out/release-native-lto/benchmark-results/v6-conservative-root-high-bound-positive-sweep-pass66/high_bound_first/comparison.csv \
  --features out/release-native-lto/benchmark-results/v6-conservative-root-high-bound-positive-discovery-pass65/discovery_case_features.csv \
  --case-output out/release-native-lto/benchmark-results/v6-conservative-root-high-bound-positive-sweep-pass66/high_bound_first/case_features.csv \
  --feature-output out/release-native-lto/benchmark-results/v6-conservative-root-high-bound-positive-sweep-pass66/high_bound_first/feature_summary.csv
```

## Result

| Candidate | Cases | Wins | Losses | Baseline ms | Candidate ms | Delta ms | Delta | Baseline nodes | Candidate nodes | Node delta | Winner |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `high_bound_first` | 10 | 3 | 7 | 28,603 | 19,499 | -9,104 | -31.83% | 108,903,825 | 87,727,671 | -21,176,154 | candidate |

Per bucket:

| Bucket | Cases | Wins | Losses | Baseline ms | Candidate ms | Delta ms | Delta | Worst case delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `lb10_s13-16_fd1` | 2 | 1 | 1 | 5,550 | 3,440 | -2,110 | -38.02% | 0 |
| `lb8_s13-16_fd1` | 5 | 1 | 4 | 14,948 | 11,735 | -3,213 | -21.49% | 301 |
| `lb9_s0-4_fd0` | 3 | 1 | 2 | 8,105 | 4,324 | -3,781 | -46.65% | 6 |

The largest wins were:

| Case | Bucket | Baseline ms | Candidate ms | Delta ms | Node delta |
| --- | --- | ---: | ---: | ---: | ---: |
| `hardening:depth15:seed424242:random_424242_5` | `lb9_s0-4_fd0` | 7,630 | 3,838 | -3,792 | -9,506,758 |
| `hardening:depth15:seed12345:random_12345_5` | `lb8_s13-16_fd1` | 11,863 | 8,333 | -3,530 | -4,517,261 |
| `hardening:depth15:seed424242:random_424242_6` | `lb10_s13-16_fd1` | 5,482 | 3,372 | -2,110 | -7,019,880 |

The largest regression was:

| Case | Bucket | Baseline ms | Candidate ms | Delta ms | Node delta |
| --- | --- | ---: | ---: | ---: | ---: |
| `hardening:depth15:seed271828:random_271828_13` | `lb8_s13-16_fd1` | 2,458 | 2,759 | 301 | -117,042 |

## Decision

`high_bound_first` is still not a global policy.

It is a viable gated candidate for the positive bucket set because the aggregate
gain is large and the observed regressions are much smaller than the wins. The
next step should implement the gated policy behind an internal candidate mode,
then replay both:

```text
pass61 all-profile corpus
pass63-64 negative fd0 corpus
```

The policy must keep the negative `fd0` family on the default ordering while
applying `high_bound_first` only to the positive bucket set above.
