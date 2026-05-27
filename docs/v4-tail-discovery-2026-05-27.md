# V4 Tail Discovery - 2026-05-27

This document records the first V4 CPU-only optimal tail discovery run. The
numbers are local desktop measurements only.

## Commands

```sh
cmake --build out/release-native-lto --target rubik-benchmark-v4-tail-discovery -- -j1
```

The target runs:

```sh
scripts/run_v4_tail_discovery.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v4_tail_discovery_cache \
  --output-dir out/release-native-lto/benchmark-results/v4-tail-discovery \
  --threads 0 \
  --max-memory-mb 2048
```

## Configuration

- Build preset: `release-native-lto`
- Mode: `SolveMode::Optimal`
- Requested profile: `auto`
- Effective profile: `large-local`
- Threads: `16`
- Maximum memory setting: `2048 MB`
- Table payload: `1392639935` bytes
- Seeds: `987654321,424242,1009,2016,666,555,99,888,12345,8675309,20260525`
- Random depth: `15`
- Random cases per seed: `1`
- Timeout: `30000 ms`

The initial cache preparation observed for this target can be much larger than
per-cube solving time. The final captured warm-cache setup row for this run was
`653 ms`; an earlier cold preparation of the same cache path in this session
reported `230935 ms`.

## Result

- Cases: `11/11` solved
- Failed cases: `0`
- Slowest solver elapsed time: `9410 ms`
- Slowest wall elapsed time: `10060 ms`
- Slowest seed: `1009`
- Newly promoted slow seed from this discovery set: `2016`

## Slowest Rows

| Seed | Solver ms | Nodes | Wall ms |
| --- | ---: | ---: | ---: |
| `1009` | 9410 | 33146538 | 10060 |
| `987654321` | 7470 | 26120636 | 8109 |
| `2016` | 5380 | 19933087 | 6029 |
| `8675309` | 4999 | 18508779 | 5655 |
| `12345` | 4855 | 17540634 | 5504 |
| `424242` | 2986 | 10493503 | 3628 |
| `555` | 2915 | 9659301 | 3558 |
| `99` | 2011 | 6087359 | 2675 |
| `888` | 1906 | 5613892 | 2562 |
| `666` | 969 | 2808340 | 1614 |
| `20260525` | 102 | 144388 | 749 |

## Interpretation

The first expanded V4 discovery set confirms that `1009` is still the slowest
known depth-15 case on this host. Seed `2016` is now part of the V4 tail corpus
because it lands between the previous `987654321` and `8675309` hard cases.

No Raspberry Pi, Jetson, Orin, or other embedded hardware measurements are
included in this document.
