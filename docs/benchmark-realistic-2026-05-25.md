# Realistic Benchmark Report - 2026-05-25

Host:

- CPU: AMD Ryzen 9 8940HX, 16 cores / 32 threads
- Build preset: `release-native-lto`
- Cache: warm, `RUBIK_TABLE_CACHE_DIR=/tmp/rubik_cube_library_phase2_cache`
- Solver profile: `Default`
- Optimal policy: three-direction phase-1 bound enabled by default

Important limitation: `taskset -c 0` pins the process to one desktop CPU core. It
does not simulate Raspberry Pi memory bandwidth, cache hierarchy, CPU frequency,
thermal behavior, or ARM code generation.

## Results

| Benchmark | Cases | Solved | Failed | Avg ms | P50 ms | P90 ms | P95 ms | P99 ms | Max ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Fast random depth-20 | 1000 | 1000 | 0 | 101.18 | 87 | 187 | 214 | 328 | 1430 |
| Optimal deterministic depth-13 | 13 | 13 | 0 | 22.46 | 0 | 80 | 196 | 196 | 196 |
| Optimal random depth-12 | 100 | 100 | 0 | 78.29 | 56 | 185 | 244 | 377 | 435 |
| Optimal random depth-12, `taskset -c 0` | 100 | 100 | 0 | 77.49 | 54 | 190 | 232 | 370 | 433 |
| Optimal random depth-13 | 50 | 50 | 0 | 1013.64 | 402 | 2977 | 3301 | 5370 | 5370 |
| Optimal random depth-20, timeout 5s | 5 | 0 | 5 | 5000.00 | 5000 | 5000 | 5000 | 5000 | 5000 |

## Interpretation

Fast mode has acceptable desktop latency for depth-20 random scrambles, but it is
not optimal.

Optimal mode is usable for depth-12 and many depth-13 scrambles on this desktop,
but depth-13 already has multi-second tail latency. Random depth-20 optimal
solving is not currently production-ready with a 5 second timeout: all 5 sampled
cases timed out.

Do not derive Raspberry Pi or Jetson latency numbers from this run. Before
publishing hardware-specific performance claims, run the same CSV suite on the
actual target hardware and include thermal/governor settings.
