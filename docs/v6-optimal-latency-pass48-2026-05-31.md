# V6 optimal latency pass 48

## Goal

Replay the pass 47 discovery-only corpus against `phase2_tiebreak` to determine
whether the newly discovered fourth target row changes the decision.

## Inputs

```text
out/release-native-lto/benchmark-results/v6-conservative-root-targeted-corpus-pass47/targeted_corpus.csv
out/release-native-lto/benchmark-results/v6-conservative-root-targeted-corpus-pass47/targeted_cases.csv
```

The corpus contains 4 rows:

```text
hardening,42,1,15,1,conservative_root
hardening,99,1,15,1,conservative_root
hardening,424242,1,15,1,conservative_root
hardening,424242,8,15,1,conservative_root
```

## Command

```bash
scripts/run_v6_conservative_root_ordering_sweep.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-targeted-replay-pass48 \
  --corpus-file out/release-native-lto/benchmark-results/v6-conservative-root-targeted-corpus-pass47/targeted_corpus.csv \
  --timeout-ms 30000 \
  --threads 0 \
  --max-memory-mb 2048 \
  --candidates phase2_tiebreak
```

The per-profile summaries were generated with:

```bash
scripts/summarize_v6_targeted_corpus.py \
  --targeted-cases out/release-native-lto/benchmark-results/v6-conservative-root-targeted-corpus-pass47/targeted_cases.csv \
  --comparison out/release-native-lto/benchmark-results/v6-conservative-root-targeted-replay-pass48/phase2_tiebreak/comparison.csv \
  --case-output out/release-native-lto/benchmark-results/v6-conservative-root-targeted-replay-pass48/case_summary.csv \
  --profile-output out/release-native-lto/benchmark-results/v6-conservative-root-targeted-replay-pass48/profile_summary.csv
```

## Cache State

The replay used a warm `large-local` cache:

```text
cache_setup,effective_profile,large-local
cache_setup,cache_warm,true
cache_setup,bytes_missing,0
```

## Overall Result

| Candidate | Cases | Baseline ms | Candidate ms | Delta ms | Delta % | Baseline max ms | Candidate max ms | Max delta ms | Baseline nodes | Candidate nodes | Node delta | Winner |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `phase2_tiebreak` | 4 | 8,163 | 8,184 | 21 | 0.26 | 2,914 | 2,881 | -33 | 47,651,895 | 47,196,777 | -455,118 | baseline |

## Profile Summary

| Profile | Cases | Candidate wins | Candidate losses | Baseline ms | Candidate ms | Delta ms | Delta % | Node delta | Winner |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `8:11:1` | 1 | 0 | 1 | 2,291 | 2,342 | 51 | 2.23 | 101,839 | baseline |
| `8:7:1` | 2 | 2 | 0 | 4,094 | 4,054 | -40 | -0.98 | -619,516 | candidate |
| `9:14:0` | 1 | 0 | 1 | 1,778 | 1,788 | 10 | 0.56 | 62,559 | baseline |

## Case Summary

| Case | Profile | Baseline ms | Candidate ms | Delta ms | Winner |
| --- | --- | ---: | ---: | ---: | --- |
| `hardening:depth15:seed42:random_42_1` | `8:11:1` | 2,291 | 2,342 | 51 | baseline |
| `hardening:depth15:seed99:random_99_1` | `8:7:1` | 1,180 | 1,173 | -7 | candidate |
| `hardening:depth15:seed424242:random_424242_1` | `9:14:0` | 1,778 | 1,788 | 10 | baseline |
| `hardening:depth15:seed424242:random_424242_8` | `8:7:1` | 2,914 | 2,881 | -33 | candidate |

## Decision

Do not promote broad `phase2_tiebreak`. The four-row replay remains slightly
slower overall by `21 ms`, despite expanding `455,118` fewer nodes.

The useful signal is narrower: profile `8:7:1` won both measured rows in this
pass. That is still not enough for a solver policy, but it gives a sharper next
experiment: gather more `8:7:1` conservative-root rows and replay only that
profile before considering a narrowly guarded ordering policy.
