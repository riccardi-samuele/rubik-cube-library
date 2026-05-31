# V6 optimal latency pass 51

## Goal

Group the pass50 conservative-root discovery rows into coarser buckets. Exact
`adaptive_lb:adaptive_strong_min_count:adaptive_first_diff` profiles were too
fragmented to drive a reliable replay, so this pass measures whether broader
strong-minimum-count ranges produce useful target density.

## Tooling

Added `scripts/summarize_v6_profile_buckets.py`.

The script reads `targeted_cases.csv` and groups rows by:

- exact `adaptive_lb`
- `adaptive_strong_min_count` bucket: `0-4`, `5-8`, `9-12`, `13-16`, `17+`
- exact `adaptive_first_diff`

## Command

```bash
scripts/summarize_v6_profile_buckets.py \
  --targeted-cases out/release-native-lto/benchmark-results/v6-conservative-root-all-profiles-pass50/targeted_cases.csv \
  --output out/release-native-lto/benchmark-results/v6-conservative-root-all-profiles-pass50/bucket_summary.csv
```

## Bucket Result

The input contained 29 conservative-root rows from pass50. Bucket grouping
reduced the profile spread from 22 exact profiles to 15 broader buckets.

The densest buckets were:

| Bucket | Cases | Discovery elapsed ms | Discovery nodes | Profiles |
| --- | ---: | ---: | ---: | --- |
| `lb8_s5-8_fd1` | 6 | 4,248 | 24,158,608 | `8:5:1`, `8:6:1`, `8:7:1` |
| `lb8_s9-12_fd1` | 4 | 7,818 | 44,510,418 | `8:9:1`, `8:10:1`, `8:11:1` |
| `lb8_s13-16_fd1` | 4 | 9,939 | 42,504,976 | `8:13:1`, `8:14:1`, `8:16:1` |
| `lb9_s0-4_fd1` | 3 | 6,672 | 40,628,702 | `9:2:1`, `9:3:1` |
| `lb9_s5-8_fd1` | 2 | 2,286 | 12,822,359 | `9:8:1` |

All other buckets had one case in this sample.

## Decision

The coarse buckets are more useful than exact profiles, but they are still only
discovery density signals. Do not promote a solver policy from this pass alone.

The next replay should target `lb8_s5-8_fd1` first because it is the densest
observed bucket and it includes the previously studied `8:7:1` profile without
overfitting to that exact triple. If that replay is neutral or noisy, replay the
two four-case `lb8` buckets separately before changing solver behavior.
