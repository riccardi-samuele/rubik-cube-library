# V6 optimal latency pass 29

## Goal

Make the existing root-search profile more actionable before changing solver
policy again. Pass 28 showed that the next measurement target is the
`lb8_stable_mid_strong_min` bucket, but the summary did not show whether slow
cases spend most work before the solution root, inside the solution root, or in
one large root.

## Change

`scripts/analyze_root_search_profile.py --summary` now emits these additional
per-case fields:

- `before_solution_root_nodes`
- `before_solution_root_elapsed_ms`
- `before_solution_nodes_share_ppm`
- `before_solution_elapsed_share_ppm`
- `solution_root_nodes`
- `solution_root_nodes_share_ppm`
- `max_root_nodes`
- `max_root_nodes_share_ppm`

This is tooling only. It does not change solver behavior, move ordering,
optimality, or public API output.

## Validation

Commands:

```bash
cmake --preset release-native-lto
ctest --test-dir out/release-native-lto -R '^analyze_root_search_profile' --output-on-failure
scripts/analyze_root_search_profile.py \
  --input-dir out/release-native-lto/benchmark-results/v6-tail-pass20 \
  --summary \
  --output out/release-native-lto/benchmark-results/v6-pass29-root-search-summary.csv
```

Result:

- `analyze_root_search_profile`: passed
- `analyze_root_search_profile_rejects_missing_values`: passed
- `analyze_root_search_profile_summary`: passed

## Pass 20 findings with the new summary

Slowest rows:

| Case | Solver ms | Reason | Before solution elapsed share | Solution root node share | Max root node share |
| --- | ---: | --- | ---: | ---: | ---: |
| hardening depth 15 seed 987654321 | 3593 | `lb8_stable_mid_strong_min` | 96.4% | 3.6% | 9.8% |
| tail depth 15 seed 987654321 | 3545 | `lb8_stable_mid_strong_min` | 96.7% | 3.3% | 9.8% |
| hardening depth 15 seed 42 | 2296 | `conservative_root` | 56.2% | 6.3% | 6.5% |
| tail depth 15 seed 555 | 1829 | `lb9_low_strong_min` | 99.7% | 0.3% | 18.7% |
| hardening depth 15 seed 424242 | 1827 | `conservative_root` | 81.2% | 6.2% | 6.4% |

Grouped by adaptive reason:

| Reason | Cases | Total solver ms | Max solver ms | Before solution node share | Before solution elapsed share |
| --- | ---: | ---: | ---: | ---: | ---: |
| `conservative_root` | 10 | 10371 | 2296 | 72.5% | 72.6% |
| `lb8_stable_mid_strong_min` | 4 | 7468 | 3593 | 96.3% | 96.3% |
| `lb9_mid_strong_min` | 9 | 4357 | 1784 | 84.1% | 83.7% |
| `lb9_low_strong_min` | 5 | 3320 | 1829 | 95.0% | 95.1% |
| `depth14_conservative_root` | 13 | 1967 | 349 | 93.2% | 93.4% |
| `remaining_depth_lt_5` | 2 | 554 | 452 | 90.2% | 90.6% |

## Interpretation

The worst `lb8_stable_mid_strong_min` cases are dominated by roots searched
before the solution root. The solution root itself is small: around 3-4% of
nodes, while the largest individual root is about 10% of nodes.

The next solver experiment should therefore focus on reducing pre-solution root
work for this bucket. Candidate directions include a more selective root
ordering signal for `lb8_stable_mid_strong_min`, or a conservative split/early
ordering strategy that is validated against the full Pass 20 corpus before being
accepted.
