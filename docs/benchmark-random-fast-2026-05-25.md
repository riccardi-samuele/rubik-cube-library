# Random Fast Benchmark - 2026-05-25

This report records the first larger random benchmark for `SolveMode::Fast` and
the result after adaptive fast-mode tuning.

## Command

```sh
RUBIK_TABLE_CACHE_DIR=/tmp/rubik_cube_library_phase2_cache \
./build/rubik-bench \
  --mode fast \
  --timeout-ms 5000 \
  --max-depth 24 \
  --case-set random \
  --random-count 100 \
  --random-depth 20 \
  --random-seed 12345 \
  --slowest-count 10
```

## Environment

- Date: 2026-05-25
- Cache mode: warm cache
- Cache directory: `/tmp/rubik_cube_library_phase2_cache`
- Metric: HTM
- Profile: default
- Timeout: 5000 ms per case
- Max depth: 24

## Summary

Current adaptive fast-mode result:

```text
summary,total_cases,100
summary,solved,100
summary,failed,0
summary,total_elapsed_ms,49268
summary,total_nodes_expanded,21695139
summary,average_elapsed_ms,492.68
summary,average_nodes_expanded,216951.39
summary,max_elapsed_ms,2374
```

Derived values:

- solved rate: 100%
- average elapsed time: 493 ms per case
- average expanded nodes: 216951 nodes per case
- slowest case: 2374 ms

Previous adaptive result before robust phase-2 timeout tuning:

```text
summary,total_cases,100
summary,solved,100
summary,failed,0
summary,total_elapsed_ms,56201
summary,total_nodes_expanded,26076393
summary,average_elapsed_ms,562.01
summary,average_nodes_expanded,260763.93
summary,max_elapsed_ms,3257
```

Previous adaptive result before phase-1 candidate scoring:

```text
summary,total_cases,100
summary,solved,100
summary,failed,0
summary,total_elapsed_ms,58073
summary,total_nodes_expanded,26282351
summary,average_elapsed_ms,580.73
summary,average_nodes_expanded,262823.51
summary,max_elapsed_ms,3274
```

Previous adaptive result before quick-budget tuning:

```text
summary,total_cases,100
summary,solved,100
summary,total_elapsed_ms,62085
summary,total_nodes_expanded,25061388
```

Previous pre-adaptive result:

```text
summary,total_cases,100
summary,solved,100
summary,total_elapsed_ms,176305
summary,total_nodes_expanded,83320120
```

Previous derived values:

- solved rate: 100%
- average elapsed time: 1763 ms per case
- average expanded nodes: 833201 nodes per case

## Notes

All 100 random depth-20 scrambles generated from seed `12345` were solved within
the 5 second per-case timeout before and after adaptive tuning.

Adaptive tuning adds a small low-latency two-phase attempt before the larger
default attempt. Later tuning shortened quick-attempt budgets and sorted phase-1
candidates by an estimated phase-2 lower bound. The current result also reduces
the default-profile robust phase-2 per-candidate timeout to 150 ms. This reduces
total elapsed time by about 72% compared with the original pre-adaptive baseline
while preserving the 100% solve rate.

Slowest cases from the current run:

```text
slowest,1,random_12345_5,20,"B R2 B2 U2 B' U2 D' L' U D R2 D2 U' D' F B' D' B R B'",Found,22,2374,1063962,"U2 F R F' D B' R' L2 D' F' U L2 F2 R2 U' B2 U' R2 B2 U R2 U2"
slowest,2,random_12345_50,20,"F2 D' R' D' F' D' F2 L' B' U2 B2 R' U2 D' B' U2 R2 B' F2 D2",Found,23,2274,1021879,"B R D' F B2 R D' L F' R2 U B2 D' R2 B2 D' F2 U F2 L2 F2 U' B2"
slowest,3,random_12345_33,20,"D F' D' U D2 R F2 L U2 B' L F' L U' F L2 D2 L' U L2",Found,23,2231,997085,"D B2 L2 B' R2 D2 F2 D L' F' U2 B2 U F2 D' L2 D2 R2 F2 R2 L2 D R2"
slowest,4,random_12345_92,20,"U2 R' F L' R2 L' D2 B2 D2 U R2 U' B' L B L2 D R2 L2 U",Found,21,2050,921784,"D B R D L' F2 B R2 B U' R2 D' F2 B2 D2 L2 B2 D B2 U B2"
slowest,5,random_12345_47,20,"U F' L2 D2 R2 L2 D' L' D' F L2 B' L2 R2 L2 R' D2 B R' F",Found,22,2048,922867,"U R' U' B2 U' F' R' D2 F' R' L2 U D2 L2 F2 L2 U F2 L2 U' F2 B2"
slowest,6,random_12345_75,20,"L' R D L D2 B' U' B F2 L2 B F L2 D2 F' U' D2 L' U2 F'",Found,23,2028,926213,"U' L' U' D2 R F2 U' F B' D2 F2 U L2 U2 F2 L2 F2 R2 D R2 F2 D2 F2"
slowest,7,random_12345_10,20,"L' R2 D R F R B2 L U' R' L' D2 L D2 U2 F U2 L D2 U'",Found,23,2024,906238,"B' U B R B2 U' B2 U' L' U2 R2 U' L2 D' R2 U2 R2 F2 U R2 U2 L2 D'"
slowest,8,random_12345_34,20,"F' U R' F2 B' L D' B L' B' L U2 D F2 U R2 D2 F' B' L",Found,21,1993,888519,"L' B' U' B2 U F2 R L2 B L2 U' R2 D' B2 D F2 R2 D R2 B2 U"
slowest,9,random_12345_13,20,"B' R U R F2 B R' F' L R B2 L2 R' U2 D B2 U' F2 D B",Found,23,1984,879403,"F' B R' D' B2 U2 R2 F2 B' L U2 F2 D2 B2 L2 D B2 U F2 D' B2 D' R2"
slowest,10,random_12345_45,20,"D' F2 L' D' B U2 R' D B' U' B' U R2 D U2 D F2 L B2 U2",Found,20,1977,877874,"F2 U L U L B2 D2 B' L' D2 L2 U L2 D R2 U' R2 L2 U2 F2"
```

This is a local development-machine result, not a Raspberry Pi result. The next
useful comparison is the same command on Raspberry Pi 4 with the same cache
setup and source snapshot.
