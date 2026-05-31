# V6 optimal latency pass 56

## Goal

Mine the existing pass52-pass54 conservative-root bucket replays for stronger
feature signals before proposing another solver policy.

Pass55 closed out `phase2_tiebreak` as a broad policy. This pass adds a
repeatable feature analysis tool and runs it over the measured closeout corpus.

## Tooling

Added `scripts/analyze_v6_conservative_root_features.py`.

The script accepts one or more run directories containing:

- `targeted_cases.csv`
- `case_summary.csv`

It joins cases by benchmark case key, extracts profile fields from
`root_ordering_profile`, and writes:

- per-case feature rows
- grouped feature summaries

Current extracted features:

- bucket, such as `lb8_s5-8_fd1`
- exact profile, such as `8:7:1`
- solution-rank bucket: `1-3`, `4-6`, `7-9`, `10+`
- whether `solution_first` equals `base_first`
- whether `solution_first` equals `strong_first`

## Command

```bash
scripts/analyze_v6_conservative_root_features.py \
  --run-dir out/release-native-lto/benchmark-results/v6-conservative-root-bucket-lb8-s5-8-fd1-pass52 \
  --run-dir out/release-native-lto/benchmark-results/v6-conservative-root-bucket-lb8-s9-12-fd1-pass53 \
  --run-dir out/release-native-lto/benchmark-results/v6-conservative-root-bucket-lb8-s13-16-fd1-pass54 \
  --case-output out/release-native-lto/benchmark-results/v6-conservative-root-phase2-closeout-pass55/case_features.csv \
  --feature-output out/release-native-lto/benchmark-results/v6-conservative-root-phase2-closeout-pass55/feature_summary.csv
```

## Feature Result

Aggregate closeout result:

| Feature | Value | Cases | Wins | Losses | Delta ms | Delta | Node delta | Winner |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `bucket` | `lb8_s5-8_fd1` | 6 | 4 | 2 | -21 | -0.49% | -153,406 | candidate |
| `bucket` | `lb8_s9-12_fd1` | 4 | 1 | 3 | 58 | 0.75% | 188,303 | baseline |
| `bucket` | `lb8_s13-16_fd1` | 4 | 2 | 2 | 38 | 0.38% | 10,290 | baseline |
| `solution_rank_bucket` | `1-3` | 4 | 3 | 1 | -7 | -0.23% | -81,203 | candidate |
| `solution_rank_bucket` | `4-6` | 1 | 0 | 1 | 28 | 0.62% | 141,865 | baseline |
| `solution_rank_bucket` | `10+` | 9 | 4 | 5 | 54 | 0.37% | -15,475 | baseline |
| `solution_matches_base_first` | `1` | 2 | 2 | 0 | -6 | -0.21% | -80,969 | candidate |
| `solution_matches_base_first` | `0` | 12 | 5 | 7 | 81 | 0.42% | 126,156 | baseline |
| `solution_matches_strong_first` | `0` | 14 | 7 | 7 | 75 | 0.34% | 45,187 | baseline |

## Decision

No feature in this small closeout corpus is strong enough to justify a solver
policy.

The most interesting weak signals are:

- `solution_rank_bucket=1-3`: 4 cases, -7 ms, -0.23%
- `solution_matches_base_first=1`: 2 cases, -6 ms, -0.21%

Both are too small and too weak to promote. They are useful only as mining
directions for a larger discovery corpus.

## Next Direction

Extend the feature analysis to richer conservative-root diagnostics before
running another candidate replay. The next useful additions are:

- parse `root_search` to identify whether one heavy child dominates runtime
- parse `root_order` and `solution_rank` together to find late-solution cases
- group by `solution_rank_bucket` over a larger discovery corpus, not just
  pass52-pass54 replay cases

The next implementation step should expand the feature miner, not modify the
solver.
