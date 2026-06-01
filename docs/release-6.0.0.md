# Release Checklist - 6.0.0

V6 is a local optimal-latency release. It promotes measured conservative
root-ordering policy, keeps benchmark replay tooling available, and preserves
the certified optimality contract.

## Status

| Area | Status | Notes |
| --- | --- | --- |
| Certified optimal solving | Unchanged | `SolveStatus::Optimal` remains a proven-minimal HTM result. |
| V6 conservative root ordering | Verified locally | Rollback replays support keeping the measured policy enabled. |
| Benchmark tooling | Verified locally | V6 transition-corpus extraction and replay aggregation are covered by CTest fixtures. |
| Public docs | Pending final verification | README, API notes, roadmap, benchmark notes, and release docs reference V6. |
| Release gates | Pending final verification | Full CTest, release check, archive check, and public-doc guardrails must pass after the version bump. |
| Hardware claims | Not release-claimed | Raspberry Pi, Jetson, Orin, GPU, and cloud claims require direct measurements before publication. |

## Required Verification

```sh
ctest --test-dir out/release-native-lto --output-on-failure
scripts/release_check.sh --profile standard
scripts/check_release_archive.sh
ctest --test-dir out/release-native-lto -R "public_docs_current_version|public_docs_no_unverified_hardware_estimates|public_examples_current_api|check_release_archive_versioned_docs" --output-on-failure
```

Run benchmark gates only for benchmark claims that will be published in the
release notes.

## Benchmark Evidence

The release uses local-only V6 benchmark evidence:

- `docs/v6-optimal-latency-pass75-78-2026-06-01.md`
- `docs/v6-optimal-latency-pass80-82-2026-06-01.md`
- `docs/v6-optimal-latency-pass83-85-2026-06-01.md`

No Raspberry Pi, Jetson, Orin, GPU, or cloud benchmark claim is published for
V6.

## Verification Evidence

Final evidence is pending in this working tree. Record only commands actually
run for this release candidate.
