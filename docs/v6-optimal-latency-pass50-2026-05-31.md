# V6 optimal latency pass 50

## Goal

Measure the full conservative-root profile distribution instead of filtering to
the previously selected profiles. This pass checks whether the low target density
comes from overly narrow filters or from genuinely fragmented adaptive profiles.

## Runner Update

`scripts/run_v6_conservative_root_targeted_corpus.sh` now accepts:

```text
--target-profiles all
```

This collects every `conservative_root` discovery row and groups the resulting
rows by `adaptive_lb:adaptive_strong_min_count:adaptive_first_diff`.

## Command

```bash
scripts/run_v6_conservative_root_targeted_corpus.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-all-profiles-pass50 \
  --seeds 42,99,424242,12345,20260525 \
  --random-count 2 \
  --random-start-indices 1,3,5,7 \
  --target-profiles all \
  --min-target-cases 1 \
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

## Density Result

The discovery input covered 5 seeds, 4 start-index windows, and 2 generated
cases per window: 40 discovery cases. The run found 29 conservative-root rows
and 22 distinct adaptive profiles.

The most frequent profiles had only 2 rows each:

| Profile | Cases | Discovery elapsed ms | Discovery nodes |
| --- | ---: | ---: | ---: |
| `8:10:1` | 2 | 4,815 | 28,353,830 |
| `8:16:1` | 2 | 413 | 2,046,926 |
| `8:5:1` | 2 | 13 | 67,108 |
| `8:6:1` | 2 | 146 | 131,044 |
| `8:7:1` | 2 | 4,089 | 23,960,456 |
| `9:2:1` | 2 | 3,054 | 18,523,658 |
| `9:8:1` | 2 | 2,286 | 12,822,359 |

All other observed profiles had one row:

```text
10:12:0
10:14:1
10:18:0
10:8:1
8:11:1
8:13:1
8:14:1
8:15:0
8:4:1
8:5:0
8:9:1
9:14:0
9:18:1
9:2:0
9:3:1
```

## Decision

Do not create another `phase2_tiebreak` replay from this pass. The profile
distribution is too fragmented: even after collecting all conservative-root
rows, no adaptive profile has more than two cases in this sample.

The next useful step is to stop keying experiments by exact
`lb:strong_min_count:first_diff` triples and instead group by coarser buckets,
such as lower bound and strong-minimum-count ranges. That should create denser
groups and make adaptive policy testing less noisy.
