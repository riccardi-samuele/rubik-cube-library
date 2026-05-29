# V6 Optimal Latency Pass 26

Date: 2026-05-29

## Goal

Extend the V6 comparison tooling so candidate decisions can inspect the full tail,
not only p50/p90/p95. V6 optimization trials are accepted or rejected primarily on
p95/p99/max solver latency and total corpus cost.

## Change

`scripts/compare_v6_latency.py` now adds these summary-only fields:

- `baseline_p99_ms`, `candidate_p99_ms`, `p99_delta_ms`;
- `baseline_max_elapsed_ms`, `candidate_max_elapsed_ms`,
  `max_elapsed_delta_ms`.

The percentile calculation uses the same index convention as the existing V6
pass reports and the p50/p90/p95 fields added in Pass 25.

## Validation

The fixture test now requires the new p99 and max solver fields in the
`__summary__` row. A real Pass20 vs Pass23 comparison reports:

| Metric | Pass 20 | Pass 23 | Delta |
| --- | ---: | ---: | ---: |
| Total solver elapsed | 28,037 ms | 28,066 ms | +29 ms |
| p50 | 252 ms | 254 ms | +2 ms |
| p90 | 1,784 ms | 1,772 ms | -12 ms |
| p95 | 1,829 ms | 1,842 ms | +13 ms |
| p99 | 3,545 ms | 3,594 ms | +49 ms |
| Max solver elapsed | 3,593 ms | 3,619 ms | +26 ms |
| Total nodes | 165,163,047 | 165,085,613 | -77,434 |
| Max benchmark wall time | 4,237 ms | 4,268 ms | +31 ms |

Command:

```bash
scripts/compare_v6_latency.py \
  --baseline-dir out/release-native-lto/benchmark-results/v6-tail-pass20 \
  --candidate-dir out/release-native-lto/benchmark-results/v6-tail-pass23 \
  --output out/release-native-lto/benchmark-results/v6-pass26-compare-pass20-pass23-tail.csv
```

## Decision

Accepted as tooling. This does not change solver behavior. It closes a decision
gap in the comparison script: future V6 candidates can be rejected directly from
the comparison CSV when p99 or max solver latency regresses.
