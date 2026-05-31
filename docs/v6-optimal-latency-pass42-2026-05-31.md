# V6 optimal latency pass 42

## Goal

Inspect root-search diagnostics for the pass 41 `phase2_tiebreak` near miss on
the `conservative_root` depth-15 probe.

## Commands

```bash
scripts/analyze_root_search_profile.py \
  --input-dir out/release-native-lto/benchmark-results/v6-conservative-root-ordering-sweep/phase2_tiebreak/default \
  --reason conservative_root \
  --summary \
  --sort-by solver_elapsed_ms \
  --sort-desc \
  --output out/release-native-lto/benchmark-results/v6-conservative-root-diagnostics/default-summary.csv

scripts/analyze_root_search_profile.py \
  --input-dir out/release-native-lto/benchmark-results/v6-conservative-root-ordering-sweep/phase2_tiebreak/candidate \
  --reason conservative_root \
  --summary \
  --sort-by solver_elapsed_ms \
  --sort-desc \
  --output out/release-native-lto/benchmark-results/v6-conservative-root-diagnostics/candidate-summary.csv
```

The comparison was keyed by benchmark name because `case_name` is not unique
across tail and hardening rows for repeated seeds.

## Cache State

The input artifacts come from pass 41. The pass 41 sweep recorded a warm
`large-local` cache with `bytes_missing=0` for the `phase2_tiebreak` default and
candidate runs.

## Diagnostic Comparison

| Benchmark | Default ms | Candidate ms | Delta ms | Default before solution ms | Candidate before solution ms | Before delta ms | Default solution root ms | Candidate solution root ms | Solution delta ms | Max root delta ms | Worker imbalance delta ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `v6_conservative_root_hardening_depth_15_seed_424242_start_1_count_1` | 1889 | 1863 | -26 | 18187 | 17784 | -403 | 1399 | 1368 | -31 | -31 | 0 |
| `v6_conservative_root_hardening_depth_15_seed_42_start_1_count_1` | 2466 | 2393 | -73 | 16371 | 15867 | -504 | 1819 | 1763 | -56 | -56 | 0 |
| `v6_conservative_root_hardening_depth_15_seed_99_start_1_count_1` | 1166 | 1265 | 99 | 6964 | 7560 | 596 | 580 | 630 | 50 | 49 | -1 |
| `v6_conservative_root_tail_depth_15_seed_424242_start_1_count_1` | 1937 | 1892 | -45 | 18837 | 18135 | -702 | 1449 | 1395 | -54 | -54 | 0 |
| `v6_conservative_root_tail_depth_15_seed_99_start_1_count_1` | 1175 | 1239 | 64 | 6996 | 7428 | 432 | 583 | 619 | 36 | 36 | 0 |

## Observations

`phase2_tiebreak` improves three of the five measured rows. It improves both
`424242` rows and the hardening seed `42` row, while it regresses both seed `99`
rows.

The diagnostic deltas do not show a safe broad discriminator yet. The
before-solution elapsed share is unchanged for the repeated `424242` rows and
nearly unchanged for the `99` rows, so the current summary-level metrics do not
separate wins from losses cleanly.

## Decision

Do not promote a `phase2_tiebreak` policy from these diagnostics alone. The pass
41 aggregate was still slower by `19 ms`, and pass 42 shows mixed per-case
behavior.

The next useful step is to mine the detailed root rows for a discriminator that
separates the `424242`/`42` wins from the `99` losses before creating any narrow
adaptive-policy candidate.
