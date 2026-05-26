# Embedded Fast Tail-Case Diagnostics - 2026-05-25

These are the current slowest `Embedded/Fast` random depth-20 cases from the
single-seed and multiseed benchmark runs. The suite replays each case with
`--diagnose-fast` so phase-1 and phase-2 behavior can be inspected without
rerunning the full multiseed benchmark.

Command:

```sh
cmake --build out/release-native-lto --target rubik-benchmark-embedded-fast-tail-cases
```

Output directory:

```text
out/release-native-lto/benchmark-results/embedded-fast-tail-cases
```

## Cases

| Case | Source | Status | Elapsed ms | Nodes |
| --- | --- | --- | ---: | ---: |
| `random_12345_5` | original embedded fast failure | `Found` | 86 | 223,244 |
| `random_12345_13` | original embedded fast failure | `Found` | 94 | 250,159 |
| `random_20260525_58` | multiseed slowest fast case | `Found` | 259 | 679,337 |
| `random_20260525_61` | multiseed second slowest fast case | `Found` | 240 | 632,080 |
| `random_42_99` | multiseed slowest fast case for seed 42 | `Found` | 184 | 488,220 |

## Diagnostic Pattern

The current solver generates 16 phase-1 candidates for the embedded robust
attempt, sorts them with the phase-2 lower bound, and tries only the best 4.
This keeps the robust phase-2 budget unchanged while preventing good candidates
from being excluded by DFS discovery order.

All five cases now share the same broad shape:

- the initial `robust` attempt finds phase-1 candidates quickly;
- at least one of the best 4 sorted candidates completes phase 2;
- the `tail` attempt remains available as a safety net, but these replay cases
  no longer need it in normal solving.

This validates candidate ordering as a strong embedded fast optimization:
latency dropped from 533-862 ms into the 86-259 ms range on these tail cases
without increasing the embedded table profile.
