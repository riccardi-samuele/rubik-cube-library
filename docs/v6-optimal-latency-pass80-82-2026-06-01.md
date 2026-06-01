# V6 optimal latency pass 80-82

## Goal

Isolate the promoted low-stable lb9 positive high-bound rule:

```text
adaptive_lb=9, adaptive_strong_min_count=1..3, adaptive_first_diff=0
```

The earlier independent touched replay showed mixed case-level stability in
this area, so this pass measures that exact profile group separately.

## Exact-profile discovery

```bash
scripts/run_v6_conservative_root_targeted_corpus.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-lb9-low-stable-discovery-pass80 \
  --seeds 303031,303032,303033,303034,303035,303036,303037,303038,303039,303040,303041,303042 \
  --random-count 2 \
  --random-start-indices 1,3,5,7,9,11 \
  --target-profiles 9:1:0,9:2:0,9:3:0 \
  --min-target-cases 12 \
  --threads 0 \
  --max-memory-mb 2048 \
  --discovery-only
```

The exact-profile density was lower than requested: 4 target cases were found.
Those cases were combined with matching exact-profile cases from pass75,
producing a 10-case corpus:

```text
out/release-native-lto/benchmark-results/v6-conservative-root-lb9-low-stable-combined-pass80/targeted_corpus.csv
```

## Rollback A/B

Baseline uses the current automatic policy. Candidate disables only the
promoted positive high-bound buckets:

```bash
scripts/run_v6_conservative_root_probe.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-lb9-low-stable-rollback-pass81 \
  --corpus-file out/release-native-lto/benchmark-results/v6-conservative-root-lb9-low-stable-combined-pass80/targeted_corpus.csv \
  --threads 0 \
  --max-memory-mb 2048 \
  --cache-mode require-warm \
  --candidate-env RUBIK_DISABLE_POSITIVE_HIGH_BOUND_ROOT_ORDERING=1
```

Pass82 repeated the same corpus:

```bash
scripts/run_v6_conservative_root_probe.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-lb9-low-stable-rollback-repeat-pass82 \
  --corpus-file out/release-native-lto/benchmark-results/v6-conservative-root-lb9-low-stable-combined-pass80/targeted_corpus.csv \
  --threads 0 \
  --max-memory-mb 2048 \
  --cache-mode require-warm \
  --candidate-env RUBIK_DISABLE_POSITIVE_HIGH_BOUND_ROOT_ORDERING=1
```

Results:

| Pass | Cases | Auto ms | Rollback ms | Delta ms | Delta | Auto max ms | Rollback max ms | Max delta ms | Auto nodes | Rollback nodes | Node delta | Winner |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 81 | 10 | 17,872 | 20,528 | 2,656 | 14.86% | 3,621 | 4,338 | 717 | 102,807,392 | 120,753,699 | 17,946,307 | auto |
| 82 | 10 | 17,774 | 20,593 | 2,819 | 15.86% | 3,611 | 4,327 | 716 | 102,879,567 | 120,971,744 | 18,092,177 | auto |

Aggregate:

```bash
scripts/aggregate_v6_replay_comparisons.py \
  --comparison out/release-native-lto/benchmark-results/v6-conservative-root-lb9-low-stable-rollback-pass81/comparison.csv \
  --comparison out/release-native-lto/benchmark-results/v6-conservative-root-lb9-low-stable-rollback-repeat-pass82/comparison.csv \
  --output out/release-native-lto/benchmark-results/v6-conservative-root-lb9-low-stable-rollback-aggregate-pass81-82/comparison_aggregate.csv
```

| Replays | Auto ms | Rollback ms | Delta ms | Delta | Baseline wins | Rollback wins | Neutral | Min delta ms | Max delta ms | Spread ms | Auto nodes | Rollback nodes | Node delta | Winner | Stability |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |
| 2 | 35,646 | 41,121 | 5,475 | 15.36% | 10 | 9 | 1 | -177 | 2,974 | 3,151 | 205,686,959 | 241,725,443 | 36,038,484 | auto | mixed |

Stable case counts:

| Stability | Rows |
| --- | ---: |
| `stable_baseline` | 4 |
| `stable_candidate` | 4 |
| `mixed` | 2 |

## Decision

Keep the promoted low-stable lb9 positive high-bound rule enabled by default.
The aggregate is mixed at case level, but the cost of disabling the rule is
large on this exact-profile corpus because one stable-baseline case regresses
by 5,946 ms and 36,313,152 nodes across the two replays.
