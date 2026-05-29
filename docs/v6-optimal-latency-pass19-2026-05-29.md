# V6 optimal latency pass 19 - exact goal table probes

Date: 2026-05-29

Status: rejected, no solver source change.

## Goal

Re-check the existing exact goal table experiment on the current V6
conservative-root tail cases. The table is exact and preserves optimality, but
it adds hash lookups near the end of DFS branches and extra memory pressure.

## Probe setup

The probes reused the warm V6 table cache and tested goal table radii 3, 4, and
5 on the same conservative-root cases used in pass 18.

```bash
RUBIK_EXPERIMENTAL_OPTIMAL_GOAL_TABLE_DEPTH=3 \
  scripts/run_v6_tail_baseline.sh \
  --output-dir out/release-native-lto/benchmark-results/v6-pass19-probe-goal-depth3 \
  --cache-mode reuse \
  --tail-seeds 99 \
  --hardening-seeds 42,99 \
  --deep-opt14-count 1 \
  --deep-opt15-count 1

RUBIK_EXPERIMENTAL_OPTIMAL_GOAL_TABLE_DEPTH=4 \
  scripts/run_v6_tail_baseline.sh \
  --output-dir out/release-native-lto/benchmark-results/v6-pass19-probe-goal-depth4 \
  --cache-mode reuse \
  --tail-seeds 99 \
  --hardening-seeds 42,99 \
  --deep-opt14-count 1 \
  --deep-opt15-count 1

RUBIK_EXPERIMENTAL_OPTIMAL_GOAL_TABLE_DEPTH=5 \
  scripts/run_v6_tail_baseline.sh \
  --output-dir out/release-native-lto/benchmark-results/v6-pass19-probe-goal-depth5 \
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

Goal table radius 3:

| Case | Solver ms | Nodes | Decision |
| --- | ---: | ---: | --- |
| hardening seed 42 depth 14 | 256 | 992614 | neutral |
| hardening seed 42 depth 15 | 2291 | 11132722 | slower |
| hardening seed 99 depth 14 | 355 | 1690778 | neutral |
| hardening seed 99 depth 15 | 1150 | 3794183 | neutral |
| tail seed 99 depth 15 | 1150 | 3788954 | neutral |

Goal table radius 4:

| Case | Solver ms | Nodes | Decision |
| --- | ---: | ---: | --- |
| hardening seed 42 depth 14 | 262 | 971826 | neutral |
| hardening seed 42 depth 15 | 2295 | 11101526 | slower |
| hardening seed 99 depth 14 | 361 | 1697509 | slower |
| hardening seed 99 depth 15 | 1151 | 3779575 | neutral |
| tail seed 99 depth 15 | 1160 | 3796354 | slower |

Goal table radius 5:

| Case | Solver ms | Nodes | Decision |
| --- | ---: | ---: | --- |
| hardening seed 42 depth 14 | 372 | 974742 | slower |
| hardening seed 42 depth 15 | 2492 | 11312413 | slower |
| hardening seed 99 depth 14 | 475 | 1695389 | slower |
| hardening seed 99 depth 15 | 1305 | 3873680 | slower |
| tail seed 99 depth 15 | 1290 | 3838428 | slower |

## Decision

Do not enable an exact goal table in the default V6 optimal path.

Small radii are mostly neutral and do not improve the slowest target case. The
larger local radius tested here adds visible overhead and regresses every target
case. The accepted implementation remains pass 12.

## Follow-up

If this idea is revisited, it should be redesigned as a cheaper specialized
late-depth lookup instead of promoting the current `unordered_map` goal table.
