# V6 optimal latency pass 18 - conservative root ordering probes

Date: 2026-05-29

Status: rejected, no solver source change.

## Goal

Check whether the large `root:conservative_root` group can be improved by changing
root tie ordering for the `lb=8`, `first_diff=1` subset before promoting any new
adaptive policy.

The accepted baseline remains pass 12:

- full V6 corpus: 43/43 optimal
- total solver elapsed: 28861 ms
- total nodes: 169155502
- p50/p90/p95/p99/max solver: 307/1742/1835/3645/3749 ms
- max wall elapsed: 4506 ms

## Probe setup

Both probes reused the warm V6 table cache and targeted the highest-signal
conservative-root cases from pass 12:

```bash
RUBIK_EXPERIMENTAL_ROOT_ORDERING=reverse_tie \
  scripts/run_v6_tail_baseline.sh \
  --output-dir out/release-native-lto/benchmark-results/v6-pass18-probe-reverse-root \
  --cache-mode reuse \
  --tail-seeds 99 \
  --hardening-seeds 42,99 \
  --deep-opt14-count 1 \
  --deep-opt15-count 1

RUBIK_EXPERIMENTAL_ROOT_ORDERING=phase2_tiebreak \
  scripts/run_v6_tail_baseline.sh \
  --output-dir out/release-native-lto/benchmark-results/v6-pass18-probe-phase2-root \
  --cache-mode reuse \
  --tail-seeds 99 \
  --hardening-seeds 42,99 \
  --deep-opt14-count 1 \
  --deep-opt15-count 1
```

## Results

Pass 12 reference values for the same cases:

| Case | Baseline solver ms | Baseline nodes |
| --- | ---: | ---: |
| hardening seed 42 depth 14 | 261 | 999030 |
| hardening seed 42 depth 15 | 2285 | 10885856 |
| hardening seed 99 depth 14 | 355 | 1682426 |
| hardening seed 99 depth 15 | 1165 | 3805653 |
| tail seed 99 depth 15 | 1149 | 3783367 |

`reverse_tie` probe:

| Case | Solver ms | Nodes | Decision |
| --- | ---: | ---: | --- |
| hardening seed 42 depth 14 | 260 | 983251 | neutral |
| hardening seed 42 depth 15 | 2341 | 11029751 | slower |
| hardening seed 99 depth 14 | 358 | 1689959 | slower |
| hardening seed 99 depth 15 | 1287 | 3829883 | slower |
| tail seed 99 depth 15 | 1299 | 3850997 | slower |

`phase2_tiebreak` probe:

| Case | Solver ms | Nodes | Decision |
| --- | ---: | ---: | --- |
| hardening seed 42 depth 14 | 260 | 985298 | neutral |
| hardening seed 42 depth 15 | 2352 | 11327158 | slower |
| hardening seed 99 depth 14 | 358 | 1705492 | slower |
| hardening seed 99 depth 15 | 1163 | 3834560 | neutral |
| tail seed 99 depth 15 | 1179 | 3864027 | slower |

## Decision

Do not promote either ordering mode into the adaptive scheduler.

The probes show no stable latency win for the conservative-root subset. The
largest target case, hardening seed 42 depth 15, regresses under both variants,
and `reverse_tie` also regresses the repeated seed 99 depth-15 cases. The
existing pass 12 policy remains the accepted implementation.

## Follow-up

Further V6 latency work should avoid generic root tie-order changes for this
subset and instead investigate pruning, root-level early cutoffs, or a policy
with stronger evidence from full-corpus measurements.
