# V6 optimal latency pass 35

## Goal

Check whether forced deep root splitting helps the remaining
`conservative_root` tail cases after the root-ordering probes were rejected.

This pass also records a cache precondition found during the probe: V6
large-local measurements are not valid unless the two large corner-permutation
edge-group tables are present in the cache.

## Cache precondition

The first probe attempt used:

```bash
RUBIK_BENCH_COMMAND_TIMEOUT_MS=45000 scripts/run_v6_tail_baseline.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --cache-mode reuse \
  --output-dir out/release-native-lto/benchmark-results/v6-pass35-conservative-probe-default \
  --tail-seeds 99 \
  --hardening-seeds 42 \
  --deep-opt14-count 1 \
  --deep-opt15-count 1 \
  --threads 0 \
  --max-memory-mb 2048 \
  --timeout-ms 30000
```

The run hit the new external hard timeout because the cache directory was only
partially warm. The cache had the normal performance tables, but was missing:

- `corner_permutation_up_edge_permutation.rpt`
- `corner_permutation_down_edge_permutation.rpt`

The required setup command was:

```bash
timeout --kill-after=10s 900s out/release-native-lto/rubik-cache-setup \
  --profile auto \
  --threads 0 \
  --max-memory-mb 2048 \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --format csv
```

Result:

| Field | Value |
| --- | ---: |
| Effective profile | `large-local` |
| Payload bytes | 1392639935 |
| Cache warm | true |
| Bytes missing | 0 |
| Setup elapsed ms | 212891 |

After setup, the cache directory was about `1.3G` and included both large files,
each about `457M`.

## Probe

Default:

```bash
RUBIK_BENCH_COMMAND_TIMEOUT_MS=45000 scripts/run_v6_tail_baseline.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --cache-mode reuse \
  --output-dir out/release-native-lto/benchmark-results/v6-pass35-conservative-probe-default \
  --tail-seeds 99 \
  --hardening-seeds 42 \
  --deep-opt14-count 1 \
  --deep-opt15-count 1 \
  --threads 0 \
  --max-memory-mb 2048 \
  --timeout-ms 30000
```

Forced deep split:

```bash
RUBIK_EXPERIMENTAL_DEEP_ROOT_SPLIT=1 RUBIK_BENCH_COMMAND_TIMEOUT_MS=45000 \
  scripts/run_v6_tail_baseline.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache \
  --cache-mode reuse \
  --output-dir out/release-native-lto/benchmark-results/v6-pass35-conservative-probe-forced-deep \
  --tail-seeds 99 \
  --hardening-seeds 42 \
  --deep-opt14-count 1 \
  --deep-opt15-count 1 \
  --threads 0 \
  --max-memory-mb 2048 \
  --timeout-ms 30000
```

Comparison:

```bash
scripts/compare_v6_latency.py \
  --baseline-dir out/release-native-lto/benchmark-results/v6-pass35-conservative-probe-default \
  --candidate-dir out/release-native-lto/benchmark-results/v6-pass35-conservative-probe-forced-deep \
  --group-by-reason \
  --sort-by elapsed_delta_ms \
  --sort-desc \
  --output out/release-native-lto/benchmark-results/v6-pass35-compare-conservative-forced-deep-reason-groups.csv
```

## Results

| Reason | Default ms | Forced deep ms | Delta ms | Winner |
| --- | ---: | ---: | ---: | --- |
| `conservative_root` | 4160 | 8001 | +3841 | Default |
| `depth14_conservative_root` | 251 | 250 | -1 | Forced deep |
| Summary | 4411 | 8251 | +3840 | Default |

Rows:

| Case | Default ms | Forced deep ms | Delta ms | Node delta |
| --- | ---: | ---: | ---: | ---: |
| tail depth 15 seed 99 | 1374 | 4374 | +3000 | +16816643 |
| hardening depth 15 seed 42 | 2786 | 3627 | +841 | +5375760 |
| hardening depth 14 seed 42 | 251 | 250 | -1 | +2240 |

## Decision

Reject forced deep root splitting for `conservative_root`. It almost doubles the
measured target subset and more than doubles node expansion in the
`conservative_root` group.

The next useful V6 work should not broaden deep splitting for conservative
depth-15 cases. Before any future large-local benchmark, run cache setup or use
`CachePolicy::RequireWarm`/the external hard timeout guard so cold-cache table
generation cannot be mistaken for solver latency.
