# V6 Optimal Latency Pass 24

Date: 2026-05-29

## Goal

Add a replayable comparison tool for V6 benchmark directories. Recent passes used one-off shell and Python snippets to compare full-corpus runs; that made it too easy to miss key collisions or summary mismatches.

## Change

Added `scripts/compare_v6_latency.py`.

The tool compares two V6 benchmark output directories and emits one CSV row per common case plus a `__summary__` row with:

- common case count;
- total solver elapsed deltas;
- total node deltas;
- max benchmark wall-time deltas;
- per-case winner;
- root ordering and adaptive scheduler reason before and after.

It accepts:

```bash
scripts/compare_v6_latency.py \
  --baseline-dir out/release-native-lto/benchmark-results/v6-tail-pass20 \
  --candidate-dir out/release-native-lto/benchmark-results/v6-tail-pass23 \
  --output out/release-native-lto/benchmark-results/v6-pass24-compare-pass20-pass23.csv
```

## Validation

The fixture covers:

- multiple cases in the same benchmark CSV;
- stable case identity from suite, depth, seed, and case name;
- benchmark wall time reported after case rows;
- missing-value CLI validation.

The real Pass20 vs Pass23 comparison now reproduces the accepted aggregate summary:

| Metric | Pass 20 | Pass 23 | Delta |
| --- | ---: | ---: | ---: |
| Common cases | 43 | 43 | 0 |
| Total solver elapsed | 28,037 ms | 28,066 ms | +29 ms |
| Total nodes | 165,163,047 | 165,085,613 | -77,434 |
| Max benchmark wall time | 4,237 ms | 4,268 ms | +31 ms |

## Decision

Accepted as tooling. This does not change solver behavior, but it makes future V6 latency passes safer because full-corpus acceptance decisions can use a checked comparison command instead of ad hoc aggregation.
