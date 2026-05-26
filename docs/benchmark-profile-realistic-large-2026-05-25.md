# Profile Realistic Large Benchmark - 2026-05-25

Host:

- CPU: AMD Ryzen 9 8940HX, 16 cores / 32 threads
- Build preset: `release-native-lto`
- Cache: warm, `RUBIK_TABLE_CACHE_DIR=/tmp/rubik_cube_library_profile_realistic_cache`
- Seed: `12345`
- Suite: `profile-realistic`

Command:

```sh
cmake --build out/release-native-lto --target rubik-benchmark-profile-realistic
```

This is the refreshed larger profile run after the embedded fast
candidate-ordering improvement and the embedded optimal three-direction
phase-1 lower-bound enablement. It keeps the same profile mix and sample counts
as the calibrated realistic suite.

## Summary

| Profile | Mode | Benchmark | Cases | Solved | Failed | Avg ms | P50 | P90 | P95 | P99 | Max | Nodes | Warmup ms |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Embedded | Fast | random depth-20 | 100 | 100 | 0 | 105.16 | 98 | 193 | 211 | 291 | 294 | 28,867,196 | 12 |
| Embedded | Optimal | random depth-12 | 20 | 20 | 0 | 241.20 | 135 | 615 | 620 | 702 | 702 | 2,462,529 | 11 |
| Embedded | Optimal | random depth-13 | 10 | 10 | 0 | 2,740.10 | 961 | 5,346 | 9,776 | 9,776 | 9,776 | 14,207,363 | 12 |
| Default | Fast | random depth-20 | 100 | 100 | 0 | 106.18 | 84 | 186 | 259 | 571 | 724 | 29,577,608 | 95 |
| Default | Optimal | random depth-12 | 20 | 20 | 0 | 82.75 | 55 | 192 | 212 | 228 | 228 | 502,579 | 97 |
| Default | Optimal | random depth-13 | 10 | 10 | 0 | 974.60 | 331 | 1,768 | 3,803 | 3,803 | 3,803 | 3,080,077 | 96 |
| Performance | Fast | random depth-20 | 100 | 100 | 0 | 206.90 | 175 | 342 | 428 | 526 | 587 | 58,382,339 | 161 |
| Performance | Optimal | random depth-12 | 20 | 20 | 0 | 81.20 | 50 | 190 | 210 | 214 | 214 | 484,147 | 162 |
| Performance | Optimal | random depth-13 | 10 | 10 | 0 | 1,015.40 | 347 | 1,830 | 3,996 | 3,996 | 3,996 | 2,993,467 | 164 |

## Interpretation

The embedded fast candidate-ordering improvement held up on the larger sample:
100/100 random depth-20 cases solved, with 105.16 ms average, 98 ms p50,
193 ms p90, 211 ms p95, 291 ms p99, and 294 ms max on this desktop. This
replaces the earlier 572 ms p99 and 655 ms max profile-realistic result.

`Embedded/Optimal` remains the limiting local profile, but the three-direction
phase-1 lower bound cut the sampled depth-13 tail substantially: 2.74 seconds
average, 0.96 seconds p50, 5.35 seconds p90, and 9.78 seconds max on this host.
This is inside the tightened 12 second desktop gate, but Raspberry Pi claims
still require real hardware measurements.

`Default` and `Performance` optimal results are again close. On this run,
`Default` is slightly faster in wall-clock at depth-13 while `Performance`
expands fewer nodes. That suggests the extra performance-profile tables are not
yet buying enough wall-clock speed on this desktop to make it the default
recommendation.

## Next Benchmark Work

- Keep the embedded multiseed gate as the primary `Embedded/Fast` regression
  check and rerun profile-realistic after solver policy changes.
- The `Embedded/Fast` profile-realistic gate is now tightened to p95 `350 ms`,
  p99 `500 ms`, and max `700 ms`; see
  `docs/benchmark-gate-calibration-2026-05-25.md`.
- Run the same command on Raspberry Pi and Jetson-class hardware before making
  embedded performance claims.
