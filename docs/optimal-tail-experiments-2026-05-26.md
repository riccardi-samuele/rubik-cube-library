# Optimal Tail Experiments - 2026-05-26

This report compares experimental optimal-mode pruning and ordering variants on
the five fixed embedded depth-13 tail cases from the V2 baseline.

The measurements are local desktop measurements only. They are not Raspberry Pi,
Jetson Nano, or Jetson Orin claims.

## Commands

Light matrix:

```sh
scripts/run_optimal_tail_experiments.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_optimal_tail_experiments_cache \
  --output-dir out/release-native-lto/benchmark-results/optimal-tail-experiments-light \
  --variants baseline,corner_state,phase2_ordering,strong_ordering,goal_depth6 \
  --timeout-ms 30000 \
  --max-memory-mb 1024
```

Heavy matrix:

```sh
scripts/run_optimal_tail_experiments.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_optimal_tail_experiments_heavy_cache \
  --output-dir out/release-native-lto/benchmark-results/optimal-tail-experiments-heavy \
  --variants corner_state_up,corner_state_down,corner_state_both \
  --timeout-ms 30000 \
  --max-memory-mb 2048
```

## Light Matrix

Profile: `Embedded`

| Variant | Cases | Solved | Average ms | Max ms | Average nodes | Max nodes | Payload bytes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `baseline` | 5 | 5 | 6,516.20 | 7,575 | 3,962,414.80 | 4,731,097 | 22,123,535 |
| `corner_state` | 5 | 5 | 1,482.60 | 2,346 | 630,429.20 | 1,001,517 | 110,303,375 |
| `phase2_ordering` | 5 | 5 | 8,342.80 | 11,446 | 3,962,348.60 | 4,731,097 | 23,661,455 |
| `strong_ordering` | 5 | 5 | 5,859.60 | 8,427 | 3,181,452.60 | 4,765,674 | 22,123,535 |
| `goal_depth6` | 5 | 5 | 11,068.00 | 16,419 | 3,961,445.60 | 4,730,778 | 22,123,535 |

Result:

- `corner_state` is the only light variant with a clear tail-latency and node
  reduction.
- `phase2_ordering` is slower than baseline on this set.
- `goal_depth6` is much slower than baseline on this set.
- `strong_ordering` reduces average nodes but increases the worst elapsed time,
  so it remains unsuitable as a default policy.

## Heavy Matrix

Profile: `Embedded`, memory budget raised to 2,048 MB.

| Variant | Cases | Solved | Average ms | Max ms | Average nodes | Max nodes | Payload bytes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `corner_state_up` | 5 | 5 | 842.40 | 1,181 | 254,752.60 | 360,657 | 589,304,975 |
| `corner_state_down` | 5 | 5 | 827.20 | 1,158 | 246,540.80 | 357,811 | 589,304,975 |
| `corner_state_both` | 5 | 5 | 650.20 | 835 | 168,000.40 | 228,304 | 1,068,306,575 |

Result:

- A single corner/edge-group table improves tail latency beyond `corner_state`,
  but costs about 589 MB of table payload.
- Both corner/edge-group tables produce the best solve times on this set, but
  exceed the current 1 GB embedded memory contract.
- The heavy variants are candidates for a local performance profile, not for
  the default embedded profile.

## Decision

Promote `corner_state` first. It keeps the table payload near 110 MB for the
embedded optimal profile and reduces the measured tail max from 7,575 ms to
2,346 ms on the fixed tail set.

Do not promote these variants yet:

- `phase2_ordering`;
- `strong_ordering`;
- `goal_depth6`;
- `corner_state_up`;
- `corner_state_down`;
- `corner_state_both`.

The corner/edge-group variants should remain experimental until larger profile
benchmarks prove that their memory cost is justified.

## Post-Promotion Validation

After promoting corner-state pruning, commit `ef19067` was benchmarked against
the old pruning policy by using `RUBIK_DISABLE_CORNER_STATE_BOUNDS=1`.

Command:

```sh
scripts/run_optimal_tail_experiments.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_optimal_tail_promoted_cache \
  --output-dir out/release-native-lto/benchmark-results/optimal-tail-promoted-ef19067 \
  --variants baseline,no_corner_state \
  --timeout-ms 30000 \
  --max-memory-mb 1024
```

| Variant | Cases | Solved | Average ms | Max ms | Average nodes | Max nodes | Payload bytes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `baseline` | 5 | 5 | 1,792.40 | 2,776 | 630,429.20 | 1,001,517 | 110,303,375 |
| `no_corner_state` | 5 | 5 | 9,473.80 | 11,825 | 3,962,414.80 | 4,731,097 | 22,123,535 |

The promoted default keeps the certified optimal result and reduces both node
expansion and tail latency on the fixed tail-case set.
