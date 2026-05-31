# V6 optimal latency pass 62

## Goal

Join the pass61 `high_bound_first` comparison with the pass50 conservative-root
feature data, then isolate whether the large pass61 regression has a usable
gate.

This pass adds repeatable analysis tooling. It does not change solver behavior.

## Command

```bash
scripts/analyze_v6_ordering_candidate_features.py \
  --comparison out/release-native-lto/benchmark-results/v6-conservative-root-all-profiles-high-bound-sweep-pass61/high_bound_first/comparison.csv \
  --features out/release-native-lto/benchmark-results/v6-conservative-root-all-profiles-pass50/discovery_case_features.csv \
  --case-output out/release-native-lto/benchmark-results/v6-conservative-root-all-profiles-high-bound-sweep-pass61/high_bound_first/case_features.csv \
  --feature-output out/release-native-lto/benchmark-results/v6-conservative-root-all-profiles-high-bound-sweep-pass61/high_bound_first/feature_summary.csv
```

## Result

The full pass61 `high_bound_first` candidate remains:

| Cases | Wins | Losses | Baseline ms | Candidate ms | Delta ms | Delta |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 29 | 8 | 20 | 48,604 | 44,388 | -4,216 | -8.67% |

Feature groups with the strongest aggregate wins:

| Feature | Value | Cases | Wins | Losses | Delta ms | Delta | Worst case delta |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `profile` | `8:14:1` | 1 | 1 | 0 | -3,444 | -36.01% | -3,444 |
| `bucket` | `lb8_s13-16_fd1` | 4 | 1 | 3 | -3,439 | -34.04% | 4 |
| `solution_rank_bucket` | `10+` | 14 | 6 | 8 | -3,375 | -10.85% | 2,966 |
| `bucket` | `lb9_s0-4_fd0` | 1 | 1 | 0 | -2,744 | -45.21% | -2,744 |

The large regression is isolated to one observed bucket/profile:

| Case | Profile | Bucket | Baseline ms | Candidate ms | Delta ms | Delta | Node delta |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: |
| `hardening:depth15:seed424242:random_424242_1` | `9:14:0` | `lb9_s13-16_fd0` | 1,811 | 4,777 | 2,966 | 163.78% | 14,403,544 |

If the single observed `lb9_s13-16_fd0` case is excluded, the remaining 28 cases
would move from 46,793 ms to 39,611 ms, a -7,182 ms / -15.35% aggregate delta.
This is only a diagnostic calculation. It is not enough evidence to ship a
policy gate because that bucket currently has one measured case in this sweep.

## Decision

Do not promote `high_bound_first` yet.

The useful next gate hypothesis is:

```text
try high_bound_first where the conservative-root bucket is not lb9_s13-16_fd0
```

Before implementing that in the solver, collect a focused corpus for
`lb9_s13-16_fd0` and neighboring buckets. If the regression bucket remains
consistently negative while the positive buckets remain positive, then the gate
becomes a release candidate.

The new analyzer should be kept in the V6 workflow. It makes future ordering
sweeps auditable by showing aggregate wins, regressions, best case, and worst
case for each feature group.
