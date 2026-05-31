# V6 optimal latency pass 46

## Goal

Expand the targeted conservative-root discovery phase across multiple random
start-index windows, then replay the matching profiles against
`phase2_tiebreak`.

## Runner Update

`scripts/run_v6_conservative_root_targeted_corpus.sh` now supports:

```text
--random-start-indices LIST
```

When this option is set, the runner performs discovery for every configured seed
and every configured start index. The older `--random-start-index` option remains
supported and is used when the list option is omitted.

## Command

```bash
scripts/run_v6_conservative_root_targeted_corpus.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-targeted-corpus-pass46 \
  --seeds 42,99,424242,12345,20260525,314159,271828,987654321,7,123456789 \
  --random-count 2 \
  --random-start-indices 1,3,5 \
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

The discovery input covered 10 seeds, 3 start-index windows, and 2 generated
cases per window. It still produced only 3 targeted conservative-root replay
cases for the selected profiles:

```text
targeted_cases.csv: 3 data rows
targeted_corpus.csv: 3 data rows
```

The extra start-index windows did not increase target density for these exact
profiles in this run.

## Overall Result

| Candidate | Cases | Baseline ms | Candidate ms | Delta ms | Delta % | Baseline max ms | Candidate max ms | Max delta ms | Baseline nodes | Candidate nodes | Node delta | Winner |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `phase2_tiebreak` | 3 | 5,593 | 5,302 | -291 | -5.20 | 2,457 | 2,312 | -145 | 29,152,458 | 29,115,411 | -37,047 | candidate |

## Profile Summary

| Profile | Cases | Candidate wins | Candidate losses | Baseline ms | Candidate ms | Delta ms | Delta % | Node delta | Winner |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `8:11:1` | 1 | 1 | 0 | 2,457 | 2,312 | -145 | -5.90 | 51,219 | candidate |
| `8:7:1` | 1 | 1 | 0 | 1,249 | 1,186 | -63 | -5.04 | 39,435 | candidate |
| `9:14:0` | 1 | 1 | 0 | 1,887 | 1,804 | -83 | -4.40 | -127,701 | candidate |

## Decision

Do not promote `phase2_tiebreak` from this pass alone. Pass 46 is favorable, but
it still replayed only three target cases and those cases are the same profiles
that produced mixed or unfavorable results in earlier passes. The current data
shows run-to-run instability on a small corpus, not a robust adaptive policy.

The next useful step is to improve target density before policy work. A better
discovery pass should scan more start-index windows or loosen the profile filter,
then report how many rows each profile contributes before running expensive
ordering comparisons.
