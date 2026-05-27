# V4 CPU Tail Latency Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build V4 as a CPU-only optimal-solver tail-latency release with deterministic discovery, repeatable tail corpus gates, diagnostics, and measured CPU optimizations.

**Architecture:** V4 starts by recording a clean V3 baseline, then adds benchmark tooling that discovers and replays slow optimal depth-15 cases. Solver changes are introduced only after the corpus and A/B comparison path exist, so each CPU optimization can be accepted or rejected by data while preserving optimality.

**Tech Stack:** C++20 library, CMake targets, Bash benchmark wrappers, Python CSV analysis scripts, CTest, existing `rubik-bench` and `rubik-cache-setup` CLIs.

---

## File Structure

- `docs/v4-cpu-tail-latency-design-2026-05-27.md`: approved V4 design reference.
- `docs/v4-local-baseline-2026-05-27.md`: new public benchmark evidence document for verified local baseline and later V4 runs.
- `docs/roadmap.md`: update the active roadmap from V3 to V4 after tooling exists.
- `scripts/run_v4_tail_discovery.sh`: new wrapper around `rubik-bench` for deterministic CPU-only tail discovery.
- `scripts/run_v4_tail_corpus.sh`: new wrapper that replays promoted slow cases and writes stable CSV summaries.
- `scripts/compare_v4_tail_runs.py`: new A/B comparison script for baseline versus candidate CSV directories.
- `tests/run_v4_tail_discovery_rejects_missing_values.sh`: argument validation test for discovery wrapper.
- `tests/run_v4_tail_corpus_rejects_missing_values.sh`: argument validation test for corpus wrapper.
- `tests/compare_v4_tail_runs_rejects_missing_values.sh`: argument validation test for comparison script.
- `tests/fixtures/benchmark-results/v4_baseline.csv`: fixture CSV for comparison tests.
- `tests/fixtures/benchmark-results/v4_candidate.csv`: fixture CSV for comparison tests.
- `CMakeLists.txt`: CTest entries and benchmark targets for V4 tooling.
- `src/solver.cpp`: later CPU optimization work, only after benchmark evidence exists.
- `include/rubik/solver.hpp`: only additive diagnostics if needed; avoid public API changes unless benchmark data justifies them.
- `tests/rubik_tests.cpp`: unit tests for any solver behavior or diagnostic additions.

## Task 1: Record Clean V3 Baseline

**Files:**
- Create: `docs/v4-local-baseline-2026-05-27.md`

- [ ] **Step 1: Build release-native-lto benchmark binaries**

Run:

```bash
cmake --build out/release-native-lto --target rubik-bench rubik-cache-setup
```

Expected: build completes with no errors.

- [ ] **Step 2: Run current V3 Auto gates**

Run:

```bash
cmake --build out/release-native-lto --target rubik-benchmark-v3-auto-gates
```

Expected: target completes and all V3 Auto benchmark gates pass.

- [ ] **Step 3: Extract the current slowest Auto tail rows**

Run:

```bash
scripts/extract_slowest_cases.sh \
  --input-dir out/release-native-lto/benchmark-results/optimal-auto-tail \
  --output out/release-native-lto/benchmark-results/v4_baseline_slowest.csv \
  --limit 20
```

Expected: `out/release-native-lto/benchmark-results/v4_baseline_slowest.csv` exists and starts with:

```csv
elapsed_ms,source_file,benchmark,mode,profile,slowest_rank,case_name,case_depth,scramble,status,move_count,nodes_expanded,solution
```

- [ ] **Step 4: Write the baseline document**

Create `docs/v4-local-baseline-2026-05-27.md` with this structure and fill
the result section from `v4_baseline_slowest.csv` before saving the file:

```markdown
# V4 Local Baseline - 2026-05-27

This document records the local V3 baseline used before V4 CPU tail-latency
work. The numbers are local desktop measurements only.

## Commands

```sh
cmake --build out/release-native-lto --target rubik-bench rubik-cache-setup
cmake --build out/release-native-lto --target rubik-benchmark-v3-auto-gates
scripts/extract_slowest_cases.sh \
  --input-dir out/release-native-lto/benchmark-results/optimal-auto-tail \
  --output out/release-native-lto/benchmark-results/v4_baseline_slowest.csv \
  --limit 20
```

## Result

- V3 Auto gates: passed
- Slowest known depth-15 Auto tail seed: `1009`
- Highest solver elapsed time observed in this run: use the first row of
  `v4_baseline_slowest.csv`
- Highest wall elapsed time observed in this run: use the matching benchmark
  file's `benchmark,wall_elapsed_ms` row

## Slowest Rows

| Seed | Solver ms | Nodes | Wall ms |
| --- | ---: | ---: | ---: |
| Use measured seed | Use measured solver elapsed ms | Use measured nodes | Use measured wall elapsed ms |

No Raspberry Pi, Jetson, Orin, or other embedded hardware measurements are
included in this document.
```

Replace the descriptive table row with measured values before committing.

- [ ] **Step 5: Verify the document has no placeholders**

Run:

```bash
rg -n "Use measured|PLACEHOLDER" docs/v4-local-baseline-2026-05-27.md
```

Expected: no matches.

- [ ] **Step 6: Commit baseline**

Run:

```bash
git add docs/v4-local-baseline-2026-05-27.md
git commit -m "Record V4 local baseline"
```

Expected: commit succeeds.

## Task 2: Add V4 Tail Discovery Wrapper

**Files:**
- Create: `scripts/run_v4_tail_discovery.sh`
- Create: `tests/run_v4_tail_discovery_rejects_missing_values.sh`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing argument validation test**

Create `tests/run_v4_tail_discovery_rejects_missing_values.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/run_v4_tail_discovery.sh}"

"${script}" --build-dir > /tmp/run_v4_tail_discovery_missing_value.out 2>&1 && {
    cat /tmp/run_v4_tail_discovery_missing_value.out >&2
    exit 1
}

grep -q "Usage: scripts/run_v4_tail_discovery.sh" /tmp/run_v4_tail_discovery_missing_value.out
```

Run:

```bash
chmod +x tests/run_v4_tail_discovery_rejects_missing_values.sh
tests/run_v4_tail_discovery_rejects_missing_values.sh
```

Expected: fails because `scripts/run_v4_tail_discovery.sh` does not exist yet.

- [ ] **Step 2: Create the discovery wrapper**

Create `scripts/run_v4_tail_discovery.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail

build_dir="out/release-native-lto"
cache_dir="${RUBIK_TABLE_CACHE_DIR:-/tmp/rubik_cube_library_v4_tail_discovery_cache}"
output_dir="benchmark-results/v4-tail-discovery"
seeds="987654321,424242,1009,2016,666,555,99,888,12345,8675309,20260525"
timeout_ms="30000"
max_depth="15"
random_depth="15"
random_count="1"
threads="0"
max_memory_mb="2048"
slowest_limit="50"
cache_mode="warm"

usage() {
    cat <<'USAGE'
Usage: scripts/run_v4_tail_discovery.sh [options]

Options:
  --build-dir DIR       CMake build directory, default: out/release-native-lto
  --cache-dir DIR       pruning table cache directory
  --output-dir DIR      output directory, default: benchmark-results/v4-tail-discovery
  --seeds LIST          comma-separated random seeds
  --timeout-ms N        per-case timeout, default: 30000
  --max-depth N         solver max depth, default: 15
  --random-depth N      random scramble depth, default: 15
  --random-count N      random cases per seed, default: 1
  --threads N           solver threads, default: 0
  --max-memory-mb N     solver memory limit, default: 2048
  --slowest-limit N     slowest rows to extract, default: 50
  --cache-mode MODE     warm|cold|reuse, default: warm
  -h, --help            show this help
USAGE
}

require_value() {
    if [[ $# -lt 2 || "$2" == -* ]]; then
        usage >&2
        exit 2
    fi
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir) require_value "$@"; build_dir="$2"; shift 2 ;;
        --cache-dir) require_value "$@"; cache_dir="$2"; shift 2 ;;
        --output-dir) require_value "$@"; output_dir="$2"; shift 2 ;;
        --seeds) require_value "$@"; seeds="$2"; shift 2 ;;
        --timeout-ms) require_value "$@"; timeout_ms="$2"; shift 2 ;;
        --max-depth) require_value "$@"; max_depth="$2"; shift 2 ;;
        --random-depth) require_value "$@"; random_depth="$2"; shift 2 ;;
        --random-count) require_value "$@"; random_count="$2"; shift 2 ;;
        --threads) require_value "$@"; threads="$2"; shift 2 ;;
        --max-memory-mb) require_value "$@"; max_memory_mb="$2"; shift 2 ;;
        --slowest-limit) require_value "$@"; slowest_limit="$2"; shift 2 ;;
        --cache-mode) require_value "$@"; cache_mode="$2"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) usage >&2; exit 2 ;;
    esac
done

for numeric in "${timeout_ms}" "${max_depth}" "${random_depth}" "${random_count}" "${threads}" "${max_memory_mb}" "${slowest_limit}"; do
    if [[ ! "${numeric}" =~ ^[0-9]+$ ]]; then
        usage >&2
        exit 2
    fi
done

if (( timeout_ms < 1 || max_depth < 1 || random_depth < 1 || random_count < 1 || max_memory_mb < 1 || slowest_limit < 1 )); then
    usage >&2
    exit 2
fi

if [[ "${cache_mode}" != "warm" && "${cache_mode}" != "cold" && "${cache_mode}" != "reuse" ]]; then
    usage >&2
    exit 2
fi

IFS=',' read -r -a seed_list <<< "${seeds}"
for seed in "${seed_list[@]}"; do
    if [[ -z "${seed}" || ! "${seed}" =~ ^[0-9]+$ ]]; then
        usage >&2
        exit 2
    fi
done

cmake --build "${build_dir}" --target rubik-bench rubik-cache-setup

bench="${build_dir}/rubik-bench"
cache_setup="${build_dir}/rubik-cache-setup"
if [[ ! -x "${bench}" || ! -x "${cache_setup}" ]]; then
    echo "required benchmark binaries are missing in ${build_dir}" >&2
    exit 1
fi

mkdir -p "${cache_dir}" "${output_dir}"
if [[ "${cache_mode}" == "cold" ]]; then
    rm -rf "${cache_dir}"
    mkdir -p "${cache_dir}"
fi

cache_setup_output="${output_dir}/cache_setup.csv"
if [[ "${cache_mode}" == "warm" || "${cache_mode}" == "cold" ]]; then
    "${cache_setup}" \
        --profile auto \
        --threads "${threads}" \
        --max-memory-mb "${max_memory_mb}" \
        --cache-dir "${cache_dir}" \
        --format csv \
        | tee "${cache_setup_output}"
else
    {
        echo "cache_setup,status,Skipped"
        echo "cache_setup,message,cache setup skipped by cache-mode reuse"
    } > "${cache_setup_output}"
fi

manifest_file="${output_dir}/manifest.csv"
summary_file="${output_dir}/summary.csv"
slowest_file="${output_dir}/slowest.csv"

{
    echo "key,value"
    echo "git_revision,$(git rev-parse --short HEAD 2>/dev/null || echo unknown)"
    echo "build_dir,${build_dir}"
    echo "cache_dir,${cache_dir}"
    echo "cache_mode,${cache_mode}"
    echo "output_dir,${output_dir}"
    echo "seeds,${seeds}"
    echo "timeout_ms,${timeout_ms}"
    echo "max_depth,${max_depth}"
    echo "random_depth,${random_depth}"
    echo "random_count,${random_count}"
    echo "threads,${threads}"
    echo "max_memory_mb,${max_memory_mb}"
} > "${manifest_file}"

{
    echo "seed,status,optimal,move_count,initial_lower_bound,elapsed_ms,nodes_expanded,warmup_elapsed_ms,wall_elapsed_ms,output_file"
} > "${summary_file}"

suite_status=0
for seed in "${seed_list[@]}"; do
    name="v4_tail_discovery_auto_random_${random_count}_depth_${random_depth}_seed_${seed}"
    output_file="${output_dir}/${name}.csv"
    started_at="$(date +%s%3N)"
    {
        echo "benchmark,name,${name}"
        echo "benchmark,cache_dir,${cache_dir}"
        echo "benchmark,seed,${seed}"
    } > "${output_file}"

    set +e
    RUBIK_TABLE_CACHE_DIR="${cache_dir}" "${bench}" \
        --mode optimal \
        --profile auto \
        --threads "${threads}" \
        --max-memory-mb "${max_memory_mb}" \
        --timeout-ms "${timeout_ms}" \
        --max-depth "${max_depth}" \
        --case-set random \
        --random-count "${random_count}" \
        --random-depth "${random_depth}" \
        --random-seed "${seed}" \
        --slowest-count "${random_count}" \
        --diagnose-optimal \
        >> "${output_file}"
    command_status="$?"
    set -e

    ended_at="$(date +%s%3N)"
    echo "benchmark,wall_elapsed_ms,$((ended_at - started_at))" >> "${output_file}"

    status="$(awk -F, '$1 ~ /^random_/ { print $4; exit }' "${output_file}")"
    optimal="$(awk -F, '$1 ~ /^random_/ { print $5; exit }' "${output_file}")"
    move_count="$(awk -F, '$1 ~ /^random_/ { print $6; exit }' "${output_file}")"
    lower_bound="$(awk -F, '$1 ~ /^random_/ { print $7; exit }' "${output_file}")"
    elapsed="$(awk -F, '$1 ~ /^random_/ { print $8; exit }' "${output_file}")"
    nodes="$(awk -F, '$1 ~ /^random_/ { print $9; exit }' "${output_file}")"
    warmup="$(awk -F, '$1 == "benchmark" && $2 == "warmup_elapsed_ms" { print $3; exit }' "${output_file}")"
    wall="$(awk -F, '$1 == "benchmark" && $2 == "wall_elapsed_ms" { print $3; exit }' "${output_file}")"
    echo "${seed},${status:-Unknown},${optimal:-false},${move_count:-0},${lower_bound:-0},${elapsed:-0},${nodes:-0},${warmup:-0},${wall:-0},${output_file}" >> "${summary_file}"

    if (( command_status != 0 )); then
        suite_status=1
    fi
done

scripts/extract_slowest_cases.sh \
    --input-dir "${output_dir}" \
    --output "${slowest_file}" \
    --limit "${slowest_limit}"

echo "v4 tail discovery manifest: ${manifest_file}"
echo "v4 tail discovery summary: ${summary_file}"
echo "v4 tail discovery slowest: ${slowest_file}"

exit "${suite_status}"
```

Run:

```bash
chmod +x scripts/run_v4_tail_discovery.sh
```

- [ ] **Step 3: Register the validation test**

Add this CTest entry near the existing script validation tests in `CMakeLists.txt`:

```cmake
add_test(
    NAME run_v4_tail_discovery_rejects_missing_values
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_v4_tail_discovery_rejects_missing_values.sh
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_v4_tail_discovery.sh
)
```

- [ ] **Step 4: Run validation**

Run:

```bash
ctest --test-dir out/release-native-lto -R run_v4_tail_discovery_rejects_missing_values --output-on-failure
scripts/run_v4_tail_discovery.sh --help
```

Expected: CTest passes and help output starts with `Usage: scripts/run_v4_tail_discovery.sh`.

- [ ] **Step 5: Commit discovery wrapper**

Run:

```bash
git add scripts/run_v4_tail_discovery.sh tests/run_v4_tail_discovery_rejects_missing_values.sh CMakeLists.txt
git commit -m "Add V4 tail discovery wrapper"
```

Expected: commit succeeds.

## Task 3: Add V4 Tail Corpus Replay

**Files:**
- Create: `scripts/run_v4_tail_corpus.sh`
- Create: `tests/run_v4_tail_corpus_rejects_missing_values.sh`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write failing validation test**

Create `tests/run_v4_tail_corpus_rejects_missing_values.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/run_v4_tail_corpus.sh}"

"${script}" --cases-file > /tmp/run_v4_tail_corpus_missing_value.out 2>&1 && {
    cat /tmp/run_v4_tail_corpus_missing_value.out >&2
    exit 1
}

grep -q "Usage: scripts/run_v4_tail_corpus.sh" /tmp/run_v4_tail_corpus_missing_value.out
```

Run:

```bash
chmod +x tests/run_v4_tail_corpus_rejects_missing_values.sh
tests/run_v4_tail_corpus_rejects_missing_values.sh
```

Expected: fails because the script does not exist yet.

- [ ] **Step 2: Create corpus replay wrapper**

Create `scripts/run_v4_tail_corpus.sh` to accept a slowest CSV produced by discovery and replay its `case_name` seed/index pairs. The parser should support case names like `random_1009_1` from `rubik-bench`.

Minimum behavior:

```bash
scripts/run_v4_tail_corpus.sh \
  --cases-file out/release-native-lto/benchmark-results/v4-tail-discovery/slowest.csv \
  --build-dir out/release-native-lto \
  --output-dir out/release-native-lto/benchmark-results/v4-tail-corpus \
  --cache-dir /tmp/rubik_cube_library_v4_tail_corpus_cache \
  --threads 0 \
  --max-memory-mb 2048 \
  --timeout-ms 30000
```

Expected outputs:

```text
v4 tail corpus manifest: <output-dir>/manifest.csv
v4 tail corpus summary: <output-dir>/summary.csv
v4 tail corpus slowest: <output-dir>/slowest.csv
```

Implementation rule: for a case named `random_1009_1`, replay it with:

```bash
--case-set random \
--random-count 1 \
--random-depth 15 \
--random-seed 1009 \
--random-start-index 1
```

- [ ] **Step 3: Register CTest validation**

Add:

```cmake
add_test(
    NAME run_v4_tail_corpus_rejects_missing_values
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_v4_tail_corpus_rejects_missing_values.sh
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_v4_tail_corpus.sh
)
```

- [ ] **Step 4: Run validation**

Run:

```bash
ctest --test-dir out/release-native-lto -R run_v4_tail_corpus_rejects_missing_values --output-on-failure
scripts/run_v4_tail_corpus.sh --help
```

Expected: CTest passes and help output starts with `Usage: scripts/run_v4_tail_corpus.sh`.

- [ ] **Step 5: Commit corpus replay**

Run:

```bash
git add scripts/run_v4_tail_corpus.sh tests/run_v4_tail_corpus_rejects_missing_values.sh CMakeLists.txt
git commit -m "Add V4 tail corpus replay"
```

Expected: commit succeeds.

## Task 4: Add V4 A/B Comparison Script

**Files:**
- Create: `scripts/compare_v4_tail_runs.py`
- Create: `tests/compare_v4_tail_runs_rejects_missing_values.sh`
- Create: `tests/fixtures/benchmark-results/v4_baseline.csv`
- Create: `tests/fixtures/benchmark-results/v4_candidate.csv`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add fixtures**

Create `tests/fixtures/benchmark-results/v4_baseline.csv`:

```csv
seed,status,optimal,move_count,initial_lower_bound,elapsed_ms,nodes_expanded,warmup_elapsed_ms,wall_elapsed_ms,output_file
1009,Optimal,true,15,11,9400,33335637,0,10033,baseline_1009.csv
987654321,Optimal,true,15,11,7422,26255087,0,8096,baseline_987654321.csv
```

Create `tests/fixtures/benchmark-results/v4_candidate.csv`:

```csv
seed,status,optimal,move_count,initial_lower_bound,elapsed_ms,nodes_expanded,warmup_elapsed_ms,wall_elapsed_ms,output_file
1009,Optimal,true,15,11,7600,28000000,0,8200,candidate_1009.csv
987654321,Optimal,true,15,11,7000,24000000,0,7700,candidate_987654321.csv
```

- [ ] **Step 2: Write failing validation test**

Create `tests/compare_v4_tail_runs_rejects_missing_values.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/compare_v4_tail_runs.py}"

"${script}" --baseline > /tmp/compare_v4_tail_runs_missing_value.out 2>&1 && {
    cat /tmp/compare_v4_tail_runs_missing_value.out >&2
    exit 1
}

grep -q "Usage: scripts/compare_v4_tail_runs.py" /tmp/compare_v4_tail_runs_missing_value.out
```

Run:

```bash
chmod +x tests/compare_v4_tail_runs_rejects_missing_values.sh
tests/compare_v4_tail_runs_rejects_missing_values.sh
```

Expected: fails because the script does not exist yet.

- [ ] **Step 3: Implement comparison script**

Create `scripts/compare_v4_tail_runs.py` with these requirements:

- accepts `--baseline FILE`, `--candidate FILE`, optional `--output FILE`;
- rejects missing option values with `Usage: scripts/compare_v4_tail_runs.py`;
- joins rows by `seed`;
- emits `seed,baseline_elapsed_ms,candidate_elapsed_ms,elapsed_delta_ms,elapsed_delta_percent,baseline_nodes,candidate_nodes,nodes_delta,winner`;
- emits an aggregate row named `__summary__` with average elapsed values and worst elapsed values.

For the fixtures, running:

```bash
scripts/compare_v4_tail_runs.py \
  --baseline tests/fixtures/benchmark-results/v4_baseline.csv \
  --candidate tests/fixtures/benchmark-results/v4_candidate.csv
```

Expected output includes:

```csv
seed,baseline_elapsed_ms,candidate_elapsed_ms,elapsed_delta_ms,elapsed_delta_percent,baseline_nodes,candidate_nodes,nodes_delta,winner
1009,9400,7600,-1800,-19.15,33335637,28000000,-5335637,candidate
987654321,7422,7000,-422,-5.69,26255087,24000000,-2255087,candidate
__summary__,8411,7300,-1111,-13.21,59590724,52000000,-7590724,candidate
```

- [ ] **Step 4: Register tests**

Add CTest entries:

```cmake
add_test(
    NAME compare_v4_tail_runs_rejects_missing_values
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/compare_v4_tail_runs_rejects_missing_values.sh
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/compare_v4_tail_runs.py
)

add_test(
    NAME compare_v4_tail_runs_fixture
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/compare_v4_tail_runs.py
        --baseline ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/benchmark-results/v4_baseline.csv
        --candidate ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/benchmark-results/v4_candidate.csv
)
set_tests_properties(compare_v4_tail_runs_fixture PROPERTIES
    PASS_REGULAR_EXPRESSION "__summary__,8411,7300,-1111,-13.21,59590724,52000000,-7590724,candidate"
)
```

- [ ] **Step 5: Run validation**

Run:

```bash
ctest --test-dir out/release-native-lto -R "compare_v4_tail_runs" --output-on-failure
```

Expected: both comparison tests pass.

- [ ] **Step 6: Commit comparison tooling**

Run:

```bash
git add scripts/compare_v4_tail_runs.py tests/compare_v4_tail_runs_rejects_missing_values.sh tests/fixtures/benchmark-results/v4_baseline.csv tests/fixtures/benchmark-results/v4_candidate.csv CMakeLists.txt
git commit -m "Add V4 tail comparison tooling"
```

Expected: commit succeeds.

## Task 5: Add CMake Targets For V4 Tail Runs

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `docs/benchmarks.md`
- Modify: `docs/roadmap.md`

- [ ] **Step 1: Add custom targets**

Add targets:

```cmake
add_custom_target(rubik-benchmark-v4-tail-discovery
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_v4_tail_discovery.sh
        --build-dir ${CMAKE_CURRENT_BINARY_DIR}
        --cache-dir /tmp/rubik_cube_library_v4_tail_discovery_cache
        --output-dir ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results/v4-tail-discovery
        --threads 0
        --max-memory-mb 2048
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running V4 optimal CPU tail discovery"
)

add_custom_target(rubik-benchmark-v4-tail-corpus
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_v4_tail_corpus.sh
        --build-dir ${CMAKE_CURRENT_BINARY_DIR}
        --cases-file ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results/v4-tail-discovery/slowest.csv
        --cache-dir /tmp/rubik_cube_library_v4_tail_corpus_cache
        --output-dir ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results/v4-tail-corpus
        --threads 0
        --max-memory-mb 2048
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running V4 optimal CPU tail corpus"
)
```

- [ ] **Step 2: Document targets**

Add `rubik-benchmark-v4-tail-discovery` and `rubik-benchmark-v4-tail-corpus` to `docs/benchmarks.md`, clearly marking discovery as a longer local run.

Update `docs/roadmap.md` with a `Road To 4.0` section that says V4 is CPU-only tail latency, not GPU/cloud/hardware claims.

- [ ] **Step 3: Run validation**

Run:

```bash
cmake --build out/release-native-lto --target rubik-benchmark-v4-tail-discovery -- -j1
cmake --build out/release-native-lto --target rubik-benchmark-v4-tail-corpus -- -j1
```

Expected: both targets complete. If full discovery is too slow during development, rerun the underlying scripts with a smaller explicit seed list and commit only after the default target has been validated at least once.

- [ ] **Step 4: Commit targets and docs**

Run:

```bash
git add CMakeLists.txt docs/benchmarks.md docs/roadmap.md
git commit -m "Add V4 tail benchmark targets"
```

Expected: commit succeeds.

## Task 6: Run Expanded Tail Discovery And Promote Corpus

**Files:**
- Create: `docs/v4-tail-discovery-2026-05-27.md`
- Create: `docs/v4-tail-corpus-2026-05-27.md`

- [ ] **Step 1: Run expanded discovery**

Run:

```bash
scripts/run_v4_tail_discovery.sh \
  --build-dir out/release-native-lto \
  --output-dir out/release-native-lto/benchmark-results/v4-tail-discovery \
  --cache-dir /tmp/rubik_cube_library_v4_tail_discovery_cache \
  --seeds 987654321,424242,1009,2016,666,555,99,888,12345,8675309,20260525,314159,271828,1618033,777,123456789 \
  --random-count 1 \
  --threads 0 \
  --max-memory-mb 2048 \
  --timeout-ms 30000 \
  --cache-mode warm
```

Expected: `slowest.csv`, `summary.csv`, and per-seed CSV files are produced.

- [ ] **Step 2: Replay corpus**

Run:

```bash
scripts/run_v4_tail_corpus.sh \
  --build-dir out/release-native-lto \
  --cases-file out/release-native-lto/benchmark-results/v4-tail-discovery/slowest.csv \
  --output-dir out/release-native-lto/benchmark-results/v4-tail-corpus \
  --cache-dir /tmp/rubik_cube_library_v4_tail_corpus_cache \
  --threads 0 \
  --max-memory-mb 2048 \
  --timeout-ms 30000 \
  --cache-mode warm
```

Expected: corpus replay completes with the same slow cases replayable by seed/index.

- [ ] **Step 3: Analyze root diagnostics**

Run:

```bash
scripts/analyze_root_search_profile.py \
  --input-dir out/release-native-lto/benchmark-results/v4-tail-corpus \
  --summary \
  --output out/release-native-lto/benchmark-results/v4-tail-corpus/root_summary.csv
```

Expected: root summary CSV exists and includes `max_root_elapsed_ms`.

- [ ] **Step 4: Write discovery docs**

Create:

- `docs/v4-tail-discovery-2026-05-27.md`
- `docs/v4-tail-corpus-2026-05-27.md`

Each document must include commands, build preset, cache mode, thread setting, memory setting, slowest rows, and a note that no embedded hardware measurements are included.

- [ ] **Step 5: Commit evidence**

Run:

```bash
git add docs/v4-tail-discovery-2026-05-27.md docs/v4-tail-corpus-2026-05-27.md
git commit -m "Record V4 tail discovery corpus"
```

Expected: commit succeeds.

## Task 7: Evaluate CPU Optimization Candidates

**Files:**
- Modify: `src/solver.cpp`
- Modify: `tests/rubik_tests.cpp`
- Modify: `docs/v4-tail-corpus-2026-05-27.md`

- [ ] **Step 1: Run existing root ordering experiments**

Run:

```bash
scripts/benchmark_root_ordering_experiments.sh \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_root_ordering_cache \
  --output-dir out/release-native-lto/benchmark-results/v4-root-ordering-experiments \
  --seeds 987654321,424242,1009,2016,666,555,99,888,12345,8675309 \
  --threads 0 \
  --max-memory-mb 2048 \
  --cache-mode warm
```

Expected: `comparison.csv` identifies whether `default`, `reverse_tie`, or `phase2_tiebreak` wins broadly.

- [ ] **Step 2: Choose only data-backed changes**

If no candidate wins broadly, do not change default solver behavior. Instead, record the result in docs and continue with scheduling diagnostics.

If a candidate wins broadly, add a unit test in `tests/rubik_tests.cpp` asserting that the effective `plan.rootOrderingProfile` reports the chosen mode under the intended environment/configuration.

- [ ] **Step 3: Implement minimal solver change**

Modify only the relevant root ordering or scheduling block in `src/solver.cpp`. Preserve all candidate enumeration and depth proof logic. Do not accept a change that alters the optimal move count for any existing test.

- [ ] **Step 4: Run correctness tests**

Run:

```bash
ctest --test-dir out/release-native-lto -R "rubik_tests|cli_bench_reports_root_ordering_profile|analyze_root_search_profile" --output-on-failure
```

Expected: all selected tests pass.

- [ ] **Step 5: Run V4 corpus A/B**

Run baseline and candidate corpus summaries, then:

```bash
scripts/compare_v4_tail_runs.py \
  --baseline out/release-native-lto/benchmark-results/v4-tail-corpus-baseline/summary.csv \
  --candidate out/release-native-lto/benchmark-results/v4-tail-corpus/summary.csv \
  --output out/release-native-lto/benchmark-results/v4-tail-corpus/comparison.csv
```

Expected: candidate improves average or max latency without significant known-case regressions.

- [ ] **Step 6: Commit accepted optimization**

Run:

```bash
git add src/solver.cpp tests/rubik_tests.cpp docs/v4-tail-corpus-2026-05-27.md
git commit -m "Improve optimal CPU tail latency"
```

Expected: commit only if data supports the change. If data does not support it, commit the negative experiment documentation instead with message `Record V4 CPU optimization experiment`.

## Task 8: Final V4 Validation

**Files:**
- Create: `docs/release-4.0.0.md`
- Create: `docs/github-release-v4.0.0.md`
- Create: `docs/api-stability-4.0.0.md`
- Modify: `CMakeLists.txt`
- Modify: version files found by `rg -n "3\\.0\\.0|version_major|PROJECT_VERSION"`

- [ ] **Step 1: Run full quick release check before version bump**

Run:

```bash
scripts/release_check.sh --profile quick --with-v3-auto
```

Expected: release check passes before version changes.

- [ ] **Step 2: Add V4 release docs**

Create V4 release docs using only measured local data from V4 docs and benchmark CSVs. Include no hardware claims for devices not tested directly.

- [ ] **Step 3: Bump version to 4.0.0**

Update CMake/project version and version tests consistently. Use:

```bash
rg -n "3\\.0\\.0|version_major|PROJECT_VERSION"
```

Expected: no stale public version references that should be `4.0.0`.

- [ ] **Step 4: Run final validation**

Run:

```bash
scripts/release_check.sh --profile quick --with-v3-auto
ctest --test-dir out/release-native-lto -R "public_docs_no_unverified_hardware_estimates|check_release_archive|release_check_archive" --output-on-failure
```

Expected: all validation passes.

- [ ] **Step 5: Commit final V4 release prep**

Run:

```bash
git add CMakeLists.txt include src tests docs scripts
git commit -m "Prepare V4 release"
```

Expected: commit succeeds.

## Execution Notes

- Do not push unless explicitly instructed.
- Commit after each task that leaves the repository in a tested, coherent state.
- Keep generated benchmark output under build/output directories or ignored `dist/` paths unless a document intentionally records verified summary data.
- Do not publish performance claims for Raspberry Pi, Jetson, Orin, or any other hardware until those devices are directly measured.
- If a benchmark result contradicts the expected V4 target, update the target from evidence instead of forcing an optimization.
