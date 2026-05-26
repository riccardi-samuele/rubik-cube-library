# Embedded Fast Failure Diagnostics - 2026-05-25

These are the `Embedded/Fast` random depth-20 failure cases found in the
reduced profile-realistic benchmark. They are now replay targets for the
embedded tail fallback policy.

Command:

```sh
scripts/run_benchmark_suite.sh --suite embedded-fast-failures \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_embedded_fast_failure_test_cache \
  --output-dir out/release-native-lto/benchmark-results/embedded-fast-failures-test \
  --fast-timeout-ms 5000
```

The suite intentionally exits successfully because its purpose is diagnostic
replay.

## Cases

| Case | Scramble | Original Status | Tail Fallback Status |
| --- | --- | --- | --- |
| `random_12345_5` | `B R2 B2 U2 B' U2 D' L' U D R2 D2 U' D' F B' D' B R B'` | `DepthLimitExceeded` | `Found`, 659-664 ms observed |
| `random_12345_13` | `B' R U R F2 B R' F' L R B2 L2 R' U2 D B2 U' F2 D B` | `DepthLimitExceeded` | `Found`, 577-591 ms observed |

## Diagnostic Summary

Both cases reach phase 1 quickly, but the original embedded profile kept only 4
phase-1 candidates and the phase-2 completions failed within the remaining depth
budget. The beam fallback then reached `max-depth 24` without finding a
solution.

Observed diagnostic shape:

- `robust` attempt: 4 phase-1 candidates, all phase-2 candidates fail.
- `tail` attempt: 16 phase-1 candidates, later candidates solve both cases.
- final solve status after tail fallback: `Found`.

## Engineering Interpretation

The immediate weakness was not table generation or warm-up. It was the embedded
fast policy:

- candidate count is too low for these tails;
- phase-2 per-candidate search is too shallow under the selected candidates;
- beam fallback is not reliable enough as a final safety net at `max-depth 24`.

The current mitigation keeps the original lightweight embedded attempt first,
then runs a wider tail attempt with 16 candidates and 150 ms phase-2 budget only
when the first attempt fails. In the reduced profile-realistic run with 20
depth-20 cases and seed `12345`, `Embedded/Fast` improved from 18/20 solved to
20/20 solved. The clean run averaged 156.70 ms, with 660 ms max and 8,660,217
total expanded nodes.

## Replay Target

```sh
cmake --build --preset release-native-lto --target rubik-benchmark-embedded-fast-failures
```
