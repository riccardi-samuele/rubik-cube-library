# V6 optimal latency pass 75-78

## Goal

Validate the promoted positive high-bound root ordering on an independent
all-profile corpus with seeds not used by the previous pass70 broad replay.

## Independent discovery

```bash
scripts/run_v6_conservative_root_targeted_corpus.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-independent-all-profile-discovery-pass75 \
  --seeds 515151,626262,737373,848484,959595,111213,141516,171819,202122,232425 \
  --random-count 2 \
  --random-start-indices 1,3,5,7,9 \
  --target-profiles all \
  --min-target-cases 40 \
  --threads 0 \
  --max-memory-mb 2048 \
  --discovery-only
```

Result:

| Output | Rows |
| --- | ---: |
| `targeted_corpus.csv` | 66 cases |
| `targeted_cases.csv` | 66 cases |
| `targeted_profile_counts.csv` | 33 profiles |

## Broad rollback A/B

Baseline uses the current automatic policy. Candidate disables only the
promoted positive high-bound buckets:

```bash
scripts/run_v6_conservative_root_probe.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-independent-rollback-ab-pass76 \
  --corpus-file out/release-native-lto/benchmark-results/v6-conservative-root-independent-all-profile-discovery-pass75/targeted_corpus.csv \
  --threads 0 \
  --max-memory-mb 2048 \
  --cache-mode require-warm \
  --candidate-env RUBIK_DISABLE_POSITIVE_HIGH_BOUND_ROOT_ORDERING=1
```

Comparison:

```bash
scripts/compare_v6_latency.py \
  --baseline-dir out/release-native-lto/benchmark-results/v6-conservative-root-independent-rollback-ab-pass76/default \
  --candidate-dir out/release-native-lto/benchmark-results/v6-conservative-root-independent-rollback-ab-pass76/candidate \
  --output out/release-native-lto/benchmark-results/v6-conservative-root-independent-rollback-ab-pass76/comparison.csv
```

Result:

| Corpus | Cases | Auto ms | Rollback ms | Delta ms | Delta | Auto max ms | Rollback max ms | Max delta ms | Auto nodes | Rollback nodes | Node delta | Winner |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| Independent all-profile | 66 | 104,544 | 116,085 | 11,541 | 11.04% | 6,799 | 7,756 | 957 | 526,467,440 | 526,420,209 | -47,231 | auto |

Transition summary:

| Transition | Cases | Auto ms | Rollback ms | Delta ms | Auto nodes | Rollback nodes | Node delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `default -> default` | 54 | 86,426 | 95,685 | 9,259 | 435,460,504 | 435,941,092 | 480,588 |
| `high_bound_first -> default` | 12 | 18,118 | 20,400 | 2,282 | 91,006,936 | 90,479,117 | -527,819 |

## Touched-case extraction

The touched corpus was produced from pass76 with:

```bash
scripts/extract_v6_transition_corpus.py \
  --comparison out/release-native-lto/benchmark-results/v6-conservative-root-independent-rollback-ab-pass76/comparison.csv \
  --baseline-ordering high_bound_first \
  --candidate-ordering default \
  --output out/release-native-lto/benchmark-results/v6-conservative-root-independent-rollback-ab-pass76/touched_corpus.csv
```

This produced 12 cases.

## Touched-case replays

Pass77:

| Corpus | Cases | Auto ms | Rollback ms | Delta ms | Delta | Auto max ms | Rollback max ms | Max delta ms | Auto nodes | Rollback nodes | Node delta | Winner |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| Independent touched | 12 | 21,309 | 19,933 | -1,376 | -6.46% | 4,106 | 3,871 | -235 | 91,115,879 | 90,265,260 | -850,619 | rollback |

Pass78 repeated the same touched corpus:

| Corpus | Cases | Auto ms | Rollback ms | Delta ms | Delta | Auto max ms | Rollback max ms | Max delta ms | Auto nodes | Rollback nodes | Node delta | Winner |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| Independent touched repeat | 12 | 19,874 | 19,841 | -33 | -0.17% | 3,784 | 3,657 | -127 | 90,888,461 | 90,485,323 | -403,138 | rollback |

The touched-case measurements are mixed: pass76 favored the current auto
policy on the same transition set, while pass77 favored rollback and pass78 was
near neutral. The large pass74 touched replay remains the strongest positive
evidence for keeping the promoted positive high-bound policy enabled by
default, and pass76 broad replay also favors the current auto policy on the
independent corpus.

## Decision

Keep the promoted positive high-bound policy enabled by default. Keep
`RUBIK_DISABLE_POSITIVE_HIGH_BOUND_ROOT_ORDERING=1` as a selective benchmark and
safety rollback flag. Do not widen or narrow the automatic bucket from these
independent touched replays alone.
