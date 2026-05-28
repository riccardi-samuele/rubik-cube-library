# V6 Optimal Latency Pass 1 - 2026-05-28

This document records the first V6 local optimal-latency optimization pass.
The solver contract is unchanged: `SolveStatus::Optimal` still means a proven
minimum-length HTM solution for the requested options.

## Change

The Auto optimal scheduler now uses two additional local decisions for
large-local tail cases:

- selective adaptive deep-root split for stable lower-bound 8 cases;
- selective reverse root tie-break for stable lower-bound 9 cases with a small
  strong-bound minimum group.

The rules are deliberately narrow. They are based on measured local tail
corpus behavior and avoid cases that the A/B run showed could regress under
forced deep-root split.

## Verification

Unit and contract checks:

```sh
cmake --build --preset release-native-lto --target rubik_tests
ctest --test-dir out/release-native-lto -R '^rubik_tests$' --output-on-failure
```

Result: passed.

Benchmark command:

```sh
scripts/run_v6_tail_baseline.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-optimized-tail-baseline \
  --tail-seeds 987654321,424242,1009,666,555,99,888 \
  --hardening-seeds 12345,20260525,42,314159,271828,987654321,7,99,123456789,424242,8675309,20240525 \
  --timeout-ms 30000 \
  --threads 0 \
  --max-memory-mb 2048 \
  --deep-opt14-count 2 \
  --deep-opt15-count 1 \
  --cache-mode reuse
```

These are local desktop measurements only. No external hardware, GPU, or cloud
latency claims are included.

## Corpus Comparison

Baseline source: `docs/v6-tail-corpus-2026-05-28.md`.

| Metric | V6 corpus baseline | Pass 1 | Delta |
| --- | ---: | ---: | ---: |
| Cases | 43 | 43 | 0 |
| Solved | 43 | 43 | 0 |
| Failed | 0 | 0 | 0 |
| Total solver ms | 58412 | 51862 | -6550 |
| Total nodes | 198714241 | 180121743 | -18592498 |
| Max solver ms | 7636 | 6609 | -1027 |
| Max wall ms | 8282 | 7252 | -1030 |

## Suite Results

| Suite | Cases | Solved | Failed | Total solver ms | Total nodes | Max solver ms | Max wall ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| optimal-auto-tail | 7 | 7 | 0 | 23286 | 83041598 | 6499 | 7247 |
| optimal-auto-hardening | 36 | 36 | 0 | 28576 | 97080145 | 6609 | 7252 |
| combined | 43 | 43 | 0 | 51862 | 180121743 | 6609 | 7252 |

## Slowest Rows After Pass 1

| Suite | Benchmark | Solver ms | Wall ms | Nodes |
| --- | --- | ---: | ---: | ---: |
| optimal-auto-hardening | `random_seed_987654321_depth_15_count_1` | 6609 | 7252 | 23908706 |
| optimal-auto-tail | `random_seed_987654321_depth_15_count_1` | 6499 | 7247 | 23856377 |
| optimal-auto-tail | `random_seed_1009_depth_15_count_1` | 5978 | 6614 | 22834907 |
| optimal-auto-hardening | `random_seed_42_depth_15_count_1` | 3951 | 4589 | 13302955 |
| optimal-auto-tail | `random_seed_555_depth_15_count_1` | 3051 | 3693 | 11596334 |
| optimal-auto-hardening | `random_seed_424242_depth_15_count_1` | 2929 | 3575 | 10310708 |
| optimal-auto-tail | `random_seed_424242_depth_15_count_1` | 2901 | 3541 | 10257894 |
| optimal-auto-hardening | `random_seed_8675309_depth_15_count_1` | 2435 | 3085 | 9439373 |

## Reading

Pass 1 improves the measured local tail without changing optimality semantics:

- the worst measured solver time drops by 1027 ms;
- the worst measured wall time drops by 1030 ms;
- total measured solver time across the corpus drops by 6550 ms;
- total expanded nodes drop by 18592498.

The remaining slowest row is still `random_seed_987654321_depth_15_count_1`.
The next V6 optimization pass should target that case specifically, because it
still expands about 23.9 million nodes after this pass.
