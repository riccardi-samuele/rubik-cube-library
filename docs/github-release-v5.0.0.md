# GitHub Release Draft - v5.0.0

V5 is a public usability release for Rubik Cube Library.

## Highlights

- Modern certified optimal example using `SolveProfile::Auto`,
  `CachePolicy::Auto`, and automatic thread planning.
- Public cache setup example using `prepareCache()`.
- Clearer experimental fast-mode example.
- Release gates that reject stale public examples and stale public docs.

## Compatibility

The V4 certified optimality contract is unchanged. `SolveStatus::Optimal`
continues to mean the returned HTM solution is proven minimal for the requested
options.

## Hardware Claims

No Raspberry Pi, Jetson Nano, or Jetson Orin latency claims are included in this
release. Hardware-specific claims require measurements on the real target
devices.

## Verification

Before publishing, record the exact results of:

```sh
ctest --test-dir out/release-native-lto --output-on-failure
scripts/release_check.sh --profile quick
scripts/release_check.sh --profile standard
```
