# V6 optimal latency pass 60

## Goal

Replay existing non-closed root-ordering candidates on the pass59 late-rank
corpus before creating a new solver policy.

This pass tests whether an already supported ordering mode is a good candidate
for `solution_rank_bucket=10+` conservative-root cases.

## Command

```bash
scripts/run_v6_conservative_root_ordering_sweep.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-late-rank-ordering-sweep-pass60 \
  --corpus-file out/release-native-lto/benchmark-results/v6-conservative-root-late-rank-discovery-pass59/targeted_corpus.csv \
  --candidates reverse_tie,high_bound_first \
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

| Candidate | Cases | Wins | Losses | Baseline ms | Candidate ms | Delta ms | Delta | Baseline nodes | Candidate nodes | Node delta | Winner |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `reverse_tie` | 14 | 5 | 9 | 30,738 | 30,206 | -532 | -1.73% | 157,435,810 | 164,077,546 | 6,641,736 | candidate |
| `high_bound_first` | 14 | 8 | 6 | 31,297 | 27,664 | -3,633 | -11.61% | 157,096,120 | 156,883,576 | -212,544 | candidate |

`reverse_tie` improves total elapsed time but expands substantially more nodes.
`high_bound_first` improves total elapsed time, max elapsed time, and total
nodes on this corpus.

The strongest `high_bound_first` case-level win was:

| Case | Baseline ms | Candidate ms | Delta ms | Delta |
| --- | ---: | ---: | ---: | ---: |
| `hardening:depth15:seed12345:random_12345_5` | 9,606 | 6,106 | -3,500 | -36.44% |

The largest `high_bound_first` regression was:

| Case | Baseline ms | Candidate ms | Delta ms | Delta |
| --- | ---: | ---: | ---: | ---: |
| `hardening:depth15:seed424242:random_424242_1` | 1,808 | 4,625 | 2,817 | 155.81% |

## Decision

`high_bound_first` is the first strong late-rank candidate in this V6 line. Do
not promote it yet: the corpus is feature-targeted and has one large regression.

The next step should validate `high_bound_first` against a broader
conservative-root corpus, including non-late-rank cases, to check whether the
late-rank gain comes with unacceptable regressions elsewhere.
