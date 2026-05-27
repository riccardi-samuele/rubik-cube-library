# V4 Deep Root Splitting Results - 2026-05-28

This note records the first local A/B run for the experimental optimal-search
deep root splitter.

## Scope

- Implementation flag: `RUBIK_EXPERIMENTAL_DEEP_ROOT_SPLIT=1`
- Benchmark runner: `scripts/run_v4_deep_split_ab.sh`
- Build directory: `out/release-native-lto`
- Cases file: `out/release-native-lto/benchmark-results/v4-tail-discovery/slowest.csv`
- Output directory: `out/release-native-lto/benchmark-results/v4-deep-split-ab`
- Threads: `0` auto, measured as 16 worker threads in cache setup
- Memory limit: `2048` MB
- Max depth: `15`
- Timeout: `30000` ms per case
- Cache setup: baseline pass warmed the cache in `226392` ms; candidate pass reused it
- Code revision at benchmark start: `3d16a28`

This is a local desktop benchmark only. It does not contain Raspberry Pi,
Jetson, Orin, or other external hardware measurements.

## Result

The deep split candidate is not ready to become the default.

```csv
seed,baseline_elapsed_ms,candidate_elapsed_ms,elapsed_delta_ms,elapsed_delta_percent,winner
1009,9368,7439,-1929,-20.59,candidate
987654321,7572,6646,-926,-12.23,candidate
2016,5404,4188,-1216,-22.50,candidate
8675309,4877,2426,-2451,-50.26,candidate
12345,4941,2038,-2903,-58.75,candidate
424242,2934,5518,2584,88.07,baseline
555,2912,3063,151,5.19,baseline
99,2029,6352,4323,213.06,baseline
888,1911,5806,3895,203.82,baseline
666,959,5824,4865,507.30,baseline
20260525,101,81,-20,-19.80,candidate
__summary__,3909,4489,580,14.84,baseline
```

Aggregate:

- Average solver time regressed from `3909` ms to `4489` ms.
- Average regression was `580` ms, or `14.84%`.
- Total searched nodes increased from `150088502` to `183731759`.
- The candidate won 6 of 11 cases, including the five slowest cases.
- The candidate lost 5 of 11 cases, with severe regressions on seeds `99`,
  `888`, and `666`.

## Interpretation

Deep root splitting improves worker utilization on high-tail cases by replacing
18 first-move root tasks with hundreds of second-level tasks. For example, seed
`1009` reported `split_depth=2` and `split_tasks=243`.

That helps when the default root-level schedule leaves a large expensive root on
one worker. It hurts when the solution would have been found early by the
default root schedule, because the split scheduler can spend work across many
second-level tasks before reaching the decisive branch. The result is a mixed
profile: better worst-case behavior on the slowest cases, but worse average
behavior on this corpus.

The per-root elapsed values in `root_search` are aggregate task elapsed times for
deep split runs, not single-root wall time. Use `elapsed_ms`, `wall_elapsed_ms`,
and `worker_search` for A/B decisions.

## Decision

Keep `RUBIK_EXPERIMENTAL_DEEP_ROOT_SPLIT` as an experiment. Do not promote it to
default.

Next useful work:

- Add a gated/adaptive splitter instead of splitting every case.
- Trigger splitting only when diagnostics predict root-level worker imbalance.
- Preserve default root order for likely early-solution cases.
- Re-run A/B after the splitter is selective rather than unconditional.
