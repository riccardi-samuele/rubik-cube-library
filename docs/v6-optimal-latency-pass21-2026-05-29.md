# V6 Optimal Latency Pass 21

Date: 2026-05-29

## Goal

Evaluate whether depth-14 conservative roots with `initialLowerBound == 9` and `strongMinCount == 3` should stay on root-level scheduling instead of using the Pass 20 deep split policy.

## Result

Rejected. The probe slightly reduced total elapsed time, but it worsened the tail latency metrics that Pass 20 improved.

## Measurements

Command:

```bash
scripts/run_v6_tail_baseline.sh --output-dir out/release-native-lto/benchmark-results/v6-tail-pass21 --cache-mode reuse
```

Pass 21 corpus summary:

| Metric | Pass 21 |
| --- | ---: |
| Cases | 43 |
| Optimal solves | 43 |
| Failed solves | 0 |
| Total solver elapsed | 28,004 ms |
| Total nodes | 165,129,891 |
| p50 | 254 ms |
| p90 | 1,769 ms |
| p95 | 1,868 ms |
| p99 | 3,569 ms |
| Max case | 3,621 ms |
| Max benchmark wall time | 4,258 ms |

Comparison with accepted Pass 20:

| Metric | Pass 20 | Pass 21 | Delta |
| --- | ---: | ---: | ---: |
| Total solver elapsed | 28,037 ms | 28,004 ms | -33 ms |
| Total nodes | 165,163,047 | 165,129,891 | -33,156 |
| p50 | 252 ms | 254 ms | +2 ms |
| p90 | 1,784 ms | 1,769 ms | -15 ms |
| p95 | 1,829 ms | 1,868 ms | +39 ms |
| p99 | 3,545 ms | 3,569 ms | +24 ms |
| Max case | 3,593 ms | 3,621 ms | +28 ms |
| Max benchmark wall time | 4,237 ms | 4,258 ms | +21 ms |

## Decision

Keep the Pass 20 policy unchanged. Pass 21 recovers one depth-14 regression, but it also worsens the upper tail. For V6, the accepted policy remains the lower-risk choice because the optimization target is local optimal-solver latency, especially the slowest cases.
