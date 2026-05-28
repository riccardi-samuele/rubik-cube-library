# Rubik Cube Library 4.0.0

## Summary

Version 4.0.0 is a local CPU optimal-latency release. It improves difficult
`SolveMode::Optimal` cases by promoting an adaptive deep-split scheduler policy
for local large-table optimal solves while preserving the certified minimum-move
HTM contract.

## Performance

Measured locally on the V4 adaptive deep-split corpus:

- Baseline average solver time: `3921` ms
- Adaptive average solver time: `3533` ms
- Average reduction: `388` ms (`9.90%`)
- Baseline max solver time: `9457` ms
- Adaptive max solver time: `7656` ms
- Max reduction: `1801` ms
- Every adaptive benchmark row solved as `Optimal,true`

Benchmark scope:

- runner: `scripts/run_v4_deep_split_ab.sh`
- output directory: `out/release-native-lto/benchmark-results/v4-adaptive-deep-split-tuned`
- variants: baseline, unconditional deep split, adaptive deep split
- threads: `0` auto
- memory limit: `2048` MB
- timeout: `30000` ms

These are local desktop measurements only.

## Changes

- Promotes adaptive deep-split scheduling for local large-table optimal solves.
- Keeps unconditional deep splitting rejected because it regressed average
  latency in the measured corpus.
- Adds three-way V4 benchmark tooling for baseline, deep-split, and adaptive
  variants.
- Extends V4 comparison output with max elapsed fields.
- Adds diagnostics for adaptive scheduler decisions and reasons.

## Compatibility

The public optimality contract is unchanged:

- `SolveStatus::Optimal` still means a certified minimum-move HTM solution.
- Existing explicit non-large profiles keep the existing scheduler unless an
  experimental flag is set.
- Experimental flags remain available for benchmark research:
  `RUBIK_EXPERIMENTAL_DEEP_ROOT_SPLIT=1` and
  `RUBIK_EXPERIMENTAL_ADAPTIVE_DEEP_SPLIT=1`.

## Validation

Commands run before preparing these notes:

```bash
ctest --test-dir out/release-native-lto --output-on-failure
scripts/release_check.sh --profile quick
```

Both passed locally.

## Hardware Claims

This release note does not include Raspberry Pi, Jetson, Orin, GPU, cloud, or
other external hardware performance claims. Those measurements should be added
only after direct tests on those systems.
