# V6 optimal latency pass 67-69

## Goal

Implement and replay a gated internal root-ordering candidate:

```text
RUBIK_EXPERIMENTAL_ROOT_ORDERING=positive_high_bound
```

The mode applies `high_bound_first` only to the positive bucket set validated in
pass65-66:

```text
lb8_s13-16_fd1
lb9_s0-4_fd0
lb10_s13-16_fd1
```

All other buckets remain on the default root ordering. This pass validates the
candidate on the positive corpus, the negative `fd0` corpus, and the broader
all-profile corpus.

## Commands

Positive corpus:

```bash
scripts/run_v6_conservative_root_ordering_sweep.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-positive-gated-sweep-pass67 \
  --corpus-file out/release-native-lto/benchmark-results/v6-conservative-root-high-bound-positive-discovery-pass65/targeted_corpus.csv \
  --candidates positive_high_bound \
  --threads 0 \
  --max-memory-mb 2048
```

Negative `fd0` corpus:

```bash
scripts/run_v6_conservative_root_ordering_sweep.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-negative-gated-sweep-pass68 \
  --corpus-file out/release-native-lto/benchmark-results/v6-conservative-root-high-bound-gate-discovery-pass63/targeted_corpus.csv \
  --candidates positive_high_bound \
  --threads 0 \
  --max-memory-mb 2048
```

All-profile corpus:

```bash
scripts/run_v6_conservative_root_ordering_sweep.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-all-profiles-gated-sweep-pass69 \
  --corpus-file out/release-native-lto/benchmark-results/v6-conservative-root-all-profiles-pass50/targeted_corpus.csv \
  --candidates positive_high_bound \
  --threads 0 \
  --max-memory-mb 2048
```

## Result

| Corpus | Cases | Baseline ms | Candidate ms | Delta ms | Delta | Baseline nodes | Candidate nodes | Node delta | Winner |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| Positive pass65 | 10 | 25,000 | 17,210 | -7,790 | -31.16% | 108,695,333 | 87,801,926 | -20,893,407 | candidate |
| Negative pass63 | 5 | 6,185 | 6,282 | 97 | 1.57% | 32,081,778 | 32,470,398 | 388,620 | baseline |
| All-profile pass50 | 29 | 54,952 | 47,477 | -7,475 | -13.60% | 258,550,447 | 237,308,070 | -21,242,377 | candidate |

The all-profile max case improved from 10,515 ms to 6,961 ms.

The previously large regression case stayed on default ordering:

| Case | Bucket | Baseline ms | Candidate ms | Delta ms | Candidate ordering |
| --- | --- | ---: | ---: | ---: | --- |
| `hardening:depth15:seed424242:random_424242_1` | `lb9_s13-16_fd0` | 2,042 | 2,047 | 5 | `default` |

The largest all-profile regression after gating was:

| Case | Bucket | Baseline ms | Candidate ms | Delta ms |
| --- | --- | ---: | ---: | ---: |
| `hardening:depth15:seed99:random_99_4` | `lb8_s9-12_fd1` | 5,186 | 5,356 | 170 |

The strongest all-profile wins were:

| Case | Bucket | Baseline ms | Candidate ms | Delta ms |
| --- | --- | ---: | ---: | ---: |
| `hardening:depth15:seed12345:random_12345_5` | `lb8_s13-16_fd1` | 10,515 | 6,961 | -3,554 |
| `hardening:depth15:seed424242:random_424242_5` | `lb9_s0-4_fd0` | 6,739 | 3,857 | -2,882 |
| `hardening:depth15:seed424242:random_424242_6` | `lb10_s13-16_fd1` | 4,584 | 3,255 | -1,329 |

## Decision

`positive_high_bound` is a better V6 candidate than global `high_bound_first`.

It preserves the large positive-bucket wins, avoids the previously large
`lb9_s13-16_fd0` regression, and improves the all-profile corpus aggregate.
The next step should run a broader multi-seed replay before promoting it into
the automatic adaptive policy.
