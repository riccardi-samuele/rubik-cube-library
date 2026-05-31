# V6 optimal latency pass 70-71

## Goal

Validate `positive_high_bound` on a broader all-profile conservative-root corpus
before considering promotion into the automatic adaptive policy.

This pass expands beyond the earlier 29-case all-profile corpus and beyond the
positive/negative targeted probes.

## Discovery command

```bash
scripts/run_v6_conservative_root_targeted_corpus.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-broader-all-profile-discovery-pass70 \
  --seeds 42,99,424242,12345,20260525,314159,271828,987654321,7,123456789 \
  --random-count 2 \
  --random-start-indices 1,3,5,7,9 \
  --target-profiles all \
  --min-target-cases 40 \
  --threads 0 \
  --max-memory-mb 2048 \
  --discovery-only
```

The discovery pass found 68 matching conservative-root cases.

## Replay command

```bash
scripts/run_v6_conservative_root_ordering_sweep.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-broader-gated-sweep-pass71 \
  --corpus-file out/release-native-lto/benchmark-results/v6-conservative-root-broader-all-profile-discovery-pass70/targeted_corpus.csv \
  --candidates positive_high_bound \
  --threads 0 \
  --max-memory-mb 2048
```

Feature join:

```bash
scripts/analyze_v6_conservative_root_features.py \
  --run-dir out/release-native-lto/benchmark-results/v6-conservative-root-broader-all-profile-discovery-pass70 \
  --discovery-only \
  --case-output out/release-native-lto/benchmark-results/v6-conservative-root-broader-all-profile-discovery-pass70/discovery_case_features.csv \
  --feature-output out/release-native-lto/benchmark-results/v6-conservative-root-broader-all-profile-discovery-pass70/discovery_feature_summary.csv

scripts/analyze_v6_ordering_candidate_features.py \
  --comparison out/release-native-lto/benchmark-results/v6-conservative-root-broader-gated-sweep-pass71/positive_high_bound/comparison.csv \
  --features out/release-native-lto/benchmark-results/v6-conservative-root-broader-all-profile-discovery-pass70/discovery_case_features.csv \
  --case-output out/release-native-lto/benchmark-results/v6-conservative-root-broader-gated-sweep-pass71/positive_high_bound/case_features.csv \
  --feature-output out/release-native-lto/benchmark-results/v6-conservative-root-broader-gated-sweep-pass71/positive_high_bound/feature_summary.csv
```

## Result

| Candidate | Cases | Wins | Losses | Baseline ms | Candidate ms | Delta ms | Delta | Baseline nodes | Candidate nodes | Node delta | Winner |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `positive_high_bound` | 68 | 21 | 47 | 123,404 | 116,296 | -7,108 | -5.76% | 592,486,258 | 572,162,970 | -20,323,288 | candidate |

The max case improved from 10,505 ms to 8,927 ms.

Largest wins:

| Case | Bucket | Baseline ms | Candidate ms | Delta ms | Node delta |
| --- | --- | ---: | ---: | ---: | ---: |
| `hardening:depth15:seed12345:random_12345_5` | `lb8_s13-16_fd1` | 10,505 | 6,987 | -3,518 | -4,603,371 |
| `hardening:depth15:seed424242:random_424242_5` | `lb9_s0-4_fd0` | 6,768 | 3,798 | -2,970 | -9,445,046 |
| `hardening:depth15:seed424242:random_424242_6` | `lb10_s13-16_fd1` | 4,442 | 3,277 | -1,165 | -6,552,846 |

Largest regressions:

| Case | Bucket | Baseline ms | Candidate ms | Delta ms | Node delta |
| --- | --- | ---: | ---: | ---: | ---: |
| `hardening:depth15:seed42:random_42_7` | `lb10_s17+_fd0` | 2,180 | 2,247 | 67 | 335,602 |
| `hardening:depth15:seed424242:random_424242_8` | `lb8_s5-8_fd1` | 3,244 | 3,305 | 61 | 347,744 |
| `hardening:depth15:seed20260525:random_20260525_6` | `lb9_s0-4_fd1` | 4,102 | 4,157 | 55 | 6,103 |

Per targeted positive bucket:

| Bucket | Cases | Wins | Losses | Baseline ms | Candidate ms | Delta ms | Delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `lb8_s13-16_fd1` | 4 | 2 | 2 | 11,097 | 7,577 | -3,520 | -31.72% |
| `lb9_s0-4_fd0` | 2 | 1 | 1 | 6,907 | 3,944 | -2,963 | -42.90% |
| `lb10_s13-16_fd1` | 1 | 1 | 0 | 4,442 | 3,277 | -1,165 | -26.23% |

The known negative bucket stayed neutral:

| Bucket | Cases | Wins | Losses | Baseline ms | Candidate ms | Delta ms | Delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `lb9_s13-16_fd0` | 2 | 1 | 1 | 3,012 | 3,018 | 6 | 0.20% |

## Decision

`positive_high_bound` remains a valid V6 candidate after the broader replay.

The aggregate win is smaller than in the targeted probes, but it improves total
time, total nodes, and max elapsed time while keeping the largest observed
regressions small. The next step should be one more independently seeded
broader replay or a promotion experiment behind the adaptive policy with a
release-gate comparison against this pass.
