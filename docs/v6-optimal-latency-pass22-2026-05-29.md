# V6 Optimal Latency Pass 22

Date: 2026-05-29

## Goal

Check whether the remaining slow `lb8_stable_mid_strong_min` depth-15 cases benefit from an alternate root ordering while keeping the accepted Pass 20 deep-split policy.

The target cases were the two slowest Pass 20 rows:

- tail seed `987654321`, depth 15;
- hardening seed `987654321`, depth 15.

## Probe

Both probes reused the warm V6 table cache and ran only the targeted seed plus the required hardening depth-14 companion case from the runner.

```bash
RUBIK_EXPERIMENTAL_ROOT_ORDERING=reverse_tie \
  scripts/run_v6_tail_baseline.sh \
  --output-dir out/release-native-lto/benchmark-results/v6-pass22-probe-lb8-reverse-root \
  --cache-mode reuse \
  --tail-seeds 987654321 \
  --hardening-seeds 987654321 \
  --deep-opt14-count 1 \
  --deep-opt15-count 1

RUBIK_EXPERIMENTAL_ROOT_ORDERING=phase2_tiebreak \
  scripts/run_v6_tail_baseline.sh \
  --output-dir out/release-native-lto/benchmark-results/v6-pass22-probe-lb8-phase2-root \
  --cache-mode reuse \
  --tail-seeds 987654321 \
  --hardening-seeds 987654321 \
  --deep-opt14-count 1 \
  --deep-opt15-count 1
```

## Results

Comparison against accepted Pass 20 for the targeted depth-15 rows:

| Case | Pass 20 | `reverse_tie` | Delta | `phase2_tiebreak` | Delta |
| --- | ---: | ---: | ---: | ---: | ---: |
| tail seed `987654321`, depth 15 | 3,545 ms | 3,635 ms | +90 ms | 3,594 ms | +49 ms |
| hardening seed `987654321`, depth 15 | 3,593 ms | 3,652 ms | +59 ms | 3,639 ms | +46 ms |

Node counts:

| Case | Pass 20 | `reverse_tie` | Delta | `phase2_tiebreak` | Delta |
| --- | ---: | ---: | ---: | ---: | ---: |
| tail seed `987654321`, depth 15 | 21,406,615 | 21,753,938 | +347,323 | 21,508,999 | +102,384 |
| hardening seed `987654321`, depth 15 | 21,478,100 | 21,781,686 | +303,586 | 21,456,309 | -21,791 |

## Decision

Rejected. Neither alternate root ordering reduced latency for the targeted slow depth-15 cases. `phase2_tiebreak` slightly reduced nodes in one repeated row, but wall-clock solver latency still regressed on both targeted rows.

Keep the Pass 20 scheduler and root-ordering policy unchanged. The next V6 latency work should focus away from root tie ordering for this `lb8_stable_mid_strong_min` cluster.
