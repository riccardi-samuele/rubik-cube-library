# V6 optimal latency pass 47

## Goal

Split targeted conservative-root discovery from expensive ordering replay, then
measure target-profile density over a wider start-index window.

## Runner Update

`scripts/run_v6_conservative_root_targeted_corpus.sh` now supports:

```text
--discovery-only
```

In discovery-only mode the runner:

1. validates that the cache is warm;
2. runs default `optimal --profile auto` discovery cases;
3. writes `targeted_corpus.csv`;
4. writes `targeted_cases.csv`;
5. writes `targeted_profile_counts.csv`;
6. exits before running the ordering sweep.

The normal replay mode is unchanged.

## Large Attempt

An initial discovery-only attempt used 10 seeds and 8 start-index windows:

```text
--seeds 42,99,424242,12345,20260525,314159,271828,987654321,7,123456789
--random-count 2
--random-start-indices 1,3,5,7,9,11,13,15
```

That attempted 160 discovery cases and was stopped after about five minutes
because it was too expensive for a single iteration.

## Measured Command

The completed pass used 5 seeds and 4 start-index windows:

```bash
scripts/run_v6_conservative_root_targeted_corpus.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-targeted-corpus-pass47 \
  --seeds 42,99,424242,12345,20260525 \
  --random-count 2 \
  --random-start-indices 1,3,5,7 \
  --target-profiles 8:7:1,8:11:1,9:14:0 \
  --min-target-cases 3 \
  --threads 0 \
  --max-memory-mb 2048 \
  --discovery-only
```

## Cache State

The run used a warm `large-local` cache:

```text
cache_setup,effective_profile,large-local
cache_setup,cache_warm,true
cache_setup,bytes_missing,0
```

## Target Density

The completed discovery input covered 5 seeds, 4 start-index windows, and 2
generated cases per window: 40 discovery cases. It produced 4 target rows.

```text
targeted_cases.csv: 4 data rows
targeted_corpus.csv: 4 data rows
```

Profile counts:

| Profile | Cases | Discovery elapsed ms | Discovery nodes |
| --- | ---: | ---: | ---: |
| `8:11:1` | 1 | 2,299 | 13,429,968 |
| `8:7:1` | 2 | 4,044 | 23,690,183 |
| `9:14:0` | 1 | 1,822 | 10,359,435 |

The additional start-index windows found one extra target row compared with the
previous three-row corpus: seed `424242`, start index `8`, profile `8:7:1`.

## Decision

Do not run an ordering replay from this pass yet. The discovery-only pass shows
that the exact target profiles are still sparse: 4 target rows out of 40
discovery rows. The next useful step is to either:

- replay this 4-row corpus as a cheap confirmation pass; or
- broaden the target profile filter to collect enough rows per profile before
  testing another adaptive ordering policy.

The first option is cheaper and keeps the current thread incremental.
