# V6 optimal latency pass 44

## Goal

Add and run a targeted conservative-root corpus expansion tool for the pass 43
profiles, then compare the default ordering against `phase2_tiebreak` on the
newly generated target corpus.

## Runner

The new runner is:

```text
scripts/run_v6_conservative_root_targeted_corpus.sh
```

It performs three steps:

1. runs default `optimal --profile auto` discovery cases;
2. extracts cases whose adaptive profile matches the configured
   `lb:strong_min_count:first_diff` targets;
3. runs the existing conservative-root ordering sweep on the generated corpus.

The runner does not change solver behavior. It only creates a repeatable way to
mine and replay targeted conservative-root cases.

## Command

```bash
scripts/run_v6_conservative_root_targeted_corpus.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-targeted-corpus-pass44 \
  --seeds 42,99,424242 \
  --random-count 1 \
  --random-start-index 1 \
  --target-profiles 8:7:1,8:11:1,9:14:0 \
  --min-target-cases 3 \
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

## Targeted Cases

| Seed | Start index | Profile | Discovery elapsed ms | Discovery nodes |
| ---: | ---: | --- | ---: | ---: |
| 42 | 1 | `lb=8`, `strong_min_count=11`, `first_diff=1` | 2,388 | 13,409,272 |
| 99 | 1 | `lb=8`, `strong_min_count=7`, `first_diff=1` | 1,200 | 6,119,784 |
| 424242 | 1 | `lb=9`, `strong_min_count=14`, `first_diff=0` | 1,811 | 10,317,780 |

The generated replay corpus is:

```text
out/release-native-lto/benchmark-results/v6-conservative-root-targeted-corpus-pass44/targeted_corpus.csv
```

## Ordering Sweep Result

| Candidate | Cases | Baseline ms | Candidate ms | Delta ms | Delta % | Baseline max ms | Candidate max ms | Max delta ms | Baseline nodes | Candidate nodes | Node delta | Winner |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `phase2_tiebreak` | 3 | 5,279 | 5,616 | 337 | 6.38 | 2,295 | 2,473 | 178 | 29,961,692 | 29,865,629 | -96,063 | baseline |

Per-case elapsed deltas:

| Case | Baseline ms | Candidate ms | Delta ms | Winner |
| --- | ---: | ---: | ---: | --- |
| `hardening:depth15:seed424242:random_424242_1` | 1,809 | 1,905 | 96 | baseline |
| `hardening:depth15:seed42:random_42_1` | 2,295 | 2,473 | 178 | baseline |
| `hardening:depth15:seed99:random_99_1` | 1,175 | 1,238 | 63 | baseline |

## Decision

Do not promote `phase2_tiebreak`. This targeted replay regressed all three
measured profiles, including profiles that had looked promising in pass 42. The
result suggests the previous wins are not stable enough for a solver policy.

The next useful step is to use the new runner with a wider seed set and repeated
runs, then group results by profile before considering any adaptive ordering
change.
