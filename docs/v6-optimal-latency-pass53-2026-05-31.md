# V6 optimal latency pass 53

## Goal

Replay the second densest pass51 bucket, `lb8_s9-12_fd1`, against the
`phase2_tiebreak` root ordering candidate. Pass52 showed a small positive result
for `lb8_s5-8_fd1`; this pass checks whether the signal extends to nearby
`lb8` conservative-root buckets.

## Command

```bash
scripts/run_v6_conservative_root_targeted_corpus.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-bucket-lb8-s9-12-fd1-pass53 \
  --seeds 42,99,424242,12345,20260525 \
  --random-count 2 \
  --random-start-indices 1,3,5,7 \
  --target-buckets lb8_s9-12_fd1 \
  --min-target-cases 4 \
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
| `phase2_tiebreak` | 4 | 7,778 | 7,836 | 58 | 0.75% | 44,420,899 | 44,609,202 | 188,303 | baseline |

Profile breakdown:

| Profile | Cases | Wins | Losses | Baseline ms | Candidate ms | Delta ms | Delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `8:10:1` | 2 | 0 | 2 | 4,809 | 4,838 | 29 | 0.60% |
| `8:11:1` | 1 | 0 | 1 | 2,307 | 2,340 | 33 | 1.43% |
| `8:9:1` | 1 | 1 | 0 | 662 | 658 | -4 | -0.60% |

## Decision

Do not promote `phase2_tiebreak` as a broad `lb8` conservative-root policy. This
bucket moved in the opposite direction from pass52: candidate was slower and
expanded more nodes.

The next useful step is still to replay `lb8_s13-16_fd1`, but promotion now
requires a stricter condition: it must either show a materially stronger win on
that bucket or lead to a narrower discriminator than the current bucket family.
If it is neutral or negative, this ordering candidate should be closed out.
