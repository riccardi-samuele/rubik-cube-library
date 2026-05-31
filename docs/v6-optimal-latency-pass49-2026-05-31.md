# V6 optimal latency pass 49

## Goal

Isolate the only profile that looked promising in pass 48, `8:7:1`, then
measure whether `phase2_tiebreak` is stable on that narrower corpus.

## Discovery Command

```bash
scripts/run_v6_conservative_root_targeted_corpus.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-871-discovery-pass49 \
  --seeds 42,99,424242,12345,20260525,314159,271828,987654321,7,123456789 \
  --random-count 2 \
  --random-start-indices 1,3,5,7 \
  --target-profiles 8:7:1 \
  --min-target-cases 2 \
  --threads 0 \
  --max-memory-mb 2048 \
  --discovery-only
```

## Discovery Result

The discovery input covered 10 seeds, 4 start-index windows, and 2 generated
cases per window: 80 discovery cases. It found 2 target rows:

```text
hardening,99,1,15,1,conservative_root
hardening,424242,8,15,1,conservative_root
```

Profile density:

| Profile | Cases | Discovery elapsed ms | Discovery nodes |
| --- | ---: | ---: | ---: |
| `8:7:1` | 2 | 3,911 | 23,553,364 |

No new `8:7:1` target rows were found beyond the two rows already seen in pass
47.

## Replay Command

```bash
scripts/run_v6_conservative_root_ordering_sweep.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --output-dir out/release-native-lto/benchmark-results/v6-conservative-root-871-replay-pass49 \
  --corpus-file out/release-native-lto/benchmark-results/v6-conservative-root-871-discovery-pass49/targeted_corpus.csv \
  --timeout-ms 30000 \
  --threads 0 \
  --max-memory-mb 2048 \
  --candidates phase2_tiebreak
```

## Replay Result

| Candidate | Cases | Baseline ms | Candidate ms | Delta ms | Delta % | Baseline max ms | Candidate max ms | Max delta ms | Baseline nodes | Candidate nodes | Node delta | Winner |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `phase2_tiebreak` | 2 | 4,105 | 4,141 | 36 | 0.88 | 2,887 | 2,937 | 50 | 23,932,235 | 23,890,860 | -41,375 | baseline |

Per-case result:

| Case | Baseline ms | Candidate ms | Delta ms | Winner |
| --- | ---: | ---: | ---: | --- |
| `hardening:depth15:seed99:random_99_1` | 1,218 | 1,204 | -14 | candidate |
| `hardening:depth15:seed424242:random_424242_8` | 2,887 | 2,937 | 50 | baseline |

## Decision

Do not promote even the narrow `8:7:1` policy. The pass 49 replay is worse
overall by `36 ms`, with one win and one loss. This contradicts the pass 48
two-row `8:7:1` win and confirms that the current signal is unstable.

The next useful direction is not another `phase2_tiebreak` policy attempt. The
priority should move back to broader latency work: either improve the discovery
strategy enough to produce a larger profile corpus, or investigate a different
root-ordering/pruning idea with stronger expected effect.
