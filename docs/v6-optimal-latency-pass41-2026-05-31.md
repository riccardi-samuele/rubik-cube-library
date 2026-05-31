# V6 optimal latency pass 41

## Goal

Compare existing root-ordering candidates on the measured pass 40
`conservative_root` depth-15 probe before changing any default solver policy.

## Command

```bash
RUBIK_BENCH_COMMAND_TIMEOUT_MS=45000 \
  cmake --build out/release-native-lto \
  --target rubik-benchmark-v6-conservative-root-ordering-sweep
```

## Cache State

The sweep reuses the pass 40 `require-warm` probe path. Each candidate probe
requires a warm `large-local` cache and rejected cold-cache measurements.

Every candidate recorded:

| Field | Value |
| --- | --- |
| Effective profile | `large-local` |
| Payload bytes | 1392639935 |
| Cache warm | true |
| Bytes missing | 0 |
| Message | `dry run: cache warm` |

## Summary

| Candidate | Cases | Default ms | Candidate ms | Delta ms | Delta % | Default max ms | Candidate max ms | Max delta ms | Default nodes | Candidate nodes | Node delta | Winner |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `reverse_tie` | 5 | 8362 | 9109 | 747 | 8.93 | 2432 | 2502 | 70 | 46323145 | 46351655 | 28510 | `baseline` |
| `high_bound_first` | 5 | 8608 | 14276 | 5668 | 65.85 | 2471 | 4812 | 2341 | 46282445 | 75182639 | 28900194 | `baseline` |
| `phase2_tiebreak` | 5 | 8633 | 8652 | 19 | 0.22 | 2466 | 2393 | -73 | 46595437 | 46439715 | -155722 | `baseline` |

## Decision

Reject promoting these root-ordering candidates from this pass. No default
solver policy changes were made.

`phase2_tiebreak` is the only close result: it reduced max solver time by
`73 ms` and total nodes by `155722`, but total solver time still regressed by
`19 ms`. The next step should inspect per-case root diagnostics for a stronger
discriminator instead of promoting a broad root-ordering rule.
