# Profile Realistic Benchmark - 2026-05-25

Host:

- CPU: AMD Ryzen 9 8940HX, 16 cores / 32 threads
- Build preset: `release-native-lto`
- Cache: warm, `RUBIK_TABLE_CACHE_DIR=/tmp/rubik_cube_library_profile_realistic_tail_20260525_cache`
- Seed: `12345`
- Suite: `profile-realistic`

Command:

```sh
scripts/run_benchmark_suite.sh --suite profile-realistic \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_profile_realistic_tail_20260525_cache \
  --output-dir out/release-native-lto/benchmark-results/profile-realistic-tail-2026-05-25 \
  --fast-timeout-ms 5000 \
  --optimal-timeout-ms 30000 \
  --realistic-fast-count 20 \
  --realistic-opt12-count 10 \
  --realistic-opt13-count 5
```

This run includes the embedded fast tail fallback policy: `Embedded/Fast` keeps
the lightweight 4-candidate attempt first, then tries a wider 16-candidate tail
only after the normal attempt fails.

## Summary

| Profile | Mode | Benchmark | Cases | Solved | Failed | Avg ms | Max ms | Nodes | Warmup ms |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Embedded | Fast | random depth-20 | 20 | 20 | 0 | 156.70 | 660 | 8,660,217 | 2,214 |
| Embedded | Optimal | random depth-12 | 10 | 10 | 0 | 374.30 | 1,049 | 2,357,283 | 9 |
| Embedded | Optimal | random depth-13 | 5 | 5 | 0 | 7,747.00 | 15,493 | 24,369,365 | 10 |
| Default | Fast | random depth-20 | 20 | 20 | 0 | 153.60 | 664 | 8,330,949 | 27,868 |
| Default | Optimal | random depth-12 | 10 | 10 | 0 | 73.30 | 217 | 214,379 | 91 |
| Default | Optimal | random depth-13 | 5 | 5 | 0 | 1,643.20 | 3,875 | 2,539,189 | 92 |
| Performance | Fast | random depth-20 | 20 | 20 | 0 | 237.45 | 539 | 12,705,677 | 3,059 |
| Performance | Optimal | random depth-12 | 10 | 10 | 0 | 74.10 | 214 | 204,910 | 154 |
| Performance | Optimal | random depth-13 | 5 | 5 | 0 | 1,650.20 | 3,884 | 2,466,896 | 153 |

## Interpretation

`Embedded` is currently much weaker for optimal depth-13. It solved all 5
sampled depth-13 cases, but average solve time was about 7.7 seconds on the
desktop and the slowest case was about 15.5 seconds. A Raspberry Pi class device
will need real hardware measurement before we can claim a 30 second target for
similar cases.

`Default` and `Performance` are very close in optimal mode for this sample.
`Performance` expanded slightly fewer nodes, but it is not a decisive wall-clock
win here. This supports keeping `Default` as the normal public profile for now.

`Fast` mode is now robust for this reduced `Embedded` random depth-20 sample:
the tail fallback improved the known failing sample from 18/20 solved to 20/20
solved. The cost is modest on this host: average `Embedded/Fast` time increased
from the original 119.80 ms failed baseline to 156.70 ms with no failures.

Warm-up is now separated from solve timing. The large `Default/Fast` warm-up in
this run reflects table loading/generation, not per-case solve cost.

## Next Benchmark Work

- The larger follow-up suite is documented in
  `docs/benchmark-profile-realistic-large-2026-05-25.md`.
- Replay `Embedded/Fast` depth-20 failures with
  `scripts/run_benchmark_suite.sh --suite embedded-fast-failures`. The current
  tail fallback solves the original failures; keep the replay as a regression
  check.
- Run this suite on actual Raspberry Pi and Jetson-class hardware before making
  hardware claims.

## Original Embedded Fast Baseline

Before the selective tail fallback, this same reduced suite had
`Embedded/Fast` random depth-20 at 18/20 solved, 119.80 ms average, 321 ms max,
and 5,618,361 total expanded nodes. The two failing cases are preserved in
`docs/embedded-fast-failures-2026-05-25.md`.
