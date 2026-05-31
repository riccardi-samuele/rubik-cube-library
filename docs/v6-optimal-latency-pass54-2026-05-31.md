# V6 optimal latency pass 54

## Goal

Replay the third dense pass51 bucket, `lb8_s13-16_fd1`, against the
`phase2_tiebreak` root ordering candidate. Pass53 was negative for
`lb8_s9-12_fd1`; this pass decides whether the candidate has any remaining
reason to continue as a broad `lb8` conservative-root policy.

## Command

```bash
scripts/run_v6_conservative_root_targeted_corpus.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-bucket-lb8-s13-16-fd1-pass54 \
  --seeds 42,99,424242,12345,20260525 \
  --random-count 2 \
  --random-start-indices 1,3,5,7 \
  --target-buckets lb8_s13-16_fd1 \
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
| `phase2_tiebreak` | 4 | 9,938 | 9,976 | 38 | 0.38% | 42,475,751 | 42,486,041 | 10,290 | baseline |

Profile breakdown:

| Profile | Cases | Wins | Losses | Baseline ms | Candidate ms | Delta ms | Delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `8:13:1` | 1 | 1 | 0 | 65 | 64 | -1 | -1.54% |
| `8:14:1` | 1 | 0 | 1 | 9,411 | 9,449 | 38 | 0.40% |
| `8:16:1` | 2 | 1 | 1 | 462 | 463 | 1 | 0.22% |

## Decision

Close out `phase2_tiebreak` as a broad `lb8` conservative-root policy. The
three bucket replays now show:

| Bucket | Cases | Delta ms | Delta | Winner |
| --- | ---: | ---: | ---: | --- |
| `lb8_s5-8_fd1` | 6 | -21 | -0.49% | candidate |
| `lb8_s9-12_fd1` | 4 | 58 | 0.75% | baseline |
| `lb8_s13-16_fd1` | 4 | 38 | 0.38% | baseline |

The only positive bucket was small and weak, while the next two dense buckets
were negative. Do not change solver behavior from this candidate.

The next optimization work should shift away from this root ordering tweak and
look for a stronger discriminator or a different latency lever.
