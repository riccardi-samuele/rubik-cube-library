# V6 optimal latency pass 52

## Goal

Replay the densest bucket found in pass51, `lb8_s5-8_fd1`, against the
`phase2_tiebreak` root ordering candidate. This checks whether the broader
bucket is a useful optimization target or just a discovery-density artifact.

## Runner Update

`scripts/run_v6_conservative_root_targeted_corpus.sh` now accepts:

```text
--target-buckets lb8_s5-8_fd1
```

Bucket syntax is `lb<lower_bound>_s<strong_min_range>_fd<first_diff>`, where
`strong_min_range` is one of `0-4`, `5-8`, `9-12`, `13-16`, or `17+`.

When `--target-buckets` is set, it selects targets by bucket instead of exact
`--target-profiles` triples. Existing `--target-profiles` behavior is unchanged
when no bucket filter is provided.

## Command

```bash
scripts/run_v6_conservative_root_targeted_corpus.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-bucket-lb8-s5-8-fd1-pass52 \
  --seeds 42,99,424242,12345,20260525 \
  --random-count 2 \
  --random-start-indices 1,3,5,7 \
  --target-buckets lb8_s5-8_fd1 \
  --min-target-cases 6 \
  --threads 0 \
  --max-memory-mb 2048
```

## Cache State

The run used a warm `large-local` cache:

```text
cache_setup,effective_profile,large-local
cache_setup,cache_warm,true
cache_setup,bytes_missing,0
```

## Replay Result

| Candidate | Cases | Baseline ms | Candidate ms | Delta ms | Delta | Baseline nodes | Candidate nodes | Node delta | Winner |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `phase2_tiebreak` | 6 | 4,303 | 4,282 | -21 | -0.49% | 23,801,758 | 23,648,352 | -153,406 | candidate |

Profile breakdown:

| Profile | Cases | Wins | Losses | Baseline ms | Candidate ms | Delta ms | Delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `8:5:1` | 2 | 1 | 1 | 137 | 136 | -1 | -0.73% |
| `8:6:1` | 2 | 1 | 1 | 145 | 144 | -1 | -0.69% |
| `8:7:1` | 2 | 2 | 0 | 4,021 | 4,002 | -19 | -0.47% |

## Decision

Do not promote `phase2_tiebreak` from this pass alone. The candidate won on the
bucket, but the gain is small and the result still covers only six cases.

The next step is to replay the two four-case pass51 buckets separately:

- `lb8_s9-12_fd1`
- `lb8_s13-16_fd1`

If those replays are also neutral-to-positive, combine the strongest `lb8`
buckets into a larger replay before changing solver behavior.
