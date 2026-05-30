# V6 optimal latency pass 37

## Goal

Make V6 tail-latency benchmark artifacts easier to audit after a run.

The benchmark runner already writes `cache_setup.csv`; this pass records the
manifest metadata added so a report can identify the cache setup artifact and
its wrapper elapsed time without inspecting the script invocation.

## Change

`scripts/run_v6_tail_baseline.sh` now writes these additional `manifest.csv`
fields:

| Field | Meaning |
| --- | --- |
| `cache_setup_output` | Path to the cache setup or dry-run CSV artifact. |
| `cache_setup_elapsed_ms` | Wrapper elapsed time spent in cache setup or dry-run. |

For `--cache-mode require-warm`, the manifest is written before the warm-cache
check. If the cache is incomplete, the runner exits before creating the
per-suite benchmark directories, but the manifest and `cache_setup.csv` remain
available for diagnosis.

For `--cache-mode reuse`, `cache_setup_elapsed_ms` stays `0` and
`cache_setup.csv` records that setup was skipped.

## Verification

The regression test extends the cold-cache rejection case and verifies that the
manifest exists before rejection and contains both cache metadata fields.

Commands:

```bash
ctest --test-dir out/release-native-lto -R 'run_v6_tail_baseline_require_warm_rejects_cold_cache|run_v6_tail_baseline_rejects_missing_values' --output-on-failure
ctest --test-dir out/release-native-lto --output-on-failure
git diff --check
```

Results:

| Check | Result |
| --- | ---: |
| Targeted V6 runner tests | 2/2 passed |
| Full CTest suite | 94/94 passed |
| Whitespace check | passed |

## Decision

Keep the V6 runner manifest as the top-level audit artifact for latency probes.
Use `cache_setup.csv` for detailed cache state and per-case CSV files for solver
latency, nodes, warm-up timing, and root-search diagnostics.
