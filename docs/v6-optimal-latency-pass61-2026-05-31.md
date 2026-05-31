# V6 optimal latency pass 61

## Goal

Validate the `high_bound_first` root-ordering candidate from pass60 on the
broader pass50 all-profile conservative-root corpus.

The pass60 late-rank corpus showed a strong aggregate win, but it was targeted
to one feature bucket. This pass checks whether the gain survives on a broader
set before any solver policy change.

## Command

```bash
scripts/run_v6_conservative_root_ordering_sweep.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-all-profiles-high-bound-sweep-pass61 \
  --corpus-file out/release-native-lto/benchmark-results/v6-conservative-root-all-profiles-pass50/targeted_corpus.csv \
  --candidates high_bound_first \
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

## Result

| Candidate | Cases | Wins | Losses | Ties | Baseline ms | Candidate ms | Delta ms | Delta | Baseline nodes | Candidate nodes | Node delta | Winner |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `high_bound_first` | 29 | 8 | 20 | 1 | 48,604 | 44,388 | -4,216 | -8.67% | 257,505,675 | 250,423,012 | -7,082,663 | candidate |

The candidate improves aggregate elapsed time, aggregate expanded nodes, and max
case elapsed time on the all-profile corpus.

The strongest case-level win was:

| Case | Baseline ms | Candidate ms | Delta ms | Delta | Node delta |
| --- | ---: | ---: | ---: | ---: | ---: |
| `hardening:depth15:seed12345:random_12345_5` | 9,563 | 6,119 | -3,444 | -36.01% | -4,802,242 |

The largest regression was:

| Case | Baseline ms | Candidate ms | Delta ms | Delta | Node delta |
| --- | ---: | ---: | ---: | ---: | ---: |
| `hardening:depth15:seed424242:random_424242_1` | 1,811 | 4,777 | 2,966 | 163.78% | 14,403,544 |

## Decision

`high_bound_first` remains a strong V6 candidate because it improved both the
targeted late-rank corpus and the broader all-profile corpus.

Do not promote it globally yet. The all-profile aggregate is positive, but the
candidate still has one large per-case regression. The next step should isolate
the regression case profile and design a gated policy that applies
`high_bound_first` only where the available pre-search features support it.
