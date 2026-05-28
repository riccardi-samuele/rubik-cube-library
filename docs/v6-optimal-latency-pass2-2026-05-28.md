# V6 Optimal Latency Pass 2 - 2026-05-28

This document records the second V6 local optimal-latency optimization pass.
The solver contract is unchanged: `SolveStatus::Optimal` still means a proven
minimum-length HTM solution for the requested options.

## Change

Auto optimal no longer enables strong-bound move ordering for stable lower-bound
9 root profiles with a mid-sized strong-bound minimum group:

- `initial_lower_bound=9`;
- `first_diff=0`;
- `strong_min_count` from 4 to 7.

This is a narrow rule. The measured corpus showed that these cases are better
served by the V6 reverse root tie-break from pass 1 while preserving base-bound
move ordering. Other lower-bound 9 profiles continue to use auto strong-bound
ordering.

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
  --output-dir out/release-native-lto/benchmark-results/v6-pass2-optimized-tail-baseline \
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

Baseline source: `docs/v6-optimal-latency-pass1-2026-05-28.md`.

| Metric | Pass 1 | Pass 2 | Delta |
| --- | ---: | ---: | ---: |
| Cases | 43 | 43 | 0 |
| Solved | 43 | 43 | 0 |
| Failed | 0 | 0 | 0 |
| Total solver ms | 51862 | 52030 | +168 |
| Total nodes | 180121743 | 169550565 | -10571178 |
| p95 solver ms | 3951 | 3591 | -360 |
| Max solver ms | 6609 | 6963 | +354 |
| Max wall ms | 7252 | 7638 | +386 |

## Suite Results

| Suite | Cases | Solved | Failed | Total solver ms | Total nodes | Max solver ms | Max wall ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| optimal-auto-tail | 7 | 7 | 0 | 21786 | 71739820 | 6748 | 7375 |
| optimal-auto-hardening | 36 | 36 | 0 | 30244 | 97810745 | 6963 | 7638 |
| combined | 43 | 43 | 0 | 52030 | 169550565 | 6963 | 7638 |

## Slowest Rows After Pass 2

| Suite | Benchmark | Solver ms | Wall ms | Nodes |
| --- | --- | ---: | ---: | ---: |
| optimal-auto-hardening | `random_seed_987654321_depth_15_count_1` | 6963 | 7638 | 23893344 |
| optimal-auto-tail | `random_seed_987654321_depth_15_count_1` | 6748 | 7375 | 23872868 |
| optimal-auto-hardening | `random_seed_42_depth_15_count_1` | 4130 | 4769 | 13340354 |
| optimal-auto-tail | `random_seed_1009_depth_15_count_1` | 3591 | 4234 | 11471886 |
| optimal-auto-tail | `random_seed_555_depth_15_count_1` | 3225 | 3859 | 11632691 |
| optimal-auto-hardening | `random_seed_424242_depth_15_count_1` | 3018 | 3679 | 10312296 |
| optimal-auto-tail | `random_seed_424242_depth_15_count_1` | 2944 | 3582 | 10346055 |
| optimal-auto-hardening | `random_seed_8675309_depth_15_count_1` | 2868 | 3560 | 9406551 |

## Reading

Pass 2 improves the measured p95 and removes about 10.6 million expanded nodes,
mainly by keeping the `random_seed_1009_depth_15_count_1` profile on base-bound
move ordering. The measured max did not improve in this run; the remaining V6
tail target is still `random_seed_987654321_depth_15_count_1`.
