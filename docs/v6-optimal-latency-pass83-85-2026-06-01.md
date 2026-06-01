# V6 optimal latency pass 83-85

## Goal

Isolate the promoted high-diff lb8 positive high-bound rule:

```text
adaptive_lb=8, adaptive_strong_min_count=13..16, adaptive_first_diff=1
```

This pass mirrors the lb9 exact-profile rollback replay and checks whether
disabling the promoted positive high-bound rule helps or hurts the lb8
high-diff bucket.

## Exact-profile discovery

```bash
scripts/run_v6_conservative_root_targeted_corpus.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-lb8-high-diff-discovery-pass83 \
  --seeds 404031,404032,404033,404034,404035,404036,404037,404038,404039,404040,404041,404042 \
  --random-count 2 \
  --random-start-indices 1,3,5,7,9,11 \
  --target-profiles 8:13:1,8:14:1,8:15:1,8:16:1 \
  --min-target-cases 12 \
  --threads 0 \
  --max-memory-mb 2048 \
  --discovery-only
```

The exact-profile density was lower than requested: 4 target cases were found.
Those cases were combined with matching exact-profile cases from pass75,
producing a 9-case corpus:

```text
out/release-native-lto/benchmark-results/v6-conservative-root-lb8-high-diff-combined-pass83/targeted_corpus.csv
```

## Rollback A/B

Baseline uses the current automatic policy. Candidate disables only the
promoted positive high-bound buckets:

```bash
scripts/run_v6_conservative_root_probe.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-lb8-high-diff-rollback-pass84 \
  --corpus-file out/release-native-lto/benchmark-results/v6-conservative-root-lb8-high-diff-combined-pass83/targeted_corpus.csv \
  --threads 0 \
  --max-memory-mb 2048 \
  --cache-mode require-warm \
  --candidate-env RUBIK_DISABLE_POSITIVE_HIGH_BOUND_ROOT_ORDERING=1
```

Pass85 repeated the same corpus:

```bash
scripts/run_v6_conservative_root_probe.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-lb8-high-diff-rollback-repeat-pass85 \
  --corpus-file out/release-native-lto/benchmark-results/v6-conservative-root-lb8-high-diff-combined-pass83/targeted_corpus.csv \
  --threads 0 \
  --max-memory-mb 2048 \
  --cache-mode require-warm \
  --candidate-env RUBIK_DISABLE_POSITIVE_HIGH_BOUND_ROOT_ORDERING=1
```

Results:

| Pass | Cases | Auto ms | Rollback ms | Delta ms | Delta | Auto max ms | Rollback max ms | Max delta ms | Auto nodes | Rollback nodes | Node delta | Winner |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 84 | 9 | 16,216 | 21,183 | 4,967 | 30.63% | 5,013 | 7,766 | 2,753 | 78,529,461 | 92,930,725 | 14,401,264 | auto |
| 85 | 9 | 16,162 | 20,184 | 4,022 | 24.89% | 4,933 | 7,016 | 2,083 | 78,388,400 | 93,557,521 | 15,169,121 | auto |

Aggregate:

```bash
scripts/aggregate_v6_replay_comparisons.py \
  --comparison out/release-native-lto/benchmark-results/v6-conservative-root-lb8-high-diff-rollback-pass84/comparison.csv \
  --comparison out/release-native-lto/benchmark-results/v6-conservative-root-lb8-high-diff-rollback-repeat-pass85/comparison.csv \
  --output out/release-native-lto/benchmark-results/v6-conservative-root-lb8-high-diff-rollback-aggregate-pass84-85/comparison_aggregate.csv
```

| Replays | Auto ms | Rollback ms | Delta ms | Delta | Baseline wins | Rollback wins | Neutral | Min delta ms | Max delta ms | Spread ms | Auto nodes | Rollback nodes | Node delta | Winner | Stability |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |
| 2 | 32,378 | 41,367 | 8,989 | 27.76% | 14 | 3 | 1 | -108 | 4,480 | 4,588 | 156,917,861 | 186,488,246 | 29,570,385 | auto | mixed |

Stable case counts:

| Stability | Rows |
| --- | ---: |
| `stable_baseline` | 6 |
| `stable_candidate` | 1 |
| `mixed` | 2 |

## Decision

Keep the promoted high-diff lb8 positive high-bound rule enabled by default.
The rollback repeat is consistently slower on the exact-profile corpus, and one
stable-baseline case regresses by 8,170 ms and 29,440,062 nodes across the two
replays.
