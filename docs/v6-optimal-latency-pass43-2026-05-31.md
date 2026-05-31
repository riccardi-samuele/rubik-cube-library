# V6 optimal latency pass 43

## Goal

Mine the pass 42 conservative-root diagnostics for a safe discriminator that
explains when `phase2_tiebreak` helps and when it regresses.

## Inputs

```text
out/release-native-lto/benchmark-results/v6-conservative-root-diagnostics/comparison.csv
out/release-native-lto/benchmark-results/v6-conservative-root-diagnostics/default-summary.csv
out/release-native-lto/benchmark-results/v6-conservative-root-diagnostics/candidate-summary.csv
```

The generated discriminator artifact is:

```text
out/release-native-lto/benchmark-results/v6-conservative-root-discriminator/discriminator.csv
```

## Discriminator Table

| Benchmark | Result | Solver delta ms | LB | Strong min count | First diff | Root node delta |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `v6_conservative_root_hardening_depth_15_seed_424242_start_1_count_1` | win | -26 | 9 | 14 | 0 | -50,213 |
| `v6_conservative_root_hardening_depth_15_seed_42_start_1_count_1` | win | -73 | 8 | 11 | 1 | 204,077 |
| `v6_conservative_root_hardening_depth_15_seed_99_start_1_count_1` | loss | 99 | 8 | 7 | 1 | -16,079 |
| `v6_conservative_root_tail_depth_15_seed_424242_start_1_count_1` | win | -45 | 9 | 14 | 0 | -325,982 |
| `v6_conservative_root_tail_depth_15_seed_99_start_1_count_1` | loss | 64 | 8 | 7 | 1 | 32,475 |

## Observations

The measured losses are cleanly grouped by the adaptive profile
`lb=8`, `strong_min_count=7`, `first_diff=1`. The measured wins are split across
two profiles:

- `lb=9`, `strong_min_count=14`, `first_diff=0`
- `lb=8`, `strong_min_count=11`, `first_diff=1`

Root-node delta alone does not explain the elapsed result. The hardening seed
`42` row wins even though the candidate expands more root nodes, while one seed
`99` loss expands fewer root nodes. The elapsed change is therefore still tied to
root shape and per-root cost, not only aggregate node count.

## Decision

Do not promote `phase2_tiebreak` yet. The discriminator is useful because it
identifies a clearly bad measured profile, but the useful profiles are too small
and partly repeated to justify a solver policy change.

The next useful step is a targeted corpus expansion around these profiles:

- reject or avoid the measured bad profile
  `lb=8`, `strong_min_count=7`, `first_diff=1`;
- collect more rows for `lb=8` with higher strong-minimum counts;
- re-check `lb=9`, `strong_min_count=14`, `first_diff=0` against broader corpus
  data before considering a narrow adaptive policy.
