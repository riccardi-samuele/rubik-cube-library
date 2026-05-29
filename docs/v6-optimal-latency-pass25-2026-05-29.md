# V6 Optimal Latency Pass 25

Date: 2026-05-29

## Goal

Extend the V6 comparison tooling with percentile deltas. V6 acceptance decisions depend on tail latency, so the comparison summary needs more than total solver time and max wall time.

## Change

`scripts/compare_v6_latency.py` now adds these summary-only fields:

- `baseline_p50_ms`, `candidate_p50_ms`, `p50_delta_ms`;
- `baseline_p90_ms`, `candidate_p90_ms`, `p90_delta_ms`;
- `baseline_p95_ms`, `candidate_p95_ms`, `p95_delta_ms`.

The percentile index matches the benchmark aggregation convention already used in the V6 pass reports.

## Validation

The fixture test now checks percentile columns in the `__summary__` row. A real Pass20 vs Pass23 comparison reports:

| Metric | Pass 20 | Pass 23 | Delta |
| --- | ---: | ---: | ---: |
| Total solver elapsed | 28,037 ms | 28,066 ms | +29 ms |
| p50 | 252 ms | 254 ms | +2 ms |
| p90 | 1,784 ms | 1,772 ms | -12 ms |
| p95 | 1,829 ms | 1,842 ms | +13 ms |
| Total nodes | 165,163,047 | 165,085,613 | -77,434 |
| Max benchmark wall time | 4,237 ms | 4,268 ms | +31 ms |

Command:

```bash
scripts/compare_v6_latency.py \
  --baseline-dir out/release-native-lto/benchmark-results/v6-tail-pass20 \
  --candidate-dir out/release-native-lto/benchmark-results/v6-tail-pass23 \
  --output out/release-native-lto/benchmark-results/v6-pass25-compare-pass20-pass23-percentiles.csv
```

## Decision

Accepted as tooling. This does not change solver behavior. It makes later V6 optimization trials easier to accept or reject using the same tail-latency criteria used in the written pass reports.
