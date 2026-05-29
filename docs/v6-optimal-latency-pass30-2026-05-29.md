# V6 optimal latency pass 30

## Goal

Make the Pass 29 finding repeatable: extract the most expensive roots for a
specific adaptive reason without hand-written shell pipelines.

## Change

`scripts/analyze_root_search_profile.py` now accepts:

- `--reason NAME` to keep rows with a matching `adaptive_reason`
- `--sort-by FIELD` to sort output rows by any emitted field
- `--sort-desc` to sort descending

The options work for both detailed root rows and `--summary` rows. This is
tooling only; solver behavior and public API behavior are unchanged.

## Validation

Commands:

```bash
cmake --preset release-native-lto
ctest --test-dir out/release-native-lto -R '^analyze_root_search_profile' --output-on-failure
scripts/analyze_root_search_profile.py \
  --input-dir out/release-native-lto/benchmark-results/v6-tail-pass20 \
  --reason lb8_stable_mid_strong_min \
  --sort-by root_elapsed_ms \
  --sort-desc \
  --limit 24 \
  --output out/release-native-lto/benchmark-results/v6-pass30-lb8-stable-top-roots.csv
```

Result:

- `analyze_root_search_profile`: passed
- `analyze_root_search_profile_rejects_missing_values`: passed
- `analyze_root_search_profile_filters_and_sorts`: passed
- `analyze_root_search_profile_summary`: passed

## Pass 20 top-root extraction

The 24 most expensive roots for `lb8_stable_mid_strong_min` are all
`not_found`, all before the solution root, and all from the two depth-15
`987654321` runs in the Pass 20 corpus.

Top rows:

| Source | Rank | Move | Outcome | Root elapsed ms | Nodes | Before solution |
| --- | ---: | --- | --- | ---: | ---: | --- |
| hardening depth 15 seed 987654321 | 11 | `U'` | `not_found` | 4930 | 2104537 | true |
| tail depth 15 seed 987654321 | 11 | `U'` | `not_found` | 4864 | 2104537 | true |
| hardening depth 15 seed 987654321 | 12 | `R` | `not_found` | 4445 | 1875933 | true |
| tail depth 15 seed 987654321 | 12 | `R` | `not_found` | 4391 | 1875933 | true |
| hardening depth 15 seed 987654321 | 3 | `R2` | `not_found` | 4318 | 1796088 | true |
| tail depth 15 seed 987654321 | 3 | `R2` | `not_found` | 4285 | 1796088 | true |

The largest per-root elapsed values are larger than solver elapsed because root
timings are per worker in the parallel root search; they measure accumulated
root work, not end-to-end wall time.

## Interpretation

The target bucket is not dominated by one pathological solution branch. It is
dominated by many expensive failed roots that are searched before the solution
root. The next solver experiment should be a narrow A/B policy for the
`lb8_stable_mid_strong_min` profile that attempts to move the eventual solution
root earlier or reduce expensive pre-solution roots, then compare against the
full Pass 20 corpus before acceptance.
