# Rubik Cube Library 4.0.0

V4 is a local CPU optimal-latency release focused on difficult
`SolveMode::Optimal` cases.

## Highlights

- Promoted adaptive deep-split scheduling for local large-table optimal solves.
- Preserved the certified minimum-move HTM contract for `SolveStatus::Optimal`.
- Added three-way V4 benchmark tooling for baseline, unconditional deep split,
  and adaptive deep split.
- Added adaptive scheduler diagnostics in the root ordering profile.

## Local Benchmark Result

Measured on the V4 adaptive corpus:

- average solver time: `3921 ms -> 3533 ms` (`-9.90%`)
- max solver time: `9457 ms -> 7656 ms` (`-1801 ms`)
- all adaptive rows: `Optimal,true`

Benchmark details are recorded in
`docs/v4-adaptive-deep-split-results-2026-05-28.md`.

## Validation

Passed locally:

```bash
ctest --test-dir out/release-native-lto --output-on-failure
scripts/release_check.sh --profile quick
```

No Raspberry Pi, Jetson, Orin, GPU, cloud, or other external hardware
performance claims are included in this release.
