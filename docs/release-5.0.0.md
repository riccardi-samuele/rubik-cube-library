# Release Checklist - 5.0.0

V5 is a public usability release. It modernizes examples, aligns public docs,
and adds release gates that prevent stale public examples and stale public
version references.

## Status

| Area | Status | Notes |
| --- | --- | --- |
| Certified optimal solving | Unchanged | `SolveStatus::Optimal` remains a proven-minimal HTM result. |
| Public examples | Ready for verification | Examples use current public API patterns. |
| Public docs | Ready for verification | README, API notes, roadmap, and release docs reference V5. |
| Release gates | Ready for verification | Docs and examples gates are part of CTest. |
| Hardware claims | Not release-claimed | Raspberry Pi, Jetson Nano, and Jetson Orin claims require physical hardware measurements. |

## Required Verification

```sh
ctest --test-dir out/release-native-lto --output-on-failure
scripts/release_check.sh --profile quick
scripts/release_check.sh --profile standard
```

Run benchmark gates only if solver behavior changes. V5 should not publish new
performance claims unless the benchmark data is generated during this release.
