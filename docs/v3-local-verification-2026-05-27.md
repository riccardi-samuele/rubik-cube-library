# V3 Local Verification - 2026-05-27

This document records the local V3 verification run for the current
`main` branch. The run validates the repeatable benchmark gates used for
optimal-mode hardening and Auto profile tail coverage.

Commands:

```sh
cmake --build out/release-native-lto --target rubik-benchmark-embedded-multiseed
cmake --build out/release-native-lto --target rubik-benchmark-embedded-multiseed-gates
cmake --build out/release-native-lto --target rubik-benchmark-optimal-stress
cmake --build out/release-native-lto --target rubik-benchmark-optimal-stress-gates
cmake --build out/release-native-lto --target rubik-benchmark-v3-auto-gates
```

Configuration:

- Build preset: `release-native-lto`
- Cache mode: warm
- Auto effective profile for tail/hardening runs: `large-local`
- Auto threads on this host: `16`
- Auto maximum memory setting: `2048 MB`
- Auto large-local table payload: `1392639935` bytes

## Embedded Multiseed

The embedded multiseed suite covers fast depth-20 cases and optimal depth-13
cases across 12 deterministic seeds.

Result:

- Fast cases: `1200/1200` solved
- Optimal cases: `120/120` solved
- Failed cases: `0`
- Fast gate: `p95 <= 350 ms`, `p99 <= 500 ms`, `max <= 700 ms`
- Optimal gate: `p95/p99/max <= 4000 ms`

Worst rows:

| Mode | Seed | Cases | P95 ms | P99 ms | Max ms |
| --- | --- | ---: | ---: | ---: | ---: |
| Fast | `42` | 100 | 223 | 298 | 380 |
| Fast | `987654321` | 100 | 247 | 310 | 358 |
| Fast | `8675309` | 100 | 283 | 316 | 332 |
| Optimal | `12345` | 10 | 1929 | 1929 | 1929 |
| Optimal | `20260525` | 10 | 1664 | 1664 | 1664 |
| Optimal | `424242` | 10 | 1498 | 1498 | 1498 |

## Optimal Stress

The optimal stress suite validates depth-13 optimal solving for the
`embedded`, `default`, and `performance` public profiles.

Result:

- Cases: `90/90` solved
- Failed cases: `0`
- Embedded gate: `p95/p99/max <= 4000 ms`
- Default gate: `p95/p99/max <= 2500 ms`
- Performance gate: `p95/p99/max <= 2500 ms`

Worst rows:

| Profile | Seed | Cases | P95 ms | P99 ms | Max ms |
| --- | --- | ---: | ---: | ---: | ---: |
| `embedded` | `12345` | 10 | 2041 | 2041 | 2041 |
| `embedded` | `20260525` | 10 | 1720 | 1720 | 1720 |
| `default` | `20260525` | 10 | 1088 | 1088 | 1088 |
| `performance` | `20260525` | 10 | 1114 | 1114 | 1114 |

## V3 Auto Tail

The Auto tail suite targets known depth-15 optimal cases that exercise long
search tails with the large-local Auto profile.

Result:

- Cases: `7/7` solved
- Failed cases: `0`
- Gate: `p95/p99/max <= 12000 ms`

Tail rows:

| Seed | Solver ms | Nodes | Wall ms |
| --- | ---: | ---: | ---: |
| `1009` | 9384 | 33335637 | 10033 |
| `987654321` | 7454 | 26255087 | 8096 |
| `424242` | 2908 | 10306073 | 3551 |
| `555` | 2898 | 9656990 | 3543 |
| `99` | 2024 | 6127813 | 2669 |
| `888` | 1927 | 5677437 | 2575 |
| `666` | 964 | 2803885 | 1613 |

## V3 Auto Hardening

The Auto hardening suite covers two depth-14 cases and one depth-15 case for
each configured seed.

Result:

- Cases: `36/36` solved
- Failed cases: `0`
- Depth-14 gate: `p95/p99/max <= 4000 ms`
- Depth-15 gate: `p95/p99/max <= 12000 ms`

Slowest depth-15 rows:

| Seed | Solver ms | Nodes | Wall ms |
| --- | ---: | ---: | ---: |
| `987654321` | 7520 | 26275353 | 8166 |
| `8675309` | 4928 | 18405974 | 5574 |
| `12345` | 4803 | 17819935 | 5443 |
| `42` | 3918 | 13228620 | 4560 |
| `424242` | 2936 | 10343734 | 3588 |
| `99` | 2027 | 6161293 | 2671 |

## Interpretation

All repeatable V3 local gates passed in this run. The highest solver elapsed
time observed by these gates was `9384 ms` in the Auto depth-15 tail suite
for seed `1009`. The highest wall elapsed time observed in the same tail suite
was `10033 ms`.

The Auto large-local cache setup is a separate cost from per-cube solving. In
this run, cache setup prepared a `1392639935` byte table payload before the
tail and hardening suites. Per-cube latency should be interpreted separately
from cache preparation and warmup time.

No Raspberry Pi, Jetson, Orin, or other embedded hardware measurements are
included in this document.
