# V6 Tail Corpus - 2026-05-28

This document records the replayable tail corpus used to guide V6 optimal
latency work. It uses the V6 runner added after the V5 baseline, but no solver
behavior changed for this run.

## Scope

The corpus targets local `SolveMode::Optimal` latency while preserving the
certified HTM optimality contract. When a result reports `SolveStatus::Optimal`,
the returned solution must remain a proven minimum-length HTM solution for the
requested options.

These are local desktop measurements only. No external hardware, GPU, or cloud
latency claims are included.

## Environment

- Commit: `ca1c2ff`
- Build preset: `release-native-lto`
- Benchmark runner: `scripts/run_v6_tail_baseline.sh`
- Build directory: `out/release-native-lto`
- Cache directory: `/tmp/rubik_cube_library_v6_tail_baseline_cache`
- Output directory: `out/release-native-lto/benchmark-results/v6-tail-baseline`
- Solver mode: `SolveMode::Optimal`
- Requested benchmark profile: `SolveProfile::Auto`
- Effective benchmark profile: `large-local`
- Threads selected by Auto: 16
- Memory budget: 2048 MiB
- Table payload prepared: 1392639935 bytes
- Cache setup elapsed: 226529 ms
- Cache mode: warm
- Timeout per case: 30000 ms

## Command

```sh
cmake --build out/release-native-lto --target rubik-benchmark-v6-tail-baseline
```

The runner writes a manifest and two suite summaries:

- `out/release-native-lto/benchmark-results/v6-tail-baseline/manifest.csv`
- `out/release-native-lto/benchmark-results/v6-tail-baseline/cache_setup.csv`
- `out/release-native-lto/benchmark-results/v6-tail-baseline/optimal-auto-tail/warm_optimal_auto_tail_summary.csv`
- `out/release-native-lto/benchmark-results/v6-tail-baseline/optimal-auto-hardening/warm_optimal_auto_hardening_summary.csv`

## Corpus Inputs

- Tail seeds: `987654321,424242,1009,666,555,99,888`
- Hardening seeds: `12345,20260525,42,314159,271828,987654321,7,99,123456789,424242,8675309,20240525`
- Deep depth-14 count per hardening seed: 2
- Deep depth-15 count per hardening seed: 1

The tail suite contains depth-15 single-case probes for known slow seeds. The
hardening suite mixes depth-14 two-case probes with depth-15 single-case probes
across a wider seed set.

## Aggregate Results

| Suite | Cases | Solved | Failed | Total solver ms | Total nodes | Max solver ms | Max wall ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| optimal-auto-tail | 7 | 7 | 0 | 25782 | 89104292 | 7535 | 8171 |
| optimal-auto-hardening | 36 | 36 | 0 | 32630 | 109609949 | 7636 | 8282 |
| combined | 43 | 43 | 0 | 58412 | 198714241 | 7636 | 8282 |

The slowest measured solver case in this corpus is
`random_seed_987654321_depth_15_count_1` from `optimal-auto-hardening`:

- solver elapsed: 7636 ms;
- wall elapsed: 8282 ms;
- nodes expanded: 26350894;
- scramble: `B' R' B' R' F B D' F R U2 F B U' B2 F`;
- solution length: 15 HTM.

## Slowest Rows

| Suite | Benchmark | Solver ms | Wall ms | Nodes |
| --- | --- | ---: | ---: | ---: |
| optimal-auto-hardening | `random_seed_987654321_depth_15_count_1` | 7636 | 8282 | 26350894 |
| optimal-auto-tail | `random_seed_987654321_depth_15_count_1` | 7535 | 8171 | 26185854 |
| optimal-auto-tail | `random_seed_1009_depth_15_count_1` | 7451 | 8094 | 28264755 |
| optimal-auto-hardening | `random_seed_12345_depth_15_count_1` | 4838 | 5480 | 17568403 |
| optimal-auto-hardening | `random_seed_42_depth_15_count_1` | 3957 | 4596 | 13136657 |
| optimal-auto-tail | `random_seed_424242_depth_15_count_1` | 2931 | 3571 | 10233108 |
| optimal-auto-hardening | `random_seed_424242_depth_15_count_1` | 2919 | 3565 | 10234933 |
| optimal-auto-tail | `random_seed_555_depth_15_count_1` | 2907 | 3557 | 9652387 |
| optimal-auto-hardening | `random_seed_8675309_depth_15_count_1` | 2416 | 3065 | 9338329 |
| optimal-auto-tail | `random_seed_99_depth_15_count_1` | 2050 | 2695 | 6196966 |
| optimal-auto-hardening | `random_seed_99_depth_15_count_1` | 2037 | 2676 | 6161968 |
| optimal-auto-tail | `random_seed_888_depth_15_count_1` | 1935 | 2577 | 5767448 |

## Optimization Reading

The corpus gives V6 a stable local target for tail-latency work. The first
optimization candidates are the depth-15 Auto large-local rows where the solver
expands more than 25 million nodes before proving optimality:

- `random_seed_987654321_depth_15_count_1`
- `random_seed_1009_depth_15_count_1`

Both cases are dominated by proof work rather than input validation, table
setup, or API overhead. The next implementation step should inspect their root
diagnostics and reduce unnecessary root or split-task exploration without
weakening the optimality proof.
