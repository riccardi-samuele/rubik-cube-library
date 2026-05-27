# Benchmarks

Benchmarks are run with `rubik-bench`.

The repeatable benchmark suite is wrapped by
`scripts/run_benchmark_suite.sh`. The script writes one CSV-like output file per
benchmark into `benchmark-results/` by default.

Quick suite:

```sh
cmake --build build --target rubik-benchmark-smoke
cmake --build build --target rubik-benchmark-smoke-cold
cmake --build build --target rubik-benchmark-profile-smoke
cmake --build build --target rubik-benchmark-profile-realistic
cmake --build build --target rubik-benchmark-profile-realistic-gates
cmake --build build --target rubik-benchmark-auto-profile
cmake --build build --target rubik-benchmark-auto-profile-gates
cmake --build build --target rubik-benchmark-optimal-stress
cmake --build build --target rubik-benchmark-optimal-stress-gates
cmake --build build --target rubik-benchmark-optimal-tail-cases
cmake --build build --target rubik-benchmark-optimal-tail-cases-gates
cmake --build build --target rubik-benchmark-optimal-deep-probe
cmake --build build --target rubik-benchmark-optimal-large-local
cmake --build build --target rubik-benchmark-optimal-large-local-gates
cmake --build build --target rubik-benchmark-optimal-large-local-tail-8threads
cmake --build build --target rubik-benchmark-optimal-large-local-tail-8threads-gates
cmake --build build --target rubik-benchmark-optimal-auto-tail
cmake --build build --target rubik-benchmark-optimal-auto-tail-gates
cmake --build build --target rubik-benchmark-embedded-multiseed
cmake --build build --target rubik-benchmark-embedded-multiseed-gates
cmake --build build --target rubik-benchmark-embedded-fast-tail-cases
cmake --build build --target rubik-benchmark-embedded-fast-failures
cmake --build build --target rubik-benchmark-optimal-ab
```

For performance numbers intended for comparison, prefer an optimized preset:

```sh
cmake --preset release-native-lto
cmake --build --preset release-native-lto
ctest --preset release-native-lto
```

Use `release` for portable release measurements, `release-native` for local CPU
tuning, `release-lto` for portable link-time optimization, and
`release-native-lto` for the fastest local desktop benchmark build.

Manual suite commands:

```sh
scripts/run_benchmark_suite.sh --suite smoke
scripts/run_benchmark_suite.sh --suite profile-smoke
scripts/run_benchmark_suite.sh --suite profile-realistic
scripts/check_benchmark_gates.sh --summary-file benchmark-results/warm_profile_realistic_summary.csv --gate embedded,fast,random_depth_20_count_100,100,350,500,700
scripts/run_benchmark_suite.sh --suite optimal-stress --seeds 12345,20260525,42 --realistic-opt13-count 10
scripts/run_benchmark_suite.sh --suite optimal-tail-cases
scripts/run_benchmark_suite.sh --suite optimal-deep-probe --seeds 12345 --deep-opt14-count 2 --deep-opt15-count 1
scripts/run_benchmark_suite.sh --suite optimal-large-local --seeds 12345,20260525,42,314159,271828,987654321,7,99,123456789,424242,8675309,20240525,111,222,333,444,555,666,777,888,999,13579,24680,112358 --threads 4 --max-memory-mb 2048 --deep-opt15-count 1
scripts/run_benchmark_suite.sh --suite optimal-auto-tail --seeds 987654321,424242,666,555,99,888 --threads 0 --max-memory-mb 2048 --deep-opt15-count 1
scripts/run_benchmark_suite.sh --suite embedded-multiseed --seeds 12345,20260525,42 --realistic-fast-count 100 --fast-max-depth 28 --realistic-opt13-count 10
scripts/check_benchmark_gates.sh --summary-file benchmark-results/warm_embedded_multiseed_summary.csv --gate embedded,fast,random_seed_20260525_depth_20_count_100,100,350,500,700
scripts/run_benchmark_suite.sh --suite embedded-fast-tail-cases
scripts/run_benchmark_suite.sh --suite embedded-fast-failures
scripts/run_benchmark_suite.sh --suite fast-100
scripts/run_benchmark_suite.sh --suite fast-1000
scripts/run_benchmark_suite.sh --suite optimal-depth
scripts/run_benchmark_suite.sh --suite tail-diagnostics
scripts/run_benchmark_suite.sh --suite smoke --cache-mode cold
scripts/run_benchmark_suite.sh --suite optimal-depth --profile performance
scripts/run_benchmark_suite.sh --suite profile-realistic --realistic-fast-count 20 --realistic-opt12-count 10 --realistic-opt13-count 5
scripts/run_optimal_ab.sh --case-set random --random-count 8 --random-depth 12 --max-depth 12 --timeout-ms 5000 --repetitions 3
scripts/extract_slowest_cases.sh --input-dir benchmark-results --output benchmark-results/slowest-cases.csv --limit 25
scripts/run_v2_optimal_baseline.sh --build-dir out/release-native-lto
scripts/run_optimal_tail_experiments.sh --build-dir out/release-native-lto --variants baseline,corner_state
```

`--fast-max-depth` defaults to `24`. The release-candidate
`embedded-multiseed` target uses `28` so the fast-mode robustness sweep does not
fail on valid depth-20 scrambles that need a non-optimal fast solution longer
than 24 moves.

Current profile comparison:

- [V2 Optimal Baseline - 2026-05-26](v2-optimal-baseline-2026-05-26.md)
- [V2 Corner-State Validation - 2026-05-26](v2-corner-state-validation-2026-05-26.md)
- [V2 Depth-15 Large-Local Validation - 2026-05-26](v2-depth15-large-local-2026-05-26.md)
- [Optimal Tail Experiments - 2026-05-26](optimal-tail-experiments-2026-05-26.md)
- [Release Candidate Validation - 2026-05-26](release-candidate-2026-05-26.md)
- [Optimal Stress Benchmark - 2026-05-25](benchmark-optimal-stress-2026-05-25.md)
- [Optimal Stress Corner-State Experiment - 2026-05-25](benchmark-optimal-stress-corner-state-2026-05-25.md)
- [Embedded Optimal Tail-Case Diagnostics - 2026-05-25](optimal-tail-cases-2026-05-25.md)
- [Optimal Deep Probe - 2026-05-25](optimal-deep-probe-2026-05-25.md)
- [Embedded Fast Tail-Case Diagnostics - 2026-05-25](embedded-fast-tail-cases-2026-05-25.md)
- [Embedded Multiseed Benchmark - 2026-05-25](benchmark-embedded-multiseed-2026-05-25.md)
- [Benchmark Gate Calibration - 2026-05-25](benchmark-gate-calibration-2026-05-25.md)
- [Profile Realistic Large Benchmark - 2026-05-25](benchmark-profile-realistic-large-2026-05-25.md)
- [Profile Realistic Benchmark - 2026-05-25](benchmark-profile-realistic-2026-05-25.md)
- [Embedded Fast Failure Diagnostics - 2026-05-25](embedded-fast-failures-2026-05-25.md)

Optimal A/B runner:

```sh
scripts/run_optimal_ab.sh --case-set deterministic --max-case-depth 13 --max-depth 13 --timeout-ms 30000 --repetitions 3
scripts/run_optimal_ab.sh --case-set random --random-count 8 --random-depth 12 --random-seed 20260525 --max-depth 12 --timeout-ms 5000 --repetitions 3
```

The A/B runner alternates `RUBIK_DISABLE_THREE_PHASE1_BOUNDS=1` baseline runs
and three-phase runs, writes per-run CSV files, and writes `summary.csv` plus
`aggregate.csv` for average elapsed time and expanded nodes.

Slowest-case extraction:

```sh
scripts/extract_slowest_cases.sh \
  --input-dir out/release-native-lto/benchmark-results/optimal-stress \
  --output out/release-native-lto/benchmark-results/optimal-stress/slowest-cases.csv \
  --limit 25
```

The extractor scans `slowest` rows from `rubik-bench` CSV output and writes a
single descending list by elapsed time. Use it after stress, tail, or deep-probe
runs to capture replay candidates before changing optimal-mode pruning or
search policy.

V2 optimal baseline:

```sh
scripts/run_v2_optimal_baseline.sh --build-dir out/release-native-lto
```

The V2 baseline runner executes the current optimal stress and optimal tail-case
suites, writes a manifest with the selected seed/profile configuration, and
then generates `slowest-cases.csv`. Add `--include-deep-probe` for slower
depth-14/depth-15 frontier mapping before larger optimal-engine changes.

Optimal tail experiments:

```sh
scripts/run_optimal_tail_experiments.sh \
  --build-dir out/release-native-lto \
  --variants baseline,no_corner_state,corner_state_up,corner_state_down \
  --max-memory-mb 2048
```

The tail experiment runner replays the fixed slow depth-13 tail cases and
compares experimental optimal-engine variants under the same profile, memory
budget, timeout, and thread count. It writes `summary.csv` and `aggregate.csv`
so pruning changes can be promoted or rejected based on the same cases.
After the V2 corner-state promotion, `baseline` means the current default
optimal policy and `no_corner_state` means the older 1.0-style pruning policy.

Three-direction phase-1 bounds are now enabled by default for
`SolveMode::Optimal` with all public profiles, including `Embedded`. Use
`RUBIK_DISABLE_THREE_PHASE1_BOUNDS=1` to measure the old optimal baseline, or
`RUBIK_EXPERIMENTAL_THREE_PHASE1_BOUNDS=1` to force-enable the bound in
developer experiments such as fast fallback checks.

Experimental optimal child ordering:

```sh
RUBIK_EXPERIMENTAL_STRONG_OPTIMAL_ORDERING=1 scripts/run_benchmark_suite.sh --suite optimal-deep-probe
```

This orders DFS children by the strongest already-computed candidate lower bound
instead of the historical base bound. The first embedded depth-14 frontier test
expanded more nodes with this flag, so it remains experimental.

Experimental phase-2 optimal ordering:

```sh
RUBIK_EXPERIMENTAL_PHASE2_OPTIMAL_ORDERING=1 scripts/run_benchmark_suite.sh --suite optimal-deep-probe
```

This uses phase-2 tables only as a child-ordering tie-break after a candidate
state has already entered the phase-2 subgroup. It does not prune and therefore
does not affect optimality. The first depth-15 frontier test reduced nodes
slightly but still timed out at 30 seconds, so it is not a promotion candidate.

Experimental exact goal table:

```sh
RUBIK_EXPERIMENTAL_OPTIMAL_GOAL_TABLE_DEPTH=6 scripts/run_benchmark_suite.sh --suite optimal-deep-probe
```

This builds an exact table of states up to the configured distance from solved
and lets IDA* close branches when the remaining depth is covered by that table.
The cutoff is exact, but the radius-6 probe was slower than corner-state pruning
alone on the current depth-15 frontier case.

Experimental corner/edge-group pruning:

```sh
RUBIK_EXPERIMENTAL_CORNER_UP_EDGE_BOUNDS=1 \
scripts/run_benchmark_suite.sh --suite optimal-deep-probe
```

`RUBIK_EXPERIMENTAL_CORNER_UP_EDGE_BOUNDS=1` and
`RUBIK_EXPERIMENTAL_CORNER_DOWN_EDGE_BOUNDS=1` add a 479,001,600-entry table
over corner permutation plus one edge group. A single table plus default
corner-state pruning fits under the current 1 GB logical memory budget. The
U-edge variant solved the seed `12345` depth-15 frontier case in 28.914
seconds, but seed `42` still timed out at 30 seconds, so this remains
experimental.

Large local optimal profile:

```sh
out/release-native-lto/rubik-bench \
  --mode optimal \
  --profile large-local \
  --threads 4 \
  --max-memory-mb 2048 \
  --timeout-ms 30000 \
  --max-depth 15 \
  --case-set random \
  --random-count 1 \
  --random-depth 15 \
  --random-seed 42
```

`--profile large-local` enables the largest local optimal admissible bounds
without environment flags. `--threads` controls root-parallel optimal search.
`--max-memory-mb` raises the benchmark memory contract for this high-memory
profile; the default remains 1,024 MB.

The same setup is available as a repeatable suite:

```sh
scripts/run_benchmark_suite.sh \
  --suite optimal-large-local \
  --seeds 12345,20260525,42,314159,271828,987654321,7,99,123456789,424242,8675309,20240525,111,222,333,444,555,666,777,888,999,13579,24680,112358 \
  --threads 4 \
  --max-memory-mb 2048 \
  --deep-opt15-count 1
```

The current CMake target uses twenty-four fixed depth-15 seeds and a 30 second gate:
`12345`, `20260525`, `42`, `314159`, `271828`, `987654321`, `7`, `99`,
`123456789`, `424242`, `8675309`, `20240525`, `111`, `222`, `333`, `444`,
`555`, `666`, `777`, `888`, `999`, `13579`, `24680`, and `112358`.

The current 8-thread tail target replays the six slowest known depth-15 seeds
with an 18 second gate: `987654321`, `424242`, `666`, `555`, `99`, and `888`.

Auto optimal profile:

```sh
out/release-native-lto/rubik-bench \
  --mode optimal \
  --profile auto \
  --threads 0 \
  --max-memory-mb 2048 \
  --timeout-ms 30000 \
  --max-depth 15 \
  --case-set random \
  --random-count 1 \
  --random-depth 15 \
  --random-seed 987654321
```

`--profile auto` lets the planner select the effective local profile and
thread count. Benchmark output reports both `requested_profile` and the
effective `profile`, plus `adaptive_strategy`, `threads`,
`warmup_table_payload_bytes`, and `warmup_elapsed_ms`.

Use `rubik-cache-setup --profile auto` or `prepareCache()` before
latency-sensitive benchmark runs when cold-cache setup should be separated from
solve latency. The Auto benchmark targets use warm cache mode and still print
warm-up timing so reports can distinguish cache preparation from search time.

The repeatable Auto gates are:

```sh
cmake --build out/release-native-lto --target rubik-benchmark-auto-profile
cmake --build out/release-native-lto --target rubik-benchmark-auto-profile-gates
cmake --build out/release-native-lto --target rubik-benchmark-optimal-auto-tail
cmake --build out/release-native-lto --target rubik-benchmark-optimal-auto-tail-gates
```

Default corner-state pruning:

```sh
scripts/run_benchmark_suite.sh --suite optimal-deep-probe
```

Optimal mode uses a full corner orientation + corner permutation pruning table
by default. It is admissible and costs 88,179,840 additional table bytes. The
V2 tail experiment reduced the fixed embedded tail-case average from
6,516.20 ms to 1,482.60 ms. Use `RUBIK_DISABLE_CORNER_STATE_BOUNDS=1` only for
A/B comparisons against the older 1.0 pruning policy.

Cache modes:

- `--cache-mode warm`: reuse the configured cache directory across all
  benchmarks in the suite. This is the default and measures solving after
  pruning tables already exist.
- `--cache-mode cold`: delete and recreate the configured cache directory before
  each benchmark command. This measures table generation, disk writes, and
  solving together.

Profile selection:

- `--profile embedded|default|performance|large-local`: run a normal suite under one solver
  profile.
- `--suite profile-smoke`: run a short fast and optimal smoke benchmark for
  `embedded`, `default`, and `performance`, then write
  `warm_profile_smoke_summary.csv` or `cold_profile_smoke_summary.csv`. Use
  this after table-profile or heuristic changes.
- `--suite profile-realistic`: run a larger local comparison for `embedded`,
  `default`, and `performance`: fast random depth-20, optimal random depth-12,
  and optimal random depth-13.
  It writes `*_profile_realistic_summary.csv` with average, percentile, max,
  warm-up, and wall-clock timing columns.
  The CMake target pins calibrated counts to fast `100`, optimal depth-12 `20`,
  and optimal depth-13 `10`.
- `--suite embedded-multiseed`: run `Embedded/Fast` random depth-20 and
  `Embedded/Optimal` random depth-13 across a comma-separated seed list. This is
  the broader robustness sweep used to find seed-specific pathological cases.
- `--suite optimal-stress`: run `Optimal` random depth-13 across
  embedded/default/performance profiles and a comma-separated seed list. This is
  the broader tail-latency sweep used before changing optimal-mode pruning or
  search policy.
- `--suite optimal-tail-cases`: replay the current slowest `Embedded/Optimal`
  depth-13 cases from the optimal stress suite with `--diagnose-optimal`.
- `--suite optimal-deep-probe`: run a small, non-gated optimal depth-14/depth-15
  probe across public profiles with `--diagnose-optimal`. Timeout rows are kept
  in the summary so the suite can map the current search frontier.
- `--suite optimal-large-local`: run the large local `LargeLocal/Optimal`
  depth-15 probe across fixed seeds. This suite is intended for desktop/Orin
  class validation and uses the public `large-local` profile.
- `--suite optimal-auto-tail`: replay the slowest known depth-15 local optimal
  seeds through `SolveProfile::Auto`. The CMake target uses automatic thread
  selection and a 2 GiB memory budget to validate the adaptive desktop path.
- `--suite embedded-fast-tail-cases`: replay the current slowest
  `Embedded/Fast` random depth-20 cases from the multiseed sweep with
  `--diagnose-fast`.
- `--suite embedded-fast-failures`: replay the current `Embedded/Fast`
  random depth-20 failure cases from seed `12345`, indices 5 and 13, with
  `--diagnose-fast`.
- `--realistic-fast-count`, `--realistic-opt12-count`, and
  `--realistic-opt13-count`: reduce or expand the realistic profile suite.
- `--deep-opt14-count` and `--deep-opt15-count`: reduce or expand the
  `optimal-deep-probe` depth-14/depth-15 samples.
- `--threads` and `--max-memory-mb`: forward solver thread count and memory
  budget into `rubik-bench`.
- `--seeds`: comma-separated seed list for multiseed and optimal probe suites.

Output files are prefixed with the cache mode, for example
`warm_fast_random_100_depth_20_seed_12345.csv` or
`cold_smoke_fast_random_5_seed_12345.csv`.

Benchmark gates:

```sh
scripts/check_benchmark_gates.sh \
  --summary-file out/release-native-lto/benchmark-results/profile-realistic/warm_profile_realistic_summary.csv \
  --gate embedded,fast,random_depth_20_count_100,100,350,500,700 \
  --gate embedded,optimal,random_depth_13_count_10,10,4000,4000,4000 \
  --gate default,optimal,random_depth_13_count_10,10,2500,2500,2500 \
  --gate performance,optimal,random_depth_13_count_10,10,2500,2500,2500
```

Each gate is
`profile,mode,benchmark,min_solved,max_p95_ms,max_p99_ms,max_max_ms`. Use `-1`
for a latency threshold to skip that check. Gates are intentionally separate
from normal `ctest` because profile-realistic is a long benchmark; run the
benchmark first, then run the gates against its summary CSV.
The current optimal thresholds include the promoted corner-state bound
validation; see [V2 Corner-State Validation - 2026-05-26](v2-corner-state-validation-2026-05-26.md).

Embedded multiseed gates:

```sh
scripts/check_benchmark_gates.sh \
  --summary-file out/release-native-lto/benchmark-results/embedded-multiseed/warm_embedded_multiseed_summary.csv \
  --gate embedded,fast,random_seed_12345_depth_20_count_100,100,350,500,700 \
  --gate embedded,optimal,random_seed_12345_depth_13_count_10,10,4000,4000,4000 \
  --gate embedded,fast,random_seed_20260525_depth_20_count_100,100,350,500,700 \
  --gate embedded,optimal,random_seed_20260525_depth_13_count_10,10,4000,4000,4000 \
  --gate embedded,fast,random_seed_42_depth_20_count_100,100,350,500,700 \
  --gate embedded,optimal,random_seed_42_depth_13_count_10,10,4000,4000,4000 \
  --gate embedded,fast,random_seed_314159_depth_20_count_100,100,350,500,700 \
  --gate embedded,optimal,random_seed_314159_depth_13_count_10,10,4000,4000,4000 \
  --gate embedded,fast,random_seed_271828_depth_20_count_100,100,350,500,700 \
  --gate embedded,optimal,random_seed_271828_depth_13_count_10,10,4000,4000,4000 \
  --gate embedded,fast,random_seed_987654321_depth_20_count_100,100,350,500,700 \
  --gate embedded,optimal,random_seed_987654321_depth_13_count_10,10,4000,4000,4000 \
  --gate embedded,fast,random_seed_7_depth_20_count_100,100,350,500,700 \
  --gate embedded,optimal,random_seed_7_depth_13_count_10,10,4000,4000,4000 \
  --gate embedded,fast,random_seed_99_depth_20_count_100,100,350,500,700 \
  --gate embedded,optimal,random_seed_99_depth_13_count_10,10,4000,4000,4000 \
  --gate embedded,fast,random_seed_123456789_depth_20_count_100,100,350,500,700 \
  --gate embedded,optimal,random_seed_123456789_depth_13_count_10,10,4000,4000,4000 \
  --gate embedded,fast,random_seed_424242_depth_20_count_100,100,350,500,700 \
  --gate embedded,optimal,random_seed_424242_depth_13_count_10,10,4000,4000,4000 \
  --gate embedded,fast,random_seed_8675309_depth_20_count_100,100,350,500,700 \
  --gate embedded,optimal,random_seed_8675309_depth_13_count_10,10,4000,4000,4000 \
  --gate embedded,fast,random_seed_20240525_depth_20_count_100,100,350,500,700 \
  --gate embedded,optimal,random_seed_20240525_depth_13_count_10,10,4000,4000,4000
```

These are robustness gates for the current desktop seed set. The CMake target
checks every seed emitted by the embedded-multiseed suite.

Optimal stress gates:

```sh
scripts/check_benchmark_gates.sh \
  --summary-file out/release-native-lto/benchmark-results/optimal-stress/warm_optimal_stress_summary.csv \
  --gate embedded,optimal,random_seed_12345_depth_13_count_10,10,4000,4000,4000 \
  --gate embedded,optimal,random_seed_20260525_depth_13_count_10,10,4000,4000,4000 \
  --gate embedded,optimal,random_seed_42_depth_13_count_10,10,4000,4000,4000 \
  --gate default,optimal,random_seed_12345_depth_13_count_10,10,2500,2500,2500 \
  --gate default,optimal,random_seed_20260525_depth_13_count_10,10,2500,2500,2500 \
  --gate default,optimal,random_seed_42_depth_13_count_10,10,2500,2500,2500 \
  --gate performance,optimal,random_seed_12345_depth_13_count_10,10,2500,2500,2500 \
  --gate performance,optimal,random_seed_20260525_depth_13_count_10,10,2500,2500,2500 \
  --gate performance,optimal,random_seed_42_depth_13_count_10,10,2500,2500,2500
```

The optimal thresholds were tightened after promoting corner-state pruning:
`4000 ms` for embedded and `2500 ms` for default/performance.

Optimal tail-case gates:

```sh
scripts/check_benchmark_gates.sh \
  --summary-file out/release-native-lto/benchmark-results/optimal-tail-cases/warm_optimal_tail_cases_summary.csv \
  --gate embedded,optimal,random_seed_12345_index_4_depth_13,1,4000,4000,4000 \
  --gate embedded,optimal,random_seed_42_index_2_depth_13,1,4000,4000,4000 \
  --gate embedded,optimal,random_seed_42_index_1_depth_13,1,4000,4000,4000 \
  --gate embedded,optimal,random_seed_20260525_index_7_depth_13,1,4000,4000,4000 \
  --gate embedded,optimal,random_seed_12345_index_2_depth_13,1,4000,4000,4000
```

These gates replay the current slowest embedded optimal cases with diagnostics.
They are intended for tight inner-loop comparison while changing optimal-mode
pruning. The current tail-case gate is `4000 ms` after promoting corner-state
pruning by default.

Example:

```sh
RUBIK_TABLE_CACHE_DIR=/tmp/rubik_cube_library_cache \
./build/rubik-bench --mode fast --timeout-ms 30000 --max-depth 24 --max-case-depth 13
```

Reproducible random benchmark:

```sh
RUBIK_TABLE_CACHE_DIR=/tmp/rubik_cube_library_cache \
./build/rubik-bench --mode fast --case-set random --random-count 100 --random-depth 20 --random-seed 12345 --timeout-ms 5000 --max-depth 24 --slowest-count 10
```

Replay a specific random case from the same deterministic sequence:

```sh
RUBIK_TABLE_CACHE_DIR=/tmp/rubik_cube_library_cache \
./build/rubik-bench --mode fast --case-set random --random-count 1 --random-depth 20 --random-seed 12345 --random-start-index 50 --timeout-ms 5000 --max-depth 24
```

Diagnose fast-mode phase costs for a specific case:

```sh
RUBIK_TABLE_CACHE_DIR=/tmp/rubik_cube_library_cache \
./build/rubik-bench --mode fast --case-set random --random-count 1 --random-depth 20 --random-seed 12345 --random-start-index 50 --timeout-ms 5000 --max-depth 24 --diagnose-fast
```

Available case sets:

- `--case-set deterministic`: built-in fixed cases, filtered by `--max-case-depth`.
- `--case-set random`: generated random cases only.
- `--case-set both`: deterministic cases followed by generated random cases.
- `--random-start-index N`: first generated random case to include, using
  one-based numbering.
- `--slowest-count N`: number of slowest cases reported at the end.
- `--diagnose-fast`: print `diagnostic_phase1` and `diagnostic_phase2` rows for
  fast-mode tuning.
- `--diagnose-optimal`: print `diagnostic_optimal_bounds` rows for optimal-mode
  lower-bound tuning.
- `--report-symmetry`: print symmetry infrastructure sizes and then exit.
- `--report-cache`: print pruning table cache files and byte totals, then exit.
- `--report-memory`: print pruning table RAM estimates by solver profile, then
  exit.

Random generation is deterministic for a given `--random-seed`,
`--random-count`, `--random-depth`, and `--random-start-index`. Consecutive
random moves never use the same face.

Symmetry infrastructure report:

```sh
./build/rubik-bench --report-symmetry
```

Cache report:

```sh
./build/rubik-bench --report-cache
```

Memory report:

```sh
./build/rubik-bench --report-memory
```

Current warm development cache report:

```text
cache_summary,files,17
cache_summary,total_bytes,206860687
```

Current pruning-table memory report:

```text
memory_summary,embedded_optimal,tables,9
memory_summary,embedded_optimal,total_bytes,22123535
memory_summary,default_optimal,tables,14
memory_summary,default_optimal,total_bytes,205322495
memory_summary,performance_optimal,tables,15
memory_summary,performance_optimal,total_bytes,346456895
memory_summary,fast_two_phase,tables,8
memory_summary,fast_two_phase,total_bytes,3638975
```

The memory report counts table payload bytes loaded by each profile. It is not
full process RSS, and it excludes allocator overhead and code/data outside the
pruning tables.

Current report:

```text
symmetry,rotation_count,24
symmetry,ud_slice_preserving_count,8
symmetry_coordinate,corner_orientation,2187,2187,209952,111,19.70,19683
symmetry_coordinate,edge_orientation,2048,2048,196608,114,17.96,18432
symmetry_coordinate,slice_edges,495,495,47520,,,
symmetry_combined_coordinate,corner_edge_orientation,4478976,187350,23.91,41060184
symmetry_combined_coordinate,corner_orientation_slice_edges,1082565,135576,7.98,10285389
symmetry_combined_coordinate,edge_orientation_slice_edges,1013760,127326,7.96,9633144
symmetry_pruning,corner_orientation,111,111,4,2187,2187,6
symmetry_pruning,edge_orientation,114,114,6,2048,2048,7
symmetry_combined_pruning,corner_edge_orientation,187350,187350,7,4478976,4478976
symmetry_combined_pruning,corner_orientation_slice_edges,135576,135576,9,1082565,1082565
symmetry_combined_pruning,edge_orientation_slice_edges,127326,127326,8,1013760,1013760
```

Experimental symmetry lower-bound flag:

```sh
RUBIK_EXPERIMENTAL_SYMMETRY_BOUNDS=1 ./build/rubik-bench --mode optimal --timeout-ms 30000 --max-depth 13 --max-case-depth 13
```

Current A/B result on the deterministic depth-13 suite:

```text
baseline total_nodes_expanded: 68,991
experimental total_nodes_expanded: 68,991
```

The reduced symmetry bounds are therefore not enabled by default. They are kept
as validation infrastructure for larger symmetry-reduced combined coordinates.

Experimental three-direction phase-1 flag:

```sh
RUBIK_EXPERIMENTAL_THREE_PHASE1_BOUNDS=1 ./build/rubik-bench --mode optimal --timeout-ms 30000 --max-depth 10 --max-case-depth 10
```

Earlier cubie-transform A/B result on the deterministic depth-10 suite:

```text
baseline total_nodes_expanded: 410
experimental total_nodes_expanded: 383
baseline depth_10 elapsed_ms: 4
experimental depth_10 elapsed_ms: 112
```

The first implementation reduced nodes slightly but recomputed transformed
cubie coordinates per node and was too expensive.

Incremental three-direction phase-1 A/B result:

```text
depth-10 baseline total_nodes_expanded: 410
depth-10 experimental total_nodes_expanded: 383
depth-10 baseline depth_10 elapsed_ms: 5
depth-10 experimental depth_10 elapsed_ms: 5

depth-13 baseline total_nodes_expanded: 68,991
depth-13 experimental total_nodes_expanded: 61,770
depth-13 baseline depth_13 elapsed_ms: 984
depth-13 experimental depth_13 elapsed_ms: 1007
```

The incremental version removes the large overhead and reduces depth-13 nodes by
about 10%. It is still off by default because the wall time is not yet better;
the next target is reducing per-node overhead.

After isolating default and experimental node state, the baseline no longer
updates the three phase-1 direction coordinates unless the flag is enabled:

```text
depth-13 baseline total_nodes_expanded: 68,991
depth-13 experimental total_nodes_expanded: 61,770
depth-13 baseline depth_13 elapsed_ms: 864
depth-13 experimental depth_13 elapsed_ms: 1019
```

The experiment remains useful for node reduction, but it is not ready to become
the default bound.

After removing the duplicated normal U/D phase-1 direction from the experimental
state:

```text
depth-13 baseline total_nodes_expanded: 68,991
depth-13 experimental total_nodes_expanded: 61,770
depth-13 baseline depth_13 elapsed_ms: 893
depth-13 experimental depth_13 elapsed_ms: 1017
```

The node reduction is unchanged; wall time still does not justify enabling the
flag by default.

Lower-bound micro-benchmark:

```sh
RUBIK_TABLE_CACHE_DIR=/tmp/rubik_cube_library_phase2_cache \
./build/rubik-bench --benchmark-lower-bound --max-case-depth 13 --lower-bound-iterations 1000
```

Current A/B result for the public `Solver::lowerBound` path:

```text
baseline evaluations_per_ms: 73.89
three-phase evaluations_per_ms: 26.57
```

This measures the public lower-bound API, including root coordinate setup. It
confirms that the experimental path still carries meaningful overhead.

After removing duplicated normal-direction phase-1 work, the deterministic
depth-13 solve benchmark measured:

```text
baseline depth_13 elapsed_ms: 773
three-phase depth_13 elapsed_ms: 845
baseline total_nodes_expanded: 68,991
three-phase total_nodes_expanded: 61,770
```

After making the DFS evaluate the three-direction phase-1 bound lazily, the
deterministic depth-13 solve benchmark measured:

```text
baseline depth_13 elapsed_ms: 835
three-phase lazy depth_13 elapsed_ms: 840
baseline total_nodes_expanded: 68,991
three-phase lazy total_nodes_expanded: 61,770
```

The lazy check skips the experimental bound when the cheaper standard bound
already prunes a node. This brings the wall time close to baseline while keeping
the node reduction, but it is still not consistently faster enough to enable by
default.

After removing the repeated non-root DFS entry bound check, the deterministic
depth-13 solve benchmark measured:

```text
baseline depth_13 elapsed_ms: 848
three-phase lazy depth_13 elapsed_ms: 815
baseline total_nodes_expanded: 68,991
three-phase lazy total_nodes_expanded: 61,770
diagnostic_optimal_bounds depth_13: 0 cheap node prunes, 6 three-phase node checks,
692,154 cheap candidate prunes, 63,438 three-phase candidate checks,
6,735 three-phase candidate prunes
```

This is the first local measurement where the experimental three-direction
phase-1 bound is faster than the baseline on the deterministic depth-13 case.
It remains behind a flag until repeated and larger benchmark runs confirm the
gain.

Wider validation after adding `--diagnose-optimal`:

```text
deterministic depth-13 baseline elapsed_ms: 736
deterministic depth-13 three-phase elapsed_ms: 735
deterministic depth-13 baseline nodes_expanded: 63,401
deterministic depth-13 three-phase nodes_expanded: 56,647

deterministic depth-14 baseline timeout nodes_expanded: 2,555,802
deterministic depth-14 three-phase timeout nodes_expanded: 2,338,442

random depth-12 x8 baseline total_elapsed_ms: 1,295
random depth-12 x8 three-phase total_elapsed_ms: 1,298
random depth-12 x8 baseline total_nodes_expanded: 57,710
random depth-12 x8 three-phase total_nodes_expanded: 53,144
```

The experimental bound consistently reduces expanded nodes by about 8-10% on
these samples. Wall time is still workload-sensitive, so it remains off by
default while the next optimization targets the per-candidate bound cost.

After replacing repeated phase-1 bound helper overhead with cached table
references and manual max operations, the random depth-12 sample measured:

```text
random depth-12 x8 baseline total_elapsed_ms: 1,348
random depth-12 x8 three-phase total_elapsed_ms: 1,232
random depth-12 x8 baseline total_nodes_expanded: 57,710
random depth-12 x8 three-phase total_nodes_expanded: 53,144
```

The depth-13 deterministic comparison from the same build remained effectively
neutral:

```text
deterministic depth-13 baseline elapsed_ms: 817
deterministic depth-13 three-phase elapsed_ms: 815
```

The dedicated A/B runner was then added:

```sh
scripts/run_optimal_ab.sh --case-set random --random-count 8 --random-depth 12 --random-seed 20260525 --max-depth 12 --timeout-ms 5000 --repetitions 2
```

Current A/B aggregate:

```text
baseline avg_total_elapsed_ms: 1,239
three-phase avg_total_elapsed_ms: 1,151
baseline avg_total_nodes_expanded: 57,710
three-phase avg_total_nodes_expanded: 53,144
```

Longer A/B runs:

```sh
scripts/run_optimal_ab.sh --case-set random --random-count 8 --random-depth 12 --random-seed 20260525 --max-depth 12 --timeout-ms 5000 --repetitions 5
scripts/run_optimal_ab.sh --case-set random --random-count 20 --random-depth 12 --random-seed 20260525 --max-depth 12 --timeout-ms 5000 --repetitions 3
scripts/run_optimal_ab.sh --case-set deterministic --max-case-depth 13 --max-depth 13 --timeout-ms 30000 --repetitions 3
scripts/run_optimal_ab.sh --case-set deterministic --max-case-depth 14 --max-depth 14 --timeout-ms 30000 --repetitions 2
```

Current longer A/B aggregate:

```text
random depth-12 x8 baseline avg_total_elapsed_ms: 1,198.00
random depth-12 x8 three-phase avg_total_elapsed_ms: 1,158.80
random depth-12 x8 baseline avg_total_nodes_expanded: 57,710
random depth-12 x8 three-phase avg_total_nodes_expanded: 53,144

random depth-12 x20 baseline avg_total_elapsed_ms: 5,234.00
random depth-12 x20 three-phase avg_total_elapsed_ms: 5,082.00
random depth-12 x20 baseline avg_total_nodes_expanded: 447,954
random depth-12 x20 three-phase avg_total_nodes_expanded: 402,849

deterministic depth-13 baseline avg_total_elapsed_ms: 1,383.33
deterministic depth-13 three-phase avg_total_elapsed_ms: 1,289.67
deterministic depth-13 baseline avg_total_nodes_expanded: 68,991
deterministic depth-13 three-phase avg_total_nodes_expanded: 61,770

deterministic depth-14 baseline avg_total_elapsed_ms: 31,419.50
deterministic depth-14 three-phase avg_total_elapsed_ms: 31,248.00
deterministic depth-14 baseline avg_total_nodes_expanded: 2,965,384.50
deterministic depth-14 three-phase avg_total_nodes_expanded: 2,738,006.00
deterministic depth-14 solved: 13/14 for both variants
```

These repeated warm-cache runs strengthen the case for promoting the
three-direction phase-1 bound. Depth 14 still times out at 30 seconds, but the
experimental bound expands fewer nodes in the same budget. Larger random
optimal samples still need confirmation before changing the default.

The output is CSV-like text with one row per case plus aggregate report rows.
When using `scripts/run_benchmark_suite.sh`, each output file also includes
`benchmark,...` rows for suite metadata and whole-command wall time.
Direct `rubik-bench` output includes `benchmark,version`, `benchmark,mode`,
`benchmark,profile`, `benchmark,three_phase1_policy`, and
`benchmark,three_phase1_effective` rows so old-baseline and default-candidate
runs are distinguishable in saved CSV files. It also performs an explicit
profile/table warm-up before case timing and reports the warm-up separately.

Important columns:

- `case_depth`: length of the deterministic scramble used by the benchmark.
- `status`: solver result status.
- `optimal`: whether the solution is proven minimal.
- `moves`: returned solution length.
- `initial_lower_bound`: root lower bound before solving.
- `elapsed_ms`: wall-clock solve time.
- `nodes_expanded`: number of expanded search nodes.
- `nodes_by_depth`: `|`-separated expansion counts per attempted depth/budget.
- `solution`: returned move sequence.

Additional report rows:

- `benchmark,name,N`: benchmark suite case name.
- `benchmark,cache_mode,warm|cold`
- `benchmark,cache_dir,PATH`
- `benchmark,suite_profile,default|embedded|performance`
- `benchmark,version,N`
- `benchmark,mode,optimal|fast`
- `benchmark,profile,default|embedded|performance|large-local`
- `benchmark,three_phase1_policy,default_enabled|default_disabled|forced_by_env|disabled_by_env`
- `benchmark,three_phase1_effective,true|false`
- `benchmark,warmup_table_payload_bytes,N`: logical pruning-table payload warmed
  before case timing.
- `benchmark,warmup_elapsed_ms,N`: elapsed time spent loading or generating
  active tables before case timing.
- `benchmark,wall_elapsed_ms,N`: whole command time measured by the wrapper.
- `profile,mode,...`: profile-smoke summary rows in
  `*_profile_smoke_summary.csv` and profile-realistic summary rows in
  `*_profile_realistic_summary.csv`.
- `summary,total_cases,N`
- `summary,solved,N`
- `summary,failed,N`
- `summary,total_elapsed_ms,N`
- `summary,total_nodes_expanded,N`
- `summary,average_elapsed_ms,N`
- `summary,average_nodes_expanded,N`
- `summary,p50_elapsed_ms,N`
- `summary,p90_elapsed_ms,N`
- `summary,p95_elapsed_ms,N`
- `summary,p99_elapsed_ms,N`
- `summary,max_elapsed_ms,N`
- `failure,...`: one row per failed case.
- `slowest,...`: slowest cases sorted by elapsed time.
- `diagnostic_phase1,...`: phase-1 candidate search timing and node counts.
- `diagnostic_phase2,...`: phase-2 timing and node counts per phase-1 candidate.
- `diagnostic_optimal_bounds,...`: optimal solver lower-bound pruning counters.

## Current Development Baseline

Warm-cache default optimal mode solves deterministic cases through depth 13
within the 30 second per-case target on the development machine. The current
default optimal policy includes three-direction phase-1 bounds for every
`SolveMode::Optimal` profile.

Repeated A/B runs measured deterministic depth 13 with:

```text
summary,total_cases,13
summary,solved,13
baseline avg_total_elapsed_ms: 1,383.33
three-phase default avg_total_elapsed_ms: 1,289.67
baseline total_nodes_expanded: 68,991
three-phase default total_nodes_expanded: 61,770
```

Warm-cache deterministic depth 14 with the same 30 second per-case timeout:

```text
baseline avg_total_elapsed_ms: 31,419.50
three-phase default avg_total_elapsed_ms: 31,248.00
baseline avg_total_nodes_expanded: 2,965,384.50
three-phase default avg_total_nodes_expanded: 2,738,006.00
solved: 13/14 for both variants
```

Depth 14 still timed out at 30 seconds, so depth 14+ still needs stronger
admissible pruning.

These figures are local development numbers, not Raspberry Pi numbers.

Small warm-cache random smoke test:

```text
command: ./build/rubik-bench --mode fast --timeout-ms 5000 --max-depth 24 --case-set random --random-count 5 --random-depth 20 --random-seed 12345
summary,total_cases,5
summary,solved,5
summary,total_elapsed_ms,10096
summary,total_nodes_expanded,4522744
```

This is not yet a statistically meaningful benchmark; it is a reproducibility
check for the random benchmark path.

Larger warm-cache random baseline after adaptive fast-mode tuning:

```text
command: ./build/rubik-bench --mode fast --timeout-ms 5000 --max-depth 24 --case-set random --random-count 100 --random-depth 20 --random-seed 12345 --slowest-count 10
summary,total_cases,100
summary,solved,100
summary,failed,0
summary,total_elapsed_ms,49268
summary,total_nodes_expanded,21695139
summary,average_elapsed_ms,492.68
summary,average_nodes_expanded,216951.39
summary,max_elapsed_ms,2374
```

Average solve time was about 0.49 seconds per scramble. The detailed report is
in [Random Fast Benchmark - 2026-05-25](benchmark-random-fast-2026-05-25.md).

The current optimized desktop report with 1000 fast random cases, random optimal
depth-12/depth-13 cases, and a small optimal depth-20 timeout probe is in
[Realistic Benchmark Report - 2026-05-25](benchmark-realistic-2026-05-25.md).

## Benchmark Rules

Use warm-cache and cold-cache runs separately:

- cold-cache benchmarks include table generation and disk writes;
- warm-cache benchmarks measure solving after tables already exist.

Always record:

- commit or source snapshot identifier;
- compiler and build type;
- CPU model;
- RAM;
- OS;
- cache directory location;
- command line;
- case set, random depth, random count, and seed when using generated cases;
- slowest cases and failure rows;
- full benchmark output.

The Raspberry Pi target needs its own benchmark report because table access and
memory latency will differ from desktop behavior.
