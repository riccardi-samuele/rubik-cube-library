# Optimal Deep Probe - 2026-05-25

Host:

- CPU: AMD Ryzen 9 8940HX, 16 cores / 32 threads
- Build preset: `release-native-lto`
- Cache: warm, `RUBIK_TABLE_CACHE_DIR=/tmp/rubik_cube_library_optimal_deep_probe_cache`
- Seed: `12345`
- Suite: `optimal-deep-probe`
- Timeout: `30000 ms` per case

Command:

```sh
cmake --build out/release-native-lto --target rubik-benchmark-optimal-deep-probe
```

This is a non-gated frontier probe. Timeout rows are expected and are preserved
in the summary because the goal is to identify the next optimal-search
bottleneck after the depth-13 gates.

## Summary

| Profile | Benchmark | Cases | Solved | Failed | P50 | P90 | P95 | P99 | Max | Nodes | Payload |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Embedded | depth-14 x2 | 2 | 1 | 1 | 1,322 | 30,000 | 30,000 | 30,000 | 30,000 | 17,165,258 | 22,123,535 |
| Embedded | depth-15 x1 | 1 | 0 | 1 | 30,000 | 30,000 | 30,000 | 30,000 | 30,000 | 17,055,050 | 22,123,535 |
| Default | depth-14 x2 | 2 | 2 | 0 | 588 | 13,457 | 13,457 | 13,457 | 13,457 | 4,375,720 | 205,322,495 |
| Default | depth-15 x1 | 1 | 0 | 1 | 30,000 | 30,000 | 30,000 | 30,000 | 30,000 | 9,076,787 | 205,322,495 |
| Performance | depth-14 x2 | 2 | 2 | 0 | 521 | 14,443 | 14,443 | 14,443 | 14,443 | 4,277,267 | 346,456,895 |
| Performance | depth-15 x1 | 1 | 0 | 1 | 30,000 | 30,000 | 30,000 | 30,000 | 30,000 | 8,232,898 | 346,456,895 |

## Slow Frontier Cases

| Profile | Case | Depth | Status | Elapsed ms | Nodes | Initial LB |
| --- | --- | ---: | --- | ---: | ---: | ---: |
| Embedded | `random_12345_1` | 14 | Timeout | 30,000 | 16,463,412 | 7 |
| Embedded | `random_12345_1` | 15 | Timeout | 30,000 | 17,055,050 | 8 |
| Default | `random_12345_1` | 15 | Timeout | 30,000 | 9,076,787 | 9 |
| Performance | `random_12345_1` | 15 | Timeout | 30,000 | 8,232,898 | 9 |
| Default | `random_12345_1` | 14 | Optimal | 13,457 | 4,238,448 | 9 |
| Performance | `random_12345_1` | 14 | Optimal | 14,443 | 4,143,117 | 9 |

## Interpretation

Depth 13 is now controlled by gates, but this probe shows that the next frontier
is not solved yet. `Embedded/Optimal` still times out on one depth-14 sample at
30 seconds, while `Default` and `Performance` solve the same sample in about
13-15 seconds. The first depth-15 sample times out on every profile.

The diagnostic rows show that the timeout cases still spend tens to hundreds of
millions of candidate-bound checks before expanding 8-17 million nodes. The next
optimization should therefore target stronger admissible pruning or a better
search frontier policy for depth 14/15, not another depth-13 gate adjustment.

## Follow-Up Probes

Two longer single-case probes were run after the first suite:

| Profile | Case | Depth | Timeout | Status | Elapsed ms | Nodes | Notes |
| --- | --- | ---: | ---: | --- | ---: | ---: | --- |
| Embedded | `random_12345_1` | 14 | 120,000 | Optimal | 56,923 | 22,188,844 | Solves above the 30s desktop probe limit. |
| Default | `random_12345_1` | 15 | 120,000 | Timeout | 120,000 | 33,749,992 | Still does not prove optimality. |

An experimental child-ordering run using
`RUBIK_EXPERIMENTAL_STRONG_OPTIMAL_ORDERING=1` was also tested on the embedded
depth-14 frontier case. It still timed out at 30 seconds and expanded more nodes
than the base ordering, so it is not promoted to the default policy.

## Corner-State PDB Experiment

`RUBIK_EXPERIMENTAL_CORNER_STATE_BOUNDS=1` adds a full corner-state pruning
table over corner orientation + corner permutation. The table has
88,179,840 entries. With the current embedded profile this raises the reported
optimal table payload from 22,123,535 bytes to 110,303,375 bytes.

Frontier measurements:

| Profile | Case | Depth | Timeout | Status | Elapsed ms | Nodes | Result |
| --- | --- | ---: | ---: | --- | ---: | ---: | --- |
| Embedded | `random_12345_1` | 14 | 30,000 | Optimal | 9,386 | 3,616,455 | Previously timed out at 30s and solved in 56,923 ms at 120s. |
| Embedded | `random_12345_1` | 15 | 30,000 | Timeout | 30,000 | 12,343,484 | Still not enough for this depth-15 sample. |
| Default | `random_12345_1` | 15 | 120,000 | Optimal | 35,544 | 11,233,650 | Previously timed out at 120s without the table. |

Broader warm-cache sweep with seeds `12345`, `20260525`, and `42`:

| Profile | Benchmark | Cases | Solved | Failed | P50 | P95 | Max | Nodes | Payload |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Embedded | seed 12345 depth-14 x2 | 2 | 2 | 0 | 482 | 9,008 | 9,008 | 3,816,078 | 110,303,375 |
| Embedded | seed 12345 depth-15 x1 | 1 | 0 | 1 | 30,000 | 30,000 | 30,000 | 11,178,980 | 110,303,375 |
| Embedded | seed 20260525 depth-14 x2 | 2 | 2 | 0 | 174 | 246 | 246 | 127,581 | 110,303,375 |
| Embedded | seed 20260525 depth-15 x1 | 1 | 1 | 0 | 320 | 320 | 320 | 93,975 | 110,303,375 |
| Embedded | seed 42 depth-14 x2 | 2 | 2 | 0 | 10,181 | 11,679 | 11,679 | 8,947,939 | 110,303,375 |
| Embedded | seed 42 depth-15 x1 | 1 | 0 | 1 | 30,000 | 30,000 | 30,000 | 13,508,360 | 110,303,375 |
| Default | seed 12345 depth-14 x2 | 2 | 2 | 0 | 223 | 4,819 | 4,819 | 1,500,490 | 293,502,335 |
| Default | seed 12345 depth-15 x1 | 1 | 0 | 1 | 30,000 | 30,000 | 30,000 | 9,209,621 | 293,502,335 |
| Default | seed 20260525 depth-14 x2 | 2 | 2 | 0 | 113 | 126 | 126 | 56,274 | 293,502,335 |
| Default | seed 20260525 depth-15 x1 | 1 | 1 | 0 | 177 | 177 | 177 | 37,937 | 293,502,335 |
| Default | seed 42 depth-14 x2 | 2 | 2 | 0 | 5,535 | 7,287 | 7,287 | 4,039,678 | 293,502,335 |
| Default | seed 42 depth-15 x1 | 1 | 0 | 1 | 30,000 | 30,000 | 30,000 | 9,550,119 | 293,502,335 |
| Performance | seed 12345 depth-14 x2 | 2 | 2 | 0 | 216 | 4,509 | 4,509 | 1,486,568 | 434,636,735 |
| Performance | seed 12345 depth-15 x1 | 1 | 0 | 1 | 30,000 | 30,000 | 30,000 | 9,502,736 | 434,636,735 |
| Performance | seed 20260525 depth-14 x2 | 2 | 2 | 0 | 111 | 122 | 122 | 55,341 | 434,636,735 |
| Performance | seed 20260525 depth-15 x1 | 1 | 1 | 0 | 176 | 176 | 176 | 37,627 | 434,636,735 |
| Performance | seed 42 depth-14 x2 | 2 | 2 | 0 | 5,560 | 7,285 | 7,285 | 3,998,283 | 434,636,735 |
| Performance | seed 42 depth-15 x1 | 1 | 0 | 1 | 30,000 | 30,000 | 30,000 | 9,535,545 | 434,636,735 |

The sweep confirms that the corner-state table is a real depth-14 improvement:
all sampled depth-14 cases solve within 11.679 seconds on the embedded profile
and within 7.287 seconds on the default/performance profiles. It is not enough
to claim the depth-15 frontier is controlled: two of three sampled depth-15
cases still time out at 30 seconds on every profile except the easy seed
`20260525` case.

The table remains behind an environment flag until the depth-15 tail is improved
or the default timeout/profile contract is explicitly changed.

Two follow-up experiments were checked against the default seed `12345`
depth-15 frontier case:

| Experiment | Timeout | Status | Elapsed ms | Nodes | Interpretation |
| --- | ---: | --- | ---: | ---: | --- |
| Phase-2 ordering tie-break | 30,000 | Timeout | 30,000 | 9,138,006 | Slightly fewer nodes than the 30s baseline, but not enough to solve. |
| Exact goal table radius 6 | 120,000 | Optimal | 38,656 | 11,233,637 | Correct, but slower than the corner-state-only 120s probe. |

Neither follow-up is a promotion candidate. The phase-2 tie-break remains
available only as an experiment, and the goal-table path is useful mainly as a
correctness reference for future meet-in-the-middle work.

Larger corner/edge-group PDB follow-up:

| Experiment | Case | Timeout | Status | Elapsed ms | Nodes | Payload | Interpretation |
| --- | --- | ---: | --- | ---: | ---: | ---: | --- |
| Corner-state + corner/U-edge group | seed `12345` depth-15 | 30,000 | Optimal | 28,914 | 8,705,291 | 772,503,935 | First depth-15 hard sample solved under 30 seconds. |
| Corner-state + corner/U-edge group | seed `42` depth-15 | 30,000 | Timeout | 30,000 | 9,027,461 | 772,503,935 | Still misses the 30s target. |
| Corner-state + corner/D-edge group | seed `42` depth-15 | 30,000 | Timeout | 30,000 | 9,040,506 | 772,503,935 | Similar to U-edge for this case. |
| Corner-state + both corner/edge groups, 1 thread | seed `42` depth-15 | 120,000 | Optimal | 60,971 | 18,296,008 | 1,251,505,535 | Solves, but still far above the 30s target. |
| Corner-state + both corner/edge groups, performance, 4 threads | seed `42` depth-15 | 30,000 | Optimal | 17,896 | 18,585,722 | 1,392,639,935 | First seed `42` depth-15 run under 30 seconds. |
| Corner-state + both corner/edge groups, performance, 4 threads | seed `12345` depth-15 | 30,000 | Optimal | 3,490 | 3,447,949 | 1,392,639,935 | Confirms the other hard sample remains controlled. |

The corner/edge-group table is the first depth-15 experiment that solves one of
the previous 30-second timeout samples without raising the timeout. The cost is
large: each table has 479,001,600 entries and the first cache build took about
102-135 seconds on the development desktop. A single table plus corner-state
fits under the current 1 GB logical benchmark memory budget, but using both
U-edge and D-edge variants together does not.

Root-parallel optimal search is now wired to `SolveOptions::threads`. With the
large local profile above, four threads brought the remaining sampled depth-15
timeout under 30 seconds. Two threads were not enough for seed `42`, timing out
at 30 seconds after 15,970,278 expanded nodes.

The large local setup is now captured by the `optimal-large-local` benchmark
suite. Warm-cache run with seeds `12345`, `20260525`, and `42`:

| Profile | Threads | Seed | Cases | Solved | Failed | P95 | Max | Nodes | Payload |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Performance | 4 | 12345 | 1 | 1 | 0 | 3,764 | 3,764 | 3,440,146 | 1,392,639,935 |
| Performance | 4 | 20260525 | 1 | 1 | 0 | 108 | 108 | 46,654 | 1,392,639,935 |
| Performance | 4 | 42 | 1 | 1 | 0 | 19,531 | 19,531 | 18,602,523 | 1,392,639,935 |

The initial dedicated large-local gates passed with a 30 second max latency
threshold on all three sampled depth-15 seeds.

Expanded large-local sweep:

| Profile | Threads | Seeds | Cases | Solved | Failed | Max | Worst Seed | Payload |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Performance | 4 | 24 fixed seeds | 24 | 24 | 0 | 26,660 | `987654321` | 1,392,639,935 |

Per-seed max elapsed:

| Seed | Status | Elapsed ms | Nodes |
| ---: | --- | ---: | ---: |
| 12345 | Optimal | 3,835 | 3,448,048 |
| 20260525 | Optimal | 110 | 46,858 |
| 42 | Optimal | 19,429 | 18,598,507 |
| 314159 | Optimal | 1,437 | 1,336,457 |
| 271828 | Optimal | 1,483 | 1,376,865 |
| 987654321 | Optimal | 26,660 | 25,539,650 |
| 7 | Optimal | 1,369 | 1,250,072 |
| 99 | Optimal | 21,945 | 20,822,003 |
| 123456789 | Optimal | 1,356 | 1,251,643 |
| 424242 | Optimal | 25,659 | 24,119,391 |
| 8675309 | Optimal | 20,547 | 20,136,676 |
| 20240525 | Optimal | 518 | 435,503 |
| 111 | Optimal | 1,250 | 1,115,088 |
| 222 | Optimal | 243 | 167,971 |
| 333 | Optimal | 12,938 | 11,553,991 |
| 444 | Optimal | 17,003 | 14,933,751 |
| 555 | Optimal | 23,813 | 21,972,485 |
| 666 | Optimal | 24,109 | 22,194,622 |
| 777 | Optimal | 103 | 40,389 |
| 888 | Optimal | 20,319 | 18,569,696 |
| 999 | Optimal | 2,973 | 2,522,287 |
| 13579 | Optimal | 9,005 | 7,967,553 |
| 24680 | Optimal | 1,942 | 1,733,731 |
| 112358 | Optimal | 1,097 | 976,930 |

The expanded gate set now uses these twenty-four fixed seeds. The worst samples are
close enough to 30 seconds that the next validation step should keep adding
depth-15 seeds before treating this as a stable release profile.

8-thread tail replay:

| Profile | Threads | Seeds | Cases | Solved | Failed | Max | Worst Seed | Payload |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Performance | 8 | `987654321,424242,666,555,99,888` | 6 | 6 | 0 | 16,958 | `987654321` | 1,392,639,935 |

Per-seed max elapsed:

| Seed | 4-thread ms | 8-thread ms | Nodes at 8 threads |
| ---: | ---: | ---: | ---: |
| 987654321 | 26,660 | 16,958 | 27,965,340 |
| 424242 | 25,659 | 14,677 | 23,815,005 |
| 666 | 24,109 | 10,775 | 17,676,699 |
| 555 | 23,813 | 13,400 | 21,806,391 |
| 99 | 21,945 | 11,053 | 17,854,969 |
| 888 | 20,319 | 9,522 | 14,947,024 |

The 8-thread run is a high-throughput local latency profile. It reduces wall
time substantially, while sometimes expanding more nodes because parallel
workers can continue until the winning branch stops the search.

This suite has no pass/fail latency gate yet. It is a measurement tool for
finding and replaying frontier cases before promoting a new optimization into
the gated benchmark set.
