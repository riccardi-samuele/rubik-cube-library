# V6 Optimal Latency Pass 16 - Adaptive Profile Extraction

Date: 2026-05-29

## Goal

Make the V6 optimal-latency benchmark data easier to inspect before changing
solver policy again.

## Change

`scripts/analyze_root_search_profile.py` now extracts adaptive scheduler fields
from each case `rootOrderingProfile` and includes them in both detail and
`--summary` CSV output:

- `root_ordering_mode`
- `adaptive_decision`
- `adaptive_reason`
- `adaptive_lb`
- `adaptive_max_depth`
- `adaptive_threads`
- `adaptive_strong_min_count`
- `adaptive_first_diff`

This is a benchmark-tooling change only. It does not change solver behavior,
search order, pruning, or public API.

## Pass 12 Corpus Readout

Command:

```sh
scripts/analyze_root_search_profile.py \
  --input-dir out/release-native-lto/benchmark-results/v6-tail-pass12 \
  --summary
```

Grouped by `adaptive_decision`, `adaptive_reason`, and `root_ordering_mode`,
the accepted Pass 12 corpus shows:

| Group | Cases | Total solver elapsed | Max solver elapsed | Root nodes |
| --- | ---: | ---: | ---: | ---: |
| `deep_split:lb8_stable_mid_strong_min:default` | 4 | 7722 ms | 3749 ms | 44279683 |
| `deep_split:lb9_low_strong_min:default` | 5 | 3310 ms | 1835 ms | 15870767 |
| `deep_split:lb9_mid_strong_min:default` | 6 | 2148 ms | 1468 ms | 10098358 |
| `deep_split:lb9_mid_strong_min:reverse_tie` | 3 | 2122 ms | 1731 ms | 10484541 |
| `root:conservative_root:default` | 23 | 13005 ms | 2285 ms | 53636970 |
| `root:remaining_depth_lt_5:default` | 2 | 554 ms | 449 ms | 2357715 |

The largest measured Pass 12 case remains the `lb8_stable_mid_strong_min`
deep-split group. Pass 15 already tested disabling that group and regressed, so
the next solver experiment should target a narrower policy than simply removing
that branch.

## Decision

Accepted as tooling. Pass 12 remains the accepted solver implementation.

## Verification

- `scripts/analyze_root_search_profile.py --input-dir tests/fixtures/benchmark-results --summary`
- `scripts/analyze_root_search_profile.py --input-dir out/release-native-lto/benchmark-results/v6-tail-pass12 --summary`
- `ctest --test-dir out/release-native-lto -R "analyze_root_search_profile" --output-on-failure`
- `ctest --test-dir out/release-native-lto --output-on-failure`
- `git diff --check`
