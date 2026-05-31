# V6 optimal latency pass 40

## Goal

Validate the V6 conservative-root probe runner on the measured pass 39 slow
cluster before testing solver-policy candidates.

## Command

```bash
RUBIK_BENCH_COMMAND_TIMEOUT_MS=45000 \
  cmake --build out/release-native-lto \
  --target rubik-benchmark-v6-conservative-root-probe
```

## Cache State

| Field | Value |
| --- | --- |
| Status | `Ready` |
| Effective profile | `large-local` |
| Payload bytes | 1392639935 |
| Cache warm | true |
| Bytes missing | 0 |
| Message | `dry run: cache warm` |

## Results

| Suite | Seed | Depth | Solver ms | Nodes | Adaptive reason |
| --- | ---: | ---: | ---: | ---: | --- |
| `hardening` | 42 | 15 | 4086 | 13450281 | `conservative_root` |
| `tail` | 424242 | 15 | 2430 | 10252468 | `conservative_root` |
| `hardening` | 424242 | 15 | 2382 | 10225559 | `conservative_root` |
| `tail` | 99 | 15 | 2560 | 6094772 | `conservative_root` |
| `hardening` | 99 | 15 | 2573 | 6205122 | `conservative_root` |

## Decision

Keep the default solver policy unchanged. The probe runner is ready for the
first explicit candidate experiment.
