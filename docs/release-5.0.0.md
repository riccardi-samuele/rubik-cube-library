# Release Checklist - 5.0.0

V5 is a public usability release. It modernizes examples, aligns public docs,
and adds release gates that prevent stale public examples and stale public
version references.

## Status

| Area | Status | Notes |
| --- | --- | --- |
| Certified optimal solving | Unchanged | `SolveStatus::Optimal` remains a proven-minimal HTM result. |
| Public examples | Verified | Examples use current public API patterns. |
| Public docs | Verified | README, API notes, roadmap, and release docs reference V5. |
| Release gates | Verified | Docs and examples gates are part of CTest. |
| Hardware claims | Not release-claimed | Raspberry Pi, Jetson Nano, and Jetson Orin claims require physical hardware measurements. |

## Required Verification

```sh
ctest --test-dir out/release-native-lto --output-on-failure
scripts/release_check.sh --profile quick
scripts/release_check.sh --profile standard
```

Run benchmark gates only if solver behavior changes. V5 should not publish new
performance claims unless the benchmark data is generated during this release.

## Verification Evidence

Local release validation on 2026-05-28:

- `ctest --test-dir out/release-native-lto --output-on-failure`: 85/85 passed.
- `scripts/release_check.sh --profile quick`: passed, including
  `release-native-lto`, install consumer, source archive validation, archive
  rebuild, and archive tests.
- `scripts/release_check.sh --profile standard`: passed, including `release`,
  `release-native-lto`, install consumer, source archive validation, archive
  rebuild, and archive tests.
- Public stale-marker audit found no stale current-version references in public
  docs, release scripts, tests, or CMake metadata.
