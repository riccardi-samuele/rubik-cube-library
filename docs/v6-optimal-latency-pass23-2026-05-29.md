# V6 Optimal Latency Pass 23

Date: 2026-05-29

## Goal

Check whether the `conservative_root` cluster with `initialLowerBound == 9`, `maxDepth == 15`, `strongMinCount == 14`, and `firstMoveDiffers == false` should use `phase2_tiebreak` root ordering.

The target cluster is represented by the repeated seed `424242` depth-15 rows, which were among the slowest remaining Pass 20 cases.

## Probe

The targeted probe reused the warm V6 table cache:

```bash
RUBIK_EXPERIMENTAL_ROOT_ORDERING=phase2_tiebreak \
  scripts/run_v6_tail_baseline.sh \
  --output-dir out/release-native-lto/benchmark-results/v6-pass23-probe-lb9-strong14-phase2-root \
  --cache-mode reuse \
  --tail-seeds 424242 \
  --hardening-seeds 424242 \
  --deep-opt14-count 1 \
  --deep-opt15-count 1
```

The targeted depth-15 rows improved:

| Case | Pass 20 | Probe | Delta |
| --- | ---: | ---: | ---: |
| tail seed `424242`, depth 15 | 1,777 ms | 1,767 ms | -10 ms |
| hardening seed `424242`, depth 15 | 1,827 ms | 1,754 ms | -73 ms |

## Full Corpus Trial

A temporary adaptive policy promoted only that exact profile to `phase2_tiebreak`, then ran the full V6 corpus:

```bash
scripts/run_v6_tail_baseline.sh \
  --output-dir out/release-native-lto/benchmark-results/v6-tail-pass23 \
  --cache-mode reuse
```

Full corpus comparison:

| Metric | Pass 20 | Pass 23 trial | Delta |
| --- | ---: | ---: | ---: |
| Cases | 43 | 43 | 0 |
| Optimal solves | 43 | 43 | 0 |
| Total solver elapsed | 28,037 ms | 28,066 ms | +29 ms |
| Total nodes | 165,163,047 | 165,085,613 | -77,434 |
| p50 | 252 ms | 254 ms | +2 ms |
| p90 | 1,784 ms | 1,772 ms | -12 ms |
| p95 | 1,829 ms | 1,842 ms | +13 ms |
| p99 | 3,545 ms | 3,594 ms | +49 ms |
| Max case | 3,593 ms | 3,619 ms | +26 ms |
| Max benchmark wall time | 4,237 ms | 4,268 ms | +31 ms |

## Decision

Rejected. The targeted `424242` rows improved, but the full corpus regressed total solver time, p50, p95, p99, max case latency, and max wall latency. V6 keeps the Pass 20 root-ordering policy unchanged.

The result suggests that the `lb=9`, `strongMinCount=14` profile is not broad enough to promote safely from a single repeated seed. Future work should either find a stronger discriminator or target a different latency source.
