# Optimal Stress Corner-State Experiment - 2026-05-25

Host:

- CPU: AMD Ryzen 9 8940HX, 16 cores / 32 threads
- Build preset: `release-native-lto`
- Cache: warm, `RUBIK_TABLE_CACHE_DIR=/tmp/rubik_cube_library_corner_state_sweep_cache`
- Environment: `RUBIK_EXPERIMENTAL_CORNER_STATE_BOUNDS=1`
- Suite: `optimal-stress`
- Seeds: `12345`, `20260525`, `42`
- Timeout: `30000 ms` per case

Command:

```sh
RUBIK_EXPERIMENTAL_CORNER_STATE_BOUNDS=1 scripts/run_benchmark_suite.sh \
  --suite optimal-stress \
  --cache-mode warm \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_corner_state_sweep_cache \
  --output-dir out/release-native-lto/benchmark-results/optimal-stress-corner-state \
  --seeds 12345,20260525,42 \
  --realistic-opt13-count 10
```

## Summary

| Profile | Seed | Cases | Solved | Failed | P50 | P95 | Max | Nodes | Payload |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Embedded | 12345 | 10 | 10 | 0 | 205 | 2,572 | 2,572 | 2,512,336 | 110,303,375 |
| Embedded | 20260525 | 10 | 10 | 0 | 21 | 2,134 | 2,134 | 1,263,230 | 110,303,375 |
| Embedded | 42 | 10 | 10 | 0 | 69 | 1,282 | 1,282 | 1,422,803 | 110,303,375 |
| Default | 12345 | 10 | 10 | 0 | 110 | 1,097 | 1,097 | 981,256 | 293,502,335 |
| Default | 20260525 | 10 | 10 | 0 | 13 | 1,147 | 1,147 | 565,066 | 293,502,335 |
| Default | 42 | 10 | 10 | 0 | 37 | 751 | 751 | 650,134 | 293,502,335 |
| Performance | 12345 | 10 | 10 | 0 | 114 | 1,103 | 1,103 | 969,506 | 434,636,735 |
| Performance | 20260525 | 10 | 10 | 0 | 13 | 1,165 | 1,165 | 550,658 | 434,636,735 |
| Performance | 42 | 10 | 10 | 0 | 36 | 777 | 777 | 639,945 | 434,636,735 |

## Gate Result

The existing optimal-stress gates all pass with the experimental table enabled:

- `Embedded/Optimal`: all three seed groups solve 10/10; worst P95/max is
  2,572 ms against the 12,000 ms gate.
- `Default/Optimal`: all three seed groups solve 10/10; worst P95/max is
  1,147 ms against the 5,500 ms gate.
- `Performance/Optimal`: all three seed groups solve 10/10; worst P95/max is
  1,165 ms against the 5,500 ms gate.

## Interpretation

The corner orientation + corner permutation table is strongly favorable for the
current depth-13 stress contract. It increases table payload by 88,179,840 bytes
but cuts the sampled tail latency enough that depth 13 is no longer the limiting
optimal-mode frontier on this desktop.

The promotion blocker is not depth 13 anymore. The remaining blocker is depth
15 tail behavior from the deep-probe suite, where selected cases still time out
at 30 seconds.
