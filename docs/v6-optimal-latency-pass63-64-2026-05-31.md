# V6 optimal latency pass 63-64

## Goal

Stress the pass62 gate hypothesis around the observed `high_bound_first`
regression bucket:

```text
lb9_s13-16_fd0
```

The intent was to collect a focused corpus containing that bucket and nearby
same-shape buckets, then replay `high_bound_first` against the default ordering.

## Discovery command

```bash
scripts/run_v6_conservative_root_targeted_corpus.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-high-bound-gate-discovery-pass63 \
  --seeds 42,99,424242,12345,20260525 \
  --random-count 2 \
  --random-start-indices 1,3,5,7,9,11,13,15 \
  --target-buckets lb9_s13-16_fd0,lb8_s13-16_fd0,lb10_s13-16_fd0,lb9_s9-12_fd0,lb9_s17+_fd0 \
  --min-target-cases 8 \
  --threads 0 \
  --max-memory-mb 2048 \
  --discovery-only
```

The discovery command intentionally required at least 8 target cases. It found
5, so the command failed the density gate:

```text
v6 conservative root targeted corpus failed: only 5 target cases found, expected at least 8
```

The partial discovery still wrote a targeted corpus, so pass64 replayed those 5
cases as a diagnostic run.

## Replay command

```bash
scripts/run_v6_conservative_root_ordering_sweep.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-high-bound-gate-sweep-pass64 \
  --corpus-file out/release-native-lto/benchmark-results/v6-conservative-root-high-bound-gate-discovery-pass63/targeted_corpus.csv \
  --candidates high_bound_first \
  --threads 0 \
  --max-memory-mb 2048
```

Feature join:

```bash
scripts/analyze_v6_conservative_root_features.py \
  --run-dir out/release-native-lto/benchmark-results/v6-conservative-root-high-bound-gate-discovery-pass63 \
  --discovery-only \
  --case-output out/release-native-lto/benchmark-results/v6-conservative-root-high-bound-gate-discovery-pass63/discovery_case_features.csv \
  --feature-output out/release-native-lto/benchmark-results/v6-conservative-root-high-bound-gate-discovery-pass63/discovery_feature_summary.csv

scripts/analyze_v6_ordering_candidate_features.py \
  --comparison out/release-native-lto/benchmark-results/v6-conservative-root-high-bound-gate-sweep-pass64/high_bound_first/comparison.csv \
  --features out/release-native-lto/benchmark-results/v6-conservative-root-high-bound-gate-discovery-pass63/discovery_case_features.csv \
  --case-output out/release-native-lto/benchmark-results/v6-conservative-root-high-bound-gate-sweep-pass64/high_bound_first/case_features.csv \
  --feature-output out/release-native-lto/benchmark-results/v6-conservative-root-high-bound-gate-sweep-pass64/high_bound_first/feature_summary.csv
```

## Result

| Candidate | Cases | Wins | Losses | Baseline ms | Candidate ms | Delta ms | Delta | Baseline nodes | Candidate nodes | Node delta | Winner |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `high_bound_first` | 5 | 0 | 5 | 5,279 | 8,219 | 2,940 | 55.69% | 31,588,731 | 46,698,121 | 15,109,390 | baseline |

Per bucket:

| Bucket | Cases | Wins | Losses | Baseline ms | Candidate ms | Delta ms | Delta | Worst case delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `lb8_s13-16_fd0` | 3 | 0 | 3 | 195 | 215 | 20 | 10.26% | 19 |
| `lb9_s13-16_fd0` | 1 | 0 | 1 | 1,743 | 4,553 | 2,810 | 161.22% | 2,810 |
| `lb9_s9-12_fd0` | 1 | 0 | 1 | 3,341 | 3,451 | 110 | 3.29% | 110 |

The replay confirms the large regression shape for `lb9_s13-16_fd0`. It also
shows that same-family `fd0` buckets in this small sample do not benefit from
`high_bound_first`.

## Decision

Do not promote `high_bound_first` globally.

Do not implement the pass62 gate yet. Excluding only `lb9_s13-16_fd0` looked
promising in aggregate pass61, but pass64 shows the neighboring `fd0` buckets
also lean negative in this focused sample.

The next hypothesis should move away from `high_bound_first` as a broad policy
and test a narrower candidate that targets only the positive pass61 shapes:

```text
lb8_s13-16_fd1
lb9_s0-4_fd0
lb10_s13-16_fd1
```

Those buckets produced the strongest pass61 aggregate gains, but they need a
focused replay before any solver policy change.
