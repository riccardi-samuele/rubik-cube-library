# V6 optimal latency pass 57

## Goal

Extend the conservative-root feature miner with richer `root_order` and
`root_search` features, then rerun the analysis on the pass52-pass54 closeout
corpus.

Pass56 showed only weak signals. This pass checks whether root-child dominance
or solution-root status gives a stronger discriminator before any solver change.

## Tooling Update

`scripts/analyze_v6_conservative_root_features.py` now extracts:

- `solution_order_rank`
- `solution_order_rank_bucket`
- `solution_root_status`
- `dominant_child_share_percent`
- `dominant_child_share_bucket`
- `dominant_child_move`

The parser treats missing `root_order` or `root_search` fields as `unknown`, so
older or smaller fixtures remain analyzable.

## Command

```bash
scripts/analyze_v6_conservative_root_features.py \
  --run-dir out/release-native-lto/benchmark-results/v6-conservative-root-bucket-lb8-s5-8-fd1-pass52 \
  --run-dir out/release-native-lto/benchmark-results/v6-conservative-root-bucket-lb8-s9-12-fd1-pass53 \
  --run-dir out/release-native-lto/benchmark-results/v6-conservative-root-bucket-lb8-s13-16-fd1-pass54 \
  --case-output out/release-native-lto/benchmark-results/v6-conservative-root-phase2-closeout-pass55/case_features_v2.csv \
  --feature-output out/release-native-lto/benchmark-results/v6-conservative-root-phase2-closeout-pass55/feature_summary_v2.csv
```

## Result

The richer feature set did not produce a stronger discriminator in the closeout
corpus.

| Feature | Value | Cases | Wins | Losses | Delta ms | Delta | Node delta | Winner |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `dominant_child_share_bucket` | `low` | 14 | 7 | 7 | 75 | 0.34% | 45,187 | baseline |
| `solution_root_status` | `found` | 14 | 7 | 7 | 75 | 0.34% | 45,187 | baseline |
| `solution_order_rank_bucket` | `1-3` | 4 | 3 | 1 | -7 | -0.23% | -81,203 | candidate |
| `solution_order_rank_bucket` | `4-6` | 1 | 0 | 1 | 28 | 0.62% | 141,865 | baseline |
| `solution_order_rank_bucket` | `10+` | 9 | 4 | 5 | 54 | 0.37% | -15,475 | baseline |

The root-child dominance feature is not useful in this corpus because all 14
cases fall into the same `low` dominance bucket. The solution-rank signal still
matches pass56, but remains too small and weak to justify a policy.

## Decision

Do not modify solver behavior.

The next useful step is to run feature mining over a larger discovery corpus,
not just the 14 replay cases. The pass50 all-profile discovery output has more
conservative-root rows and should be the next input for feature density mining.
