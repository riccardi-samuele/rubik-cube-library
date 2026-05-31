# V6 optimal latency pass 59

## Goal

Make the pass58 late-solution-rank signal replayable by adding feature targeting
to the conservative-root targeted corpus runner.

This pass does not change solver behavior. It only adds a way to construct a
target corpus from feature values discovered by the miner.

## Runner Update

`scripts/run_v6_conservative_root_targeted_corpus.sh` now accepts:

```text
--target-feature solution_rank_bucket=10+
```

Supported values are:

- `solution_rank_bucket=1-3`
- `solution_rank_bucket=4-6`
- `solution_rank_bucket=7-9`
- `solution_rank_bucket=10+`

When `--target-feature` is set, it takes precedence over `--target-buckets` and
`--target-profiles`. Existing profile and bucket filtering is unchanged when no
feature target is provided.

## Command

```bash
scripts/run_v6_conservative_root_targeted_corpus.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-late-rank-discovery-pass59 \
  --seeds 42,99,424242,12345,20260525 \
  --random-count 2 \
  --random-start-indices 1,3,5,7 \
  --target-feature solution_rank_bucket=10+ \
  --min-target-cases 14 \
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

## Discovery Result

The feature filter selected 14 conservative-root cases:

| Cases | Discovery elapsed ms | Discovery nodes |
| ---: | ---: | ---: |
| 14 | 30,414 | 156,774,065 |

Profile breakdown:

| Profile | Cases | Discovery elapsed ms | Discovery nodes |
| --- | ---: | ---: | ---: |
| `10:8:1` | 1 | 2,713 | 16,492,505 |
| `8:10:1` | 1 | 278 | 1,291,433 |
| `8:11:1` | 1 | 2,224 | 13,285,185 |
| `8:14:1` | 1 | 9,396 | 40,446,206 |
| `8:16:1` | 2 | 403 | 2,030,586 |
| `8:6:1` | 2 | 146 | 130,351 |
| `8:7:1` | 1 | 1,152 | 6,132,605 |
| `8:9:1` | 1 | 649 | 2,688,500 |
| `9:14:0` | 1 | 1,766 | 10,298,922 |
| `9:2:0` | 1 | 6,018 | 29,212,955 |
| `9:3:1` | 1 | 3,522 | 22,091,875 |
| `9:8:1` | 1 | 2,147 | 12,672,942 |

## Decision

The late-rank filter is viable: it reproduces the pass58 density target and
produces a 14-case corpus with high total discovery cost.

Do not replay `phase2_tiebreak` on this corpus, because pass55 already closed
that candidate. The next step should define a new late-rank-specific candidate,
then replay it against this corpus.
