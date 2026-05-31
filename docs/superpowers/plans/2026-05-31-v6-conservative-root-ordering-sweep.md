# V6 Conservative-Root Ordering Sweep Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add and run a measurement-only V6 sweep that compares existing root-ordering candidates on the pass 40 `conservative_root` probe.

**Architecture:** Add a shell orchestration runner around the existing `run_v6_conservative_root_probe.sh` and `compare_v6_latency.py` tools. The runner validates candidates, runs default-vs-candidate probe output per ordering mode, writes comparison CSV files, and aggregates a sweep summary without changing solver policy.

**Tech Stack:** Bash, CMake custom targets, CTest shell tests, existing `rubik-bench`, existing `run_v6_conservative_root_probe.sh`, existing `compare_v6_latency.py`, CSV artifacts.

---

## File Structure

- `scripts/run_v6_conservative_root_ordering_sweep.sh`: new orchestration runner. Validates arguments and candidate names, calls the existing probe runner with `--candidate-env RUBIK_EXPERIMENTAL_ROOT_ORDERING={mode}`, compares output, and writes `summary.csv`.
- `tests/run_v6_conservative_root_ordering_sweep_rejects_missing_values.sh`: missing option value contract test.
- `tests/run_v6_conservative_root_ordering_sweep_rejects_invalid_candidate.sh`: unsupported candidate rejection test.
- `tests/run_v6_conservative_root_ordering_sweep_fake_tools.sh`: fake-tool smoke test verifying candidate env forwarding and summary aggregation without running real benchmarks.
- `CMakeLists.txt`: registers the three tests and adds `rubik-benchmark-v6-conservative-root-ordering-sweep`.
- `docs/v6-optimal-latency-pass41-2026-05-31.md`: measured sweep report created after the real target runs.
- `docs/benchmarks.md`: links the pass 41 report.

## Task 1: Add Runner Contract Tests

**Files:**
- Create: `tests/run_v6_conservative_root_ordering_sweep_rejects_missing_values.sh`
- Create: `tests/run_v6_conservative_root_ordering_sweep_rejects_invalid_candidate.sh`
- Create: `tests/run_v6_conservative_root_ordering_sweep_fake_tools.sh`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add missing-value test**

Create `tests/run_v6_conservative_root_ordering_sweep_rejects_missing_values.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/run_v6_conservative_root_ordering_sweep.sh}"

"${script}" --build-dir > /tmp/run_v6_conservative_root_ordering_sweep_missing_value.out 2>&1 && {
    cat /tmp/run_v6_conservative_root_ordering_sweep_missing_value.out >&2
    exit 1
}

grep -q "Usage: scripts/run_v6_conservative_root_ordering_sweep.sh" /tmp/run_v6_conservative_root_ordering_sweep_missing_value.out
```

- [ ] **Step 2: Add invalid-candidate test**

Create `tests/run_v6_conservative_root_ordering_sweep_rejects_invalid_candidate.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/run_v6_conservative_root_ordering_sweep.sh}"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "${tmp_dir}"' EXIT

set +e
"${script}" \
    --build-dir "${tmp_dir}/build" \
    --cache-dir "${tmp_dir}/cache" \
    --output-dir "${tmp_dir}/out" \
    --corpus-file benchmarks/v6_conservative_root_corpus.csv \
    --candidates reverse_tie,not_a_candidate \
    > "${tmp_dir}/run.out" 2>&1
status="$?"
set -e

if [[ "${status}" -eq 0 ]]; then
    cat "${tmp_dir}/run.out" >&2
    echo "expected invalid candidate rejection" >&2
    exit 1
fi

grep -q "unsupported root ordering candidate: not_a_candidate" "${tmp_dir}/run.out"

if [[ -d "${tmp_dir}/out" ]]; then
    find "${tmp_dir}/out" -maxdepth 3 -type f >&2
    echo "output directory should not be created after candidate validation failure" >&2
    exit 1
fi
```

- [ ] **Step 3: Add fake-tool smoke test**

Create `tests/run_v6_conservative_root_ordering_sweep_fake_tools.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/run_v6_conservative_root_ordering_sweep.sh}"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "${tmp_dir}"' EXIT

probe_script="${tmp_dir}/fake_probe.sh"
compare_script="${tmp_dir}/fake_compare.py"
log_file="${tmp_dir}/calls.log"

cat > "${probe_script}" <<'PROBE'
#!/usr/bin/env bash
set -euo pipefail

output_dir=""
candidate_env=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --output-dir)
            output_dir="$2"
            shift 2
            ;;
        --candidate-env)
            candidate_env="$2"
            shift 2
            ;;
        *)
            shift
            ;;
    esac
done

if [[ -z "${output_dir}" || -z "${candidate_env}" ]]; then
    echo "fake probe missing required values" >&2
    exit 1
fi

mkdir -p "${output_dir}/default" "${output_dir}/candidate"
printf '%s\n' "${candidate_env}" >> "${FAKE_SWEEP_LOG}"
cat > "${output_dir}/cache_setup.csv" <<'CSV'
cache_setup,status,Ready
cache_setup,effective_profile,large-local
cache_setup,payload_bytes,1392639935
cache_setup,cache_warm,true
cache_setup,bytes_missing,0
cache_setup,message,dry run: cache warm
CSV
touch "${output_dir}/default/warm_fake.csv" "${output_dir}/candidate/warm_fake.csv"
PROBE

cat > "${compare_script}" <<'COMPARE'
#!/usr/bin/env python3
import csv
import sys
from pathlib import Path

candidate_dir = ""
output = ""
args = sys.argv[1:]
for index, token in enumerate(args):
    if token == "--candidate-dir":
        candidate_dir = args[index + 1]
    if token == "--output":
        output = args[index + 1]

if not candidate_dir or not output:
    print("fake compare missing values", file=sys.stderr)
    sys.exit(1)

name = Path(candidate_dir).parent.name
delta = {
    "reverse_tie": -100,
    "high_bound_first": 25,
    "phase2_tiebreak": -50,
}[name]
candidate_elapsed = 1000 + delta
nodes_delta = {
    "reverse_tie": -200,
    "high_bound_first": 50,
    "phase2_tiebreak": 0,
}[name]
candidate_nodes = 10000 + nodes_delta

Path(output).parent.mkdir(parents=True, exist_ok=True)
with open(output, "w", newline="") as handle:
    writer = csv.writer(handle)
    writer.writerow([
        "case_key",
        "common_cases",
        "baseline_elapsed_ms",
        "candidate_elapsed_ms",
        "elapsed_delta_ms",
        "elapsed_delta_percent",
        "baseline_p50_ms",
        "candidate_p50_ms",
        "p50_delta_ms",
        "baseline_p90_ms",
        "candidate_p90_ms",
        "p90_delta_ms",
        "baseline_p95_ms",
        "candidate_p95_ms",
        "p95_delta_ms",
        "baseline_p99_ms",
        "candidate_p99_ms",
        "p99_delta_ms",
        "baseline_max_elapsed_ms",
        "candidate_max_elapsed_ms",
        "max_elapsed_delta_ms",
        "baseline_nodes",
        "candidate_nodes",
        "nodes_delta",
        "baseline_wall_ms",
        "candidate_wall_ms",
        "wall_delta_ms",
        "winner",
        "baseline_ordering",
        "candidate_ordering",
        "baseline_reason",
        "candidate_reason",
    ])
    writer.writerow([
        "__summary__",
        5,
        1000,
        candidate_elapsed,
        delta,
        f"{delta / 10:.2f}",
        200,
        190,
        -10,
        300,
        290,
        -10,
        400,
        390,
        -10,
        500,
        490,
        -10,
        500,
        500 + max(delta, 0),
        max(delta, 0),
        10000,
        candidate_nodes,
        nodes_delta,
        600,
        600,
        0,
        "candidate" if delta < 0 else "baseline",
        "",
        "",
        "",
        "",
    ])
print(f"fake comparison: {output}")
COMPARE

chmod +x "${probe_script}" "${compare_script}"

FAKE_SWEEP_LOG="${log_file}" "${script}" \
    --build-dir "${tmp_dir}/build" \
    --cache-dir "${tmp_dir}/cache" \
    --output-dir "${tmp_dir}/out" \
    --corpus-file benchmarks/v6_conservative_root_corpus.csv \
    --probe-script "${probe_script}" \
    --compare-script "${compare_script}" \
    --candidates reverse_tie,high_bound_first,phase2_tiebreak \
    > "${tmp_dir}/run.out" 2>&1

grep -q "RUBIK_EXPERIMENTAL_ROOT_ORDERING=reverse_tie" "${log_file}"
grep -q "RUBIK_EXPERIMENTAL_ROOT_ORDERING=high_bound_first" "${log_file}"
grep -q "RUBIK_EXPERIMENTAL_ROOT_ORDERING=phase2_tiebreak" "${log_file}"

summary="${tmp_dir}/out/summary.csv"
test -f "${summary}"
grep -q "reverse_tie,5,1000,900,-100,-10.00,500,500,0,10000,9800,-200,candidate" "${summary}"
grep -q "high_bound_first,5,1000,1025,25,2.50,500,525,25,10000,10050,50,baseline" "${summary}"
grep -q "phase2_tiebreak,5,1000,950,-50,-5.00,500,500,0,10000,10000,0,candidate" "${summary}"
```

- [ ] **Step 4: Make tests executable**

Run:

```bash
chmod +x \
  tests/run_v6_conservative_root_ordering_sweep_rejects_missing_values.sh \
  tests/run_v6_conservative_root_ordering_sweep_rejects_invalid_candidate.sh \
  tests/run_v6_conservative_root_ordering_sweep_fake_tools.sh
```

- [ ] **Step 5: Register CMake tests**

Add near the existing V6 probe tests in `CMakeLists.txt`:

```cmake
add_test(
    NAME run_v6_conservative_root_ordering_sweep_rejects_missing_values
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_v6_conservative_root_ordering_sweep_rejects_missing_values.sh
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_v6_conservative_root_ordering_sweep.sh
)
add_test(
    NAME run_v6_conservative_root_ordering_sweep_rejects_invalid_candidate
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_v6_conservative_root_ordering_sweep_rejects_invalid_candidate.sh
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_v6_conservative_root_ordering_sweep.sh
)
add_test(
    NAME run_v6_conservative_root_ordering_sweep_fake_tools
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_v6_conservative_root_ordering_sweep_fake_tools.sh
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_v6_conservative_root_ordering_sweep.sh
)
```

- [ ] **Step 6: Verify RED**

Run:

```bash
cmake --preset release-native-lto
ctest --test-dir out/release-native-lto -R 'run_v6_conservative_root_ordering_sweep' --output-on-failure
```

Expected: tests fail because `scripts/run_v6_conservative_root_ordering_sweep.sh` does not exist.

## Task 2: Implement Ordering Sweep Runner

**Files:**
- Create: `scripts/run_v6_conservative_root_ordering_sweep.sh`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Create runner script**

Create `scripts/run_v6_conservative_root_ordering_sweep.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

build_dir="out/release-native-lto"
cache_dir="${RUBIK_TABLE_CACHE_DIR:-/tmp/rubik_cube_library_v6_tail_baseline_cache}"
output_dir="benchmark-results/v6-conservative-root-ordering-sweep"
corpus_file="benchmarks/v6_conservative_root_corpus.csv"
timeout_ms="30000"
threads="0"
max_memory_mb="2048"
candidates="reverse_tie,high_bound_first,phase2_tiebreak"
probe_script="${script_dir}/run_v6_conservative_root_probe.sh"
compare_script="${script_dir}/compare_v6_latency.py"

usage() {
    cat <<'USAGE'
Usage: scripts/run_v6_conservative_root_ordering_sweep.sh [options]

Options:
  --build-dir DIR        CMake build directory, default: out/release-native-lto
  --cache-dir DIR        pruning table cache directory
  --output-dir DIR       output directory, default: benchmark-results/v6-conservative-root-ordering-sweep
  --corpus-file FILE     CSV corpus, default: benchmarks/v6_conservative_root_corpus.csv
  --timeout-ms N         per-case timeout, default: 30000
  --threads N            solver threads, default: 0
  --max-memory-mb N      solver memory limit, default: 2048
  --candidates LIST      comma-separated candidates: reverse_tie,high_bound_first,phase2_tiebreak
  --probe-script FILE    probe runner, default: scripts/run_v6_conservative_root_probe.sh
  --compare-script FILE  comparison script, default: scripts/compare_v6_latency.py
  -h, --help             show this help
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
        --build-dir)
            require_value "$@"
            build_dir="$2"
            shift 2
            ;;
        --cache-dir)
            require_value "$@"
            cache_dir="$2"
            shift 2
            ;;
        --output-dir)
            require_value "$@"
            output_dir="$2"
            shift 2
            ;;
        --corpus-file)
            require_value "$@"
            corpus_file="$2"
            shift 2
            ;;
        --timeout-ms)
            require_value "$@"
            timeout_ms="$2"
            shift 2
            ;;
        --threads)
            require_value "$@"
            threads="$2"
            shift 2
            ;;
        --max-memory-mb)
            require_value "$@"
            max_memory_mb="$2"
            shift 2
            ;;
        --candidates)
            require_value "$@"
            candidates="$2"
            shift 2
            ;;
        --probe-script)
            require_value "$@"
            probe_script="$2"
            shift 2
            ;;
        --compare-script)
            require_value "$@"
            compare_script="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            usage >&2
            exit 2
            ;;
    esac
done

for numeric in "${timeout_ms}" "${threads}" "${max_memory_mb}"; do
    if [[ ! "${numeric}" =~ ^[0-9]+$ ]]; then
        usage >&2
        exit 2
    fi
done

if (( timeout_ms < 1 || max_memory_mb < 1 )); then
    usage >&2
    exit 2
fi

if [[ ! -x "${probe_script}" ]]; then
    echo "v6 conservative root ordering sweep failed: probe script is not executable: ${probe_script}" >&2
    exit 1
fi

if [[ ! -x "${compare_script}" ]]; then
    echo "v6 conservative root ordering sweep failed: compare script is not executable: ${compare_script}" >&2
    exit 1
fi

IFS=',' read -r -a candidate_list <<< "${candidates}"
if (( ${#candidate_list[@]} < 1 )); then
    usage >&2
    exit 2
fi

for candidate in "${candidate_list[@]}"; do
    case "${candidate}" in
        reverse_tie|high_bound_first|phase2_tiebreak)
            ;;
        *)
            echo "unsupported root ordering candidate: ${candidate}" >&2
            exit 2
            ;;
    esac
done

mkdir -p "${output_dir}"
summary_file="${output_dir}/summary.csv"
{
    echo "candidate,common_cases,baseline_elapsed_ms,candidate_elapsed_ms,elapsed_delta_ms,elapsed_delta_percent,baseline_max_elapsed_ms,candidate_max_elapsed_ms,max_elapsed_delta_ms,baseline_nodes,candidate_nodes,nodes_delta,winner,comparison_file"
} > "${summary_file}"

for candidate in "${candidate_list[@]}"; do
    candidate_output_dir="${output_dir}/${candidate}"
    comparison_file="${candidate_output_dir}/comparison.csv"

    "${probe_script}" \
        --build-dir "${build_dir}" \
        --cache-dir "${cache_dir}" \
        --output-dir "${candidate_output_dir}" \
        --corpus-file "${corpus_file}" \
        --timeout-ms "${timeout_ms}" \
        --threads "${threads}" \
        --max-memory-mb "${max_memory_mb}" \
        --cache-mode require-warm \
        --candidate-env "RUBIK_EXPERIMENTAL_ROOT_ORDERING=${candidate}"

    "${compare_script}" \
        --baseline-dir "${candidate_output_dir}/default" \
        --candidate-dir "${candidate_output_dir}/candidate" \
        --output "${comparison_file}"

    python3 - "${comparison_file}" "${summary_file}" "${candidate}" <<'PY'
import csv
import sys

comparison_file, summary_file, candidate = sys.argv[1:]
with open(comparison_file, newline="") as handle:
    rows = list(csv.DictReader(handle))
summary_rows = [row for row in rows if row.get("case_key") == "__summary__"]
if len(summary_rows) != 1:
    print(f"comparison summary missing for {candidate}: {comparison_file}", file=sys.stderr)
    sys.exit(1)
row = summary_rows[0]
with open(summary_file, "a", newline="") as handle:
    writer = csv.writer(handle)
    writer.writerow([
        candidate,
        row["common_cases"],
        row["baseline_elapsed_ms"],
        row["candidate_elapsed_ms"],
        row["elapsed_delta_ms"],
        row["elapsed_delta_percent"],
        row["baseline_max_elapsed_ms"],
        row["candidate_max_elapsed_ms"],
        row["max_elapsed_delta_ms"],
        row["baseline_nodes"],
        row["candidate_nodes"],
        row["nodes_delta"],
        row["winner"],
        comparison_file,
    ])
PY
done

echo "v6 conservative root ordering sweep summary: ${summary_file}"
```

- [ ] **Step 2: Make runner executable**

Run:

```bash
chmod +x scripts/run_v6_conservative_root_ordering_sweep.sh
```

- [ ] **Step 3: Add CMake target**

Add near `rubik-benchmark-v6-conservative-root-probe` in `CMakeLists.txt`:

```cmake
add_custom_target(rubik-benchmark-v6-conservative-root-ordering-sweep
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_v6_conservative_root_ordering_sweep.sh
        --build-dir ${CMAKE_CURRENT_BINARY_DIR}
        --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache
        --output-dir ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results/v6-conservative-root-ordering-sweep
        --corpus-file ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/v6_conservative_root_corpus.csv
        --threads 0
        --max-memory-mb 2048
    DEPENDS rubik-bench rubik-cache-setup
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running V6 conservative-root root-ordering sweep"
)
```

- [ ] **Step 4: Verify GREEN**

Run:

```bash
cmake --preset release-native-lto
ctest --test-dir out/release-native-lto -R 'run_v6_conservative_root_ordering_sweep' --output-on-failure
```

Expected: all three sweep runner tests pass.

- [ ] **Step 5: Run full suite**

Run:

```bash
ctest --test-dir out/release-native-lto --output-on-failure
git diff --check
```

Expected: all tests pass and `git diff --check` prints no output.

- [ ] **Step 6: Commit runner**

Run:

```bash
git add CMakeLists.txt scripts/run_v6_conservative_root_ordering_sweep.sh tests/run_v6_conservative_root_ordering_sweep_rejects_missing_values.sh tests/run_v6_conservative_root_ordering_sweep_rejects_invalid_candidate.sh tests/run_v6_conservative_root_ordering_sweep_fake_tools.sh
git commit -m "Add V6 conservative root ordering sweep"
```

## Task 3: Run Real Sweep And Document Pass 41

**Files:**
- Create: `docs/v6-optimal-latency-pass41-2026-05-31.md`
- Modify: `docs/benchmarks.md`

- [ ] **Step 1: Run real sweep target**

Run:

```bash
RUBIK_BENCH_COMMAND_TIMEOUT_MS=45000 \
  cmake --build out/release-native-lto \
  --target rubik-benchmark-v6-conservative-root-ordering-sweep
```

Expected: target exits zero and writes `out/release-native-lto/benchmark-results/v6-conservative-root-ordering-sweep/summary.csv`.

- [ ] **Step 2: Inspect summary and cache state**

Run:

```bash
cat out/release-native-lto/benchmark-results/v6-conservative-root-ordering-sweep/summary.csv
for candidate in reverse_tie high_bound_first phase2_tiebreak; do
  echo "candidate=${candidate}"
  cat "out/release-native-lto/benchmark-results/v6-conservative-root-ordering-sweep/${candidate}/cache_setup.csv"
done
```

Expected: `summary.csv` has three data rows and each `cache_setup.csv` reports `cache_warm,true` and `bytes_missing,0`.

- [ ] **Step 3: Generate pass 41 report**

Run:

````bash
summary_csv="out/release-native-lto/benchmark-results/v6-conservative-root-ordering-sweep/summary.csv"
report="docs/v6-optimal-latency-pass41-2026-05-31.md"

cat > "${report}" <<'EOF'
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
requires a warm `large-local` cache and rejects cold-cache measurements.

## Summary

| Candidate | Cases | Default ms | Candidate ms | Delta ms | Delta % | Default max ms | Candidate max ms | Max delta ms | Default nodes | Candidate nodes | Node delta | Winner |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
EOF

awk -F, 'NR > 1 {
    printf "| `%s` | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | `%s` |\n", $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13
}' "${summary_csv}" >> "${report}"

best_candidate="$(
    awk -F, 'NR > 1 && $2 == 5 && $5 + 0 < 0 && $9 + 0 <= 0 {
        print $1 "," $5 "," $12
    }' "${summary_csv}" | sort -t, -k2,2n | head -1
)"

cat >> "${report}" <<EOF

## Decision

EOF

if [[ -n "${best_candidate}" ]]; then
    candidate_name="$(printf '%s' "${best_candidate}" | cut -d, -f1)"
    cat >> "${report}" <<EOF
The sweep found at least one candidate worth a separate policy experiment:
\`${candidate_name}\`. This pass does not change the default solver policy.
The next step is a narrow adaptive-policy candidate and a broader V6 corpus
validation.
EOF
else
    cat >> "${report}" <<'EOF'
Reject promoting these root-ordering candidates from this pass. No default
solver policy changes were made. The next step should inspect per-case root
diagnostics for a stronger discriminator.
EOF
fi
````

Expected: report exists and contains one summary table row per candidate.

- [ ] **Step 4: Verify report values**

Run:

```bash
test "$(awk 'BEGIN { count = 0 } /^\| `.*` \| [0-9]+ \| [0-9]+ \| [0-9]+ \| -?[0-9]+ \| -?[0-9.]+ \| [0-9]+ \| [0-9]+ \| -?[0-9]+ \| [0-9]+ \| [0-9]+ \| -?[0-9]+ \| `.*` \|$/ { count++ } END { print count }' docs/v6-optimal-latency-pass41-2026-05-31.md)" = "3"
```

Expected: `test` exits zero.

- [ ] **Step 5: Link pass 41**

Add one bullet in `docs/benchmarks.md` under current profile comparison:

```markdown
- [V6 Conservative-Root Ordering Sweep - 2026-05-31](v6-optimal-latency-pass41-2026-05-31.md)
```

- [ ] **Step 6: Verify docs**

Run:

```bash
ctest --test-dir out/release-native-lto -R 'public_docs_no_unverified_hardware_estimates|public_docs_current_version' --output-on-failure
git diff --check
```

Expected: docs tests pass and `git diff --check` prints no output.

- [ ] **Step 7: Commit report**

Run:

```bash
git add docs/v6-optimal-latency-pass41-2026-05-31.md docs/benchmarks.md
git commit -m "Record V6 conservative root ordering sweep"
```

## Task 4: Full Verification

**Files:**
- No new files.

- [ ] **Step 1: Run full CTest suite**

Run:

```bash
ctest --test-dir out/release-native-lto --output-on-failure
```

Expected: all tests pass.

- [ ] **Step 2: Run whitespace check**

Run:

```bash
git diff --check
```

Expected: no output.

- [ ] **Step 3: Confirm status**

Run:

```bash
git status --short --branch
```

Expected: clean worktree, branch ahead of origin, no push.
