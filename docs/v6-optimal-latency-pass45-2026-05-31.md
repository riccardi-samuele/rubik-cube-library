# V6 optimal latency pass 45

## Goal

Add per-profile summaries for the targeted conservative-root corpus runner and
run a wider targeted corpus search for the pass 43 profiles.

## Tooling

The new summary helper is:

```text
scripts/summarize_v6_targeted_corpus.py
```

It joins:

- `targeted_cases.csv`
- `ordering-sweep/phase2_tiebreak/comparison.csv`

and writes:

- `case_summary.csv`
- `profile_summary.csv`

The targeted corpus runner now invokes this helper automatically after the
ordering sweep.

## Command

```bash
scripts/run_v6_conservative_root_targeted_corpus.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-targeted-corpus-pass45 \
  --seeds 42,99,424242,12345,20260525,314159,271828,987654321,7,123456789 \
  --random-count 2 \
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

## Target Density

The wider discovery input covered 10 seeds with 2 generated cases per seed. It
still produced only 3 targeted conservative-root replay cases for the selected
profiles:

```text
targeted_cases.csv: 3 data rows
targeted_corpus.csv: 3 data rows
```

This means these exact profiles remain sparse in the current generated sample.

## Overall Result

| Candidate | Cases | Baseline ms | Candidate ms | Delta ms | Delta % | Baseline max ms | Candidate max ms | Max delta ms | Baseline nodes | Candidate nodes | Node delta | Winner |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `phase2_tiebreak` | 3 | 5,383 | 5,484 | 101 | 1.88 | 2,317 | 2,421 | 104 | 29,877,304 | 29,777,087 | -100,217 | baseline |

## Profile Summary

| Profile | Cases | Candidate wins | Candidate losses | Baseline ms | Candidate ms | Delta ms | Delta % | Node delta | Winner |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `8:11:1` | 1 | 0 | 1 | 2,317 | 2,421 | 104 | 4.49 | -104,797 | baseline |
| `8:7:1` | 1 | 0 | 1 | 1,187 | 1,237 | 50 | 4.21 | -24,704 | baseline |
| `9:14:0` | 1 | 1 | 0 | 1,879 | 1,826 | -53 | -2.82 | 29,284 | candidate |

## Decision

Do not promote `phase2_tiebreak`. The wider run still loses overall, and two of
the three measured profiles regress. The `9:14:0` profile is the only measured
winner in this pass, but it has one data row here and has been unstable across
previous passes.

The next useful step is to make the discovery phase better at finding more target
rows before testing another policy. A practical next iteration is a discovery
window sweep over later random start indices, then replay only the matching
profiles.
