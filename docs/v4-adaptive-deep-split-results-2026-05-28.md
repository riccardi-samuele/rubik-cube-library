# V4 Adaptive Deep Split Results - 2026-05-28

## Scope

- Runner: `scripts/run_v4_deep_split_ab.sh`
- Output directory: `out/release-native-lto/benchmark-results/v4-adaptive-deep-split-tuned`
- Variants: baseline, unconditional deep split, adaptive deep split
- Threads: `0` auto
- Memory limit: `2048` MB
- Timeout: `30000` ms
- Cache mode: `reuse`; cache setup was skipped because the cache was already warm
- Code revision at benchmark start: `2567d87`

This is a local desktop benchmark only. It contains no Raspberry Pi, Jetson,
Orin, GPU, cloud, or other external hardware measurements.

## Adaptive Comparison

```csv
seed,baseline_elapsed_ms,candidate_elapsed_ms,elapsed_delta_ms,elapsed_delta_percent,baseline_max_elapsed_ms,candidate_max_elapsed_ms,max_elapsed_delta_ms,baseline_nodes,candidate_nodes,nodes_delta,winner
1009,9457,7542,-1915,-20.25,9457,7542,-1915,33530906,28372538,-5158368,candidate
987654321,7674,7656,-18,-0.23,7674,7656,-18,26532553,26300295,-232258,candidate
2016,5350,5302,-48,-0.90,5350,5302,-48,19772431,19405534,-366897,candidate
8675309,4949,2450,-2499,-50.50,4949,2450,-2499,18376286,9397319,-8978967,candidate
12345,4797,4943,146,3.04,4797,4943,146,17372479,17702313,329834,baseline
424242,2992,3005,13,0.43,2992,3005,13,10347362,10367518,20156,baseline
555,2870,2911,41,1.43,2870,2911,41,9449880,9483773,33893,baseline
99,2051,2070,19,0.93,2051,2070,19,6196121,6160734,-35387,baseline
888,1929,1902,-27,-1.40,1929,1902,-27,5587457,5591643,4186,candidate
666,972,988,16,1.65,972,988,16,2806560,2806608,48,baseline
20260525,100,102,2,2.00,100,102,2,141213,142458,1245,baseline
__summary__,3921,3533,-388,-9.90,9457,7656,-1801,150113248,135730733,-14382515,candidate
```

## Unconditional Deep Split Comparison

```csv
seed,baseline_elapsed_ms,candidate_elapsed_ms,elapsed_delta_ms,elapsed_delta_percent,baseline_max_elapsed_ms,candidate_max_elapsed_ms,max_elapsed_delta_ms,baseline_nodes,candidate_nodes,nodes_delta,winner
1009,9457,7500,-1957,-20.69,9457,7500,-1957,33530906,28434895,-5096011,candidate
987654321,7674,6673,-1001,-13.04,7674,6673,-1001,26532553,23834883,-2697670,candidate
2016,5350,4181,-1169,-21.85,5350,4181,-1169,19772431,16077800,-3694631,candidate
8675309,4949,2442,-2507,-50.66,4949,2442,-2507,18376286,9389480,-8986806,candidate
12345,4797,2056,-2741,-57.14,4797,2056,-2741,17372479,7703576,-9668903,candidate
424242,2992,5526,2534,84.69,2992,5526,2534,10347362,20889029,10541667,baseline
555,2870,3099,229,7.98,2870,3099,229,9449880,11632404,2182524,baseline
99,2051,6430,4379,213.51,2051,6430,4379,6196121,22907324,16711203,baseline
888,1929,5830,3901,202.23,1929,5830,3901,5587457,20667340,15079883,baseline
666,972,5827,4855,499.49,972,5827,4855,2806560,22140656,19334096,baseline
20260525,100,83,-17,-17.00,100,83,-17,141213,71101,-70112,candidate
__summary__,3921,4513,592,15.10,9457,7500,-1957,150113248,183748488,33635240,baseline
```

## Decision

Decision: `promote_adaptive`.

Rationale:

- Average latency improved from `3921` ms to `3533` ms, a `9.90%` reduction.
- Max solver latency improved from `9457` ms to `7656` ms, a `1801` ms reduction.
- Every adaptive row solved as `Optimal,true`.
- The largest adaptive regression was seed `12345`, from `4797` ms to `4943` ms
  (`+146` ms, `+3.04%`), which is small relative to the max-latency win.
- Unconditional deep split is still rejected because it regressed average latency
  by `15.10%` and caused severe per-case regressions.

The promoted behavior should apply to local optimal `SolveProfile::Auto` solves
that select the large local table profile, and to explicit local
`SolveProfile::LargeLocal` optimal solves with multiple solver threads. Smaller
profiles should keep the existing scheduler unless an experimental flag is set.
