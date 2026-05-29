# V6 Optimal Latency Pass 27

Date: 2026-05-29

## Goal

Make V6 benchmark comparisons faster to inspect. Candidate runs often have small
total deltas but important tail regressions on a few cases, so the comparison
tool needs direct sorting by numeric fields.

## Change

`scripts/compare_v6_latency.py` now accepts:

- `--sort-by FIELD` to sort case rows by a numeric output field;
- `--sort-desc` to put the largest values first.

The `__summary__` row always remains at the end of the CSV, so automation can
still read aggregate metrics from the final row.

## Validation

The fixture test covers ascending and descending sorting by `elapsed_delta_ms`.
A real Pass20 vs Pass23 comparison sorted by descending elapsed delta starts with
these regressions:

| Case | Pass 20 | Pass 23 | Delta |
| --- | ---: | ---: | ---: |
| `tail:depth15:seed987654321:random_987654321_1` | 3,545 ms | 3,594 ms | +49 ms |
| `hardening:depth15:seed42:random_42_1` | 2,296 ms | 2,341 ms | +45 ms |
| `hardening:depth15:seed987654321:random_987654321_1` | 3,593 ms | 3,619 ms | +26 ms |

Command:

```bash
scripts/compare_v6_latency.py \
  --baseline-dir out/release-native-lto/benchmark-results/v6-tail-pass20 \
  --candidate-dir out/release-native-lto/benchmark-results/v6-tail-pass23 \
  --sort-by elapsed_delta_ms \
  --sort-desc \
  --output out/release-native-lto/benchmark-results/v6-pass27-compare-pass20-pass23-regressions.csv
```

## Decision

Accepted as tooling. This does not change solver behavior. It makes later V6
optimization attempts easier to reject or investigate when a small set of tail
cases causes p99 or max solver regressions.
