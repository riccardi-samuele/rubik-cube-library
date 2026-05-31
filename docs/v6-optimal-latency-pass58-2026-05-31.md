# V6 optimal latency pass 58

## Goal

Run conservative-root feature mining over the larger pass50 discovery corpus.

Pass57 mined only the 14 replay cases from pass52 through pass54. This pass adds
a discovery-only mode so the same feature extraction can be applied to
`targeted_cases.csv` without requiring replay comparison data.

## Tooling Update

`scripts/analyze_v6_conservative_root_features.py` now supports:

```text
--discovery-only
```

In discovery-only mode the script reads `targeted_cases.csv`, extracts the same
profile features, and writes feature-density summaries with:

- cases
- total discovery elapsed milliseconds
- total discovery nodes

It does not emit candidate/baseline deltas because no replay comparison exists
in discovery-only inputs.

## Command

```bash
scripts/analyze_v6_conservative_root_features.py \
  --run-dir out/release-native-lto/benchmark-results/v6-conservative-root-all-profiles-pass50 \
  --discovery-only \
  --case-output out/release-native-lto/benchmark-results/v6-conservative-root-all-profiles-pass50/discovery_case_features.csv \
  --feature-output out/release-native-lto/benchmark-results/v6-conservative-root-all-profiles-pass50/discovery_feature_summary.csv
```

## Discovery Result

The pass50 input contains 29 conservative-root discovery rows.

Most useful density groups:

| Feature | Value | Cases | Discovery elapsed ms | Discovery nodes |
| --- | --- | ---: | ---: | ---: |
| `solution_rank_bucket` | `10+` | 14 | 30,935 | 156,549,810 |
| `solution_matches_base_first` | `0` | 23 | 42,678 | 227,057,116 |
| `solution_matches_strong_first` | `0` | 28 | 45,947 | 246,610,337 |
| `solution_order_rank_bucket` | `1-3` | 7 | 4,875 | 29,638,257 |
| `solution_order_rank_bucket` | `4-6` | 3 | 8,578 | 51,749,044 |
| `solution_order_rank_bucket` | `7-9` | 5 | 3,486 | 20,323,531 |
| `solution_order_rank_bucket` | `10+` | 14 | 30,935 | 156,549,810 |

Root-search dominance did not separate this corpus:

| Feature | Value | Cases | Discovery elapsed ms | Discovery nodes |
| --- | --- | ---: | ---: | ---: |
| `dominant_child_share_bucket` | `low` | 29 | 47,874 | 258,260,642 |
| `solution_root_status` | `found` | 29 | 47,874 | 258,260,642 |

## Decision

Do not use root-child dominance as the next optimization lever. In this corpus
all conservative-root cases fall into the same low-dominance bucket.

The strongest density signal is late solution rank:

- `solution_rank_bucket=10+`
- 14 cases
- 30,935 ms discovery time
- 156,549,810 discovery nodes

This is a better target than exact adaptive profiles. The next step should build
a targeted corpus filter for feature values, starting with
`solution_rank_bucket=10+`, then replay a candidate designed for late-solution
cases.
