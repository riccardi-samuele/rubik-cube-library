# V6 Optimal Latency Pass 28

Date: 2026-05-29

## Goal

Make V6 comparison output explain regressions by scheduler bucket. Single-case
sorting shows the worst cases, but optimizer decisions are usually made by
`adaptive_reason`, so the comparison tool should aggregate by that field.

## Change

`scripts/compare_v6_latency.py` now accepts `--group-by-reason`.

When enabled, case rows are aggregated into `__reason__:<adaptive_reason>` rows.
The `__summary__` row stays last, and `--sort-by` / `--sort-desc` still work on
the grouped rows.

## Validation

The fixture test verifies that the `conservative_root` bucket aggregates two
cases with:

- total solver elapsed: `1,300 ms -> 1,100 ms`;
- total delta: `-200 ms`;
- total nodes: `13,000 -> 12,000`.

A real Pass20 vs Pass23 comparison grouped by reason reports the largest
regression in:

| Reason | Cases | Pass 20 | Pass 23 | Delta | Max delta |
| --- | ---: | ---: | ---: | ---: | ---: |
| `lb8_stable_mid_strong_min` | 4 | 7,468 ms | 7,545 ms | +77 ms | +26 ms |
| `lb9_low_strong_min` | 5 | 3,320 ms | 3,340 ms | +20 ms | +13 ms |
| `remaining_depth_lt_5` | 2 | 554 ms | 563 ms | +9 ms | +8 ms |

Command:

```bash
scripts/compare_v6_latency.py \
  --baseline-dir out/release-native-lto/benchmark-results/v6-tail-pass20 \
  --candidate-dir out/release-native-lto/benchmark-results/v6-tail-pass23 \
  --group-by-reason \
  --sort-by elapsed_delta_ms \
  --sort-desc \
  --output out/release-native-lto/benchmark-results/v6-pass28-compare-pass20-pass23-reason-groups.csv
```

## Decision

Accepted as tooling. This does not change solver behavior. It identifies the
next measurement target more directly: remaining V6 solver work should focus on
the `lb8_stable_mid_strong_min` tail bucket before attempting broader scheduler
changes.
