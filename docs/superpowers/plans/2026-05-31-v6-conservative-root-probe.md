# V6 Conservative-Root Probe Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a repeatable V6 `conservative_root` probe workflow without changing the default solver policy.

**Architecture:** Add a fixed CSV corpus of measured slow `conservative_root` cases, a shell runner that replays the corpus through the existing benchmark binary with a required warm cache, and CTest coverage for argument and corpus validation. Later candidate solver experiments will plug into this runner through explicit environment variables or script options and will be compared with existing V6 comparison tooling.

**Tech Stack:** Bash, CMake/CTest, existing `rubik-bench` and `rubik-cache-setup` CLIs, CSV benchmark artifacts, C++20 solver unchanged in the first implementation task.

---

## File Structure

- `benchmarks/v6_conservative_root_corpus.csv`: committed corpus with the exact pass 39 cases to replay. Columns: `suite,seed,start_index,depth,count,expected_reason`.
- `scripts/run_v6_conservative_root_probe.sh`: new runner. Validates arguments and corpus rows, requires a warm `auto` cache, runs default replay into `default/`, optionally runs one candidate command environment into `candidate/`, and writes a manifest.
- `tests/run_v6_conservative_root_probe_rejects_missing_values.sh`: checks missing required option handling.
- `tests/run_v6_conservative_root_probe_rejects_invalid_corpus.sh`: checks invalid corpus row handling before any benchmark directories are created.
- `CMakeLists.txt`: registers tests and adds a non-gated `rubik-benchmark-v6-conservative-root-probe` target.
- `docs/v6-optimal-latency-pass40-2026-05-31.md`: measurement report after the runner exists and default-only replay has been validated.
- `docs/benchmarks.md`: links the pass 40 report and describes the new target.

## Task 1: Add Conservative-Root Corpus

**Files:**
- Create: `benchmarks/v6_conservative_root_corpus.csv`

- [ ] **Step 1: Create the corpus file**

Use the pass 39 slow `conservative_root` depth-15 cases. Add:

```csv
suite,seed,start_index,depth,count,expected_reason
hardening,42,1,15,1,conservative_root
tail,424242,1,15,1,conservative_root
hardening,424242,1,15,1,conservative_root
tail,99,1,15,1,conservative_root
hardening,99,1,15,1,conservative_root
```

- [ ] **Step 2: Verify CSV shape manually**

Run:

```bash
awk -F, 'NR == 1 { print $0; next } NF != 6 { print "bad row " NR; exit 1 } END { print "rows=" NR-1 }' benchmarks/v6_conservative_root_corpus.csv
```

Expected output includes:

```text
suite,seed,start_index,depth,count,expected_reason
rows=5
```

- [ ] **Step 3: Commit corpus**

Run:

```bash
git add benchmarks/v6_conservative_root_corpus.csv
git commit -m "Add V6 conservative root corpus"
```

## Task 2: Add Runner Contract Tests

**Files:**
- Create: `tests/run_v6_conservative_root_probe_rejects_missing_values.sh`
- Create: `tests/run_v6_conservative_root_probe_rejects_invalid_corpus.sh`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add missing-value test**

Create `tests/run_v6_conservative_root_probe_rejects_missing_values.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/run_v6_conservative_root_probe.sh}"

"${script}" --build-dir > /tmp/run_v6_conservative_root_probe_missing_value.out 2>&1 && {
    cat /tmp/run_v6_conservative_root_probe_missing_value.out >&2
    exit 1
}

grep -q "Usage: scripts/run_v6_conservative_root_probe.sh" /tmp/run_v6_conservative_root_probe_missing_value.out
```

- [ ] **Step 2: Add invalid-corpus test**

Create `tests/run_v6_conservative_root_probe_rejects_invalid_corpus.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/run_v6_conservative_root_probe.sh}"
repo_root="$(cd "$(dirname "${script}")/.." && pwd)"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "${tmp_dir}"' EXIT

cat > "${tmp_dir}/bad-corpus.csv" <<'CSV'
suite,seed,start_index,depth,count,expected_reason
bad_suite,42,1,15,1,conservative_root
CSV

set +e
"${script}" \
    --build-dir "${repo_root}/out/release-native-lto" \
    --cache-dir "${tmp_dir}/cache" \
    --output-dir "${tmp_dir}/out" \
    --corpus-file "${tmp_dir}/bad-corpus.csv" \
    --cache-mode require-warm \
    > "${tmp_dir}/run.out" 2>&1
status="$?"
set -e

if [[ "${status}" -eq 0 ]]; then
    cat "${tmp_dir}/run.out" >&2
    echo "expected invalid corpus rejection" >&2
    exit 1
fi

grep -q "unsupported corpus row" "${tmp_dir}/run.out"

if [[ -d "${tmp_dir}/out/default" || -d "${tmp_dir}/out/candidate" ]]; then
    find "${tmp_dir}/out" -maxdepth 2 -type d >&2
    echo "benchmark directories should not be created after corpus rejection" >&2
    exit 1
fi
```

- [ ] **Step 3: Make tests executable**

Run:

```bash
chmod +x tests/run_v6_conservative_root_probe_rejects_missing_values.sh \
  tests/run_v6_conservative_root_probe_rejects_invalid_corpus.sh
```

- [ ] **Step 4: Register tests in CMake**

Add near the existing V6 runner tests in `CMakeLists.txt`:

```cmake
add_test(
    NAME run_v6_conservative_root_probe_rejects_missing_values
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_v6_conservative_root_probe_rejects_missing_values.sh
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_v6_conservative_root_probe.sh
)
add_test(
    NAME run_v6_conservative_root_probe_rejects_invalid_corpus
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_v6_conservative_root_probe_rejects_invalid_corpus.sh
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_v6_conservative_root_probe.sh
)
```

- [ ] **Step 5: Verify RED**

Run:

```bash
cmake --preset release-native-lto
ctest --test-dir out/release-native-lto -R 'run_v6_conservative_root_probe_rejects_missing_values|run_v6_conservative_root_probe_rejects_invalid_corpus' --output-on-failure
```

Expected: tests fail because `scripts/run_v6_conservative_root_probe.sh` does not exist.

## Task 3: Implement Default-Only Probe Runner

**Files:**
- Create: `scripts/run_v6_conservative_root_probe.sh`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Create runner skeleton**

Create `scripts/run_v6_conservative_root_probe.sh` with:

```bash
#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"

build_dir="out/release-native-lto"
cache_dir="${RUBIK_TABLE_CACHE_DIR:-/tmp/rubik_cube_library_v6_tail_baseline_cache}"
output_dir="benchmark-results/v6-conservative-root-probe"
corpus_file="benchmarks/v6_conservative_root_corpus.csv"
timeout_ms="30000"
threads="0"
max_memory_mb="2048"
cache_mode="require-warm"
candidate_env=""

usage() {
    cat <<'USAGE'
Usage: scripts/run_v6_conservative_root_probe.sh [options]

Options:
  --build-dir DIR        CMake build directory, default: out/release-native-lto
  --cache-dir DIR        pruning table cache directory
  --output-dir DIR       output directory, default: benchmark-results/v6-conservative-root-probe
  --corpus-file FILE     CSV corpus, default: benchmarks/v6_conservative_root_corpus.csv
  --timeout-ms N         per-case timeout, default: 30000
  --threads N            solver threads, default: 0
  --max-memory-mb N      solver memory limit, default: 2048
  --cache-mode MODE      require-warm|reuse, default: require-warm
  --candidate-env ENV    optional KEY=VALUE candidate environment assignment
  -h, --help             show this help
USAGE
}

require_value() {
    if [[ $# -lt 2 || "$2" == -* ]]; then
        usage >&2
        exit 2
    fi
}
```

- [ ] **Step 2: Add argument parsing**

Append:

```bash
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
        --cache-mode)
            require_value "$@"
            cache_mode="$2"
            shift 2
            ;;
        --candidate-env)
            require_value "$@"
            candidate_env="$2"
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
```

- [ ] **Step 3: Add validation before build**

Append:

```bash
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

if [[ "${cache_mode}" != "require-warm" && "${cache_mode}" != "reuse" ]]; then
    usage >&2
    exit 2
fi

if [[ -n "${candidate_env}" && ! "${candidate_env}" =~ ^[A-Za-z_][A-Za-z0-9_]*=.*$ ]]; then
    usage >&2
    exit 2
fi

if [[ ! -f "${corpus_file}" ]]; then
    echo "v6 conservative root probe failed: corpus file not found: ${corpus_file}" >&2
    exit 1
fi
```

- [ ] **Step 4: Add corpus validation**

Append:

```bash
validate_corpus() {
    local file="$1"
    local line_no=0
    local rows=0
    while IFS=, read -r suite seed start_index depth count expected_reason; do
        line_no=$((line_no + 1))
        if (( line_no == 1 )); then
            if [[ "${suite},${seed},${start_index},${depth},${count},${expected_reason}" != "suite,seed,start_index,depth,count,expected_reason" ]]; then
                echo "v6 conservative root probe failed: invalid corpus header" >&2
                exit 1
            fi
            continue
        fi
        if [[ "${suite}" != "tail" && "${suite}" != "hardening" ]]; then
            echo "v6 conservative root probe failed: unsupported corpus row ${line_no}: ${suite},${seed},${start_index},${depth},${count},${expected_reason}" >&2
            exit 1
        fi
        if [[ ! "${seed}" =~ ^[0-9]+$ || ! "${start_index}" =~ ^[0-9]+$ || ! "${depth}" =~ ^[0-9]+$ || ! "${count}" =~ ^[0-9]+$ ]]; then
            echo "v6 conservative root probe failed: unsupported corpus row ${line_no}: ${suite},${seed},${start_index},${depth},${count},${expected_reason}" >&2
            exit 1
        fi
        if (( start_index < 1 || depth < 1 || count < 1 )); then
            echo "v6 conservative root probe failed: unsupported corpus row ${line_no}: ${suite},${seed},${start_index},${depth},${count},${expected_reason}" >&2
            exit 1
        fi
        if [[ "${expected_reason}" != "conservative_root" ]]; then
            echo "v6 conservative root probe failed: unsupported corpus row ${line_no}: ${suite},${seed},${start_index},${depth},${count},${expected_reason}" >&2
            exit 1
        fi
        rows=$((rows + 1))
    done < "${file}"

    if (( rows < 1 )); then
        echo "v6 conservative root probe failed: corpus contains no rows: ${file}" >&2
        exit 1
    fi
}

validate_corpus "${corpus_file}"
```

- [ ] **Step 5: Add cache preparation**

Append:

```bash
cmake --build "${build_dir}" --target rubik-bench rubik-cache-setup

bench="${build_dir}/rubik-bench"
cache_setup="${build_dir}/rubik-cache-setup"
if [[ ! -x "${bench}" || ! -x "${cache_setup}" ]]; then
    echo "required benchmark binaries are missing in ${build_dir}" >&2
    exit 1
fi

mkdir -p "${cache_dir}" "${output_dir}"

manifest_file="${output_dir}/manifest.csv"
cache_setup_output="${output_dir}/cache_setup.csv"
{
    echo "key,value"
    echo "git_revision,$(git -C "${repo_root}" rev-parse --short HEAD 2>/dev/null || echo unknown)"
    echo "build_dir,${build_dir}"
    echo "cache_dir,${cache_dir}"
    echo "output_dir,${output_dir}"
    echo "corpus_file,${corpus_file}"
    echo "timeout_ms,${timeout_ms}"
    echo "threads,${threads}"
    echo "max_memory_mb,${max_memory_mb}"
    echo "cache_mode,${cache_mode}"
    echo "candidate_env,${candidate_env}"
    echo "cache_setup_output,${cache_setup_output}"
} > "${manifest_file}"

if [[ "${cache_mode}" == "require-warm" ]]; then
    "${cache_setup}" \
        --profile auto \
        --threads "${threads}" \
        --max-memory-mb "${max_memory_mb}" \
        --cache-dir "${cache_dir}" \
        --dry-run \
        --format csv \
        | tee "${cache_setup_output}"
    cache_warm="$(awk -F, '$1 == "cache_setup" && $2 == "cache_warm" { print $3; exit }' "${cache_setup_output}")"
    bytes_missing="$(awk -F, '$1 == "cache_setup" && $2 == "bytes_missing" { print $3; exit }' "${cache_setup_output}")"
    if [[ "${cache_warm}" != "true" ]]; then
        echo "cache is not warm for V6 conservative root probe; run rubik-cache-setup first (bytes_missing=${bytes_missing:-unknown})" >&2
        exit 1
    fi
else
    {
        echo "cache_setup,status,Skipped"
        echo "cache_setup,message,cache setup skipped by cache-mode reuse"
    } > "${cache_setup_output}"
fi
```

- [ ] **Step 6: Add replay function**

Append:

```bash
run_variant() {
    local variant_name="$1"
    local env_assignment="$2"
    local variant_dir="${output_dir}/${variant_name}"
    local summary_file="${variant_dir}/summary.csv"
    mkdir -p "${variant_dir}"

    {
        echo "variant,suite,seed,start_index,depth,count,status,optimal,move_count,elapsed_ms,nodes_expanded,adaptive_reason,output_file"
    } > "${summary_file}"

    while IFS=, read -r suite seed start_index depth count expected_reason; do
        if [[ "${suite}" == "suite" ]]; then
            continue
        fi

        local name="v6_conservative_root_${suite}_depth_${depth}_seed_${seed}_start_${start_index}_count_${count}"
        local output_file="${variant_dir}/warm_${name}.csv"
        {
            echo "benchmark,name,${name}"
            echo "benchmark,variant,${variant_name}"
            echo "benchmark,suite,${suite}"
            echo "benchmark,expected_reason,${expected_reason}"
            echo "benchmark,cache_dir,${cache_dir}"
        } > "${output_file}"

        set +e
        if [[ -n "${env_assignment}" ]]; then
            env RUBIK_TABLE_CACHE_DIR="${cache_dir}" "${env_assignment}" "${bench}" \
                --mode optimal \
                --profile auto \
                --threads "${threads}" \
                --max-memory-mb "${max_memory_mb}" \
                --timeout-ms "${timeout_ms}" \
                --max-depth "${depth}" \
                --case-set random \
                --random-count "${count}" \
                --random-depth "${depth}" \
                --random-seed "${seed}" \
                --random-start-index "${start_index}" \
                --slowest-count "${count}" \
                --diagnose-optimal \
                >> "${output_file}"
        else
            env RUBIK_TABLE_CACHE_DIR="${cache_dir}" "${bench}" \
                --mode optimal \
                --profile auto \
                --threads "${threads}" \
                --max-memory-mb "${max_memory_mb}" \
                --timeout-ms "${timeout_ms}" \
                --max-depth "${depth}" \
                --case-set random \
                --random-count "${count}" \
                --random-depth "${depth}" \
                --random-seed "${seed}" \
                --random-start-index "${start_index}" \
                --slowest-count "${count}" \
                --diagnose-optimal \
                >> "${output_file}"
        fi
        local command_status="$?"
        set -e

        local status
        local optimal
        local move_count
        local elapsed
        local nodes
        local adaptive_reason
        status="$(awk -F, '$1 ~ /^random_/ { print $4; exit }' "${output_file}")"
        optimal="$(awk -F, '$1 ~ /^random_/ { print $5; exit }' "${output_file}")"
        move_count="$(awk -F, '$1 ~ /^random_/ { print $6; exit }' "${output_file}")"
        elapsed="$(awk -F, '$1 ~ /^random_/ { print $8; exit }' "${output_file}")"
        nodes="$(awk -F, '$1 ~ /^random_/ { print $9; exit }' "${output_file}")"
        adaptive_reason="$(
            awk -F, '$1 ~ /^random_/ { print $16; exit }' "${output_file}" \
                | sed -n 's/.*adaptive_reason=\([^;"]*\).*/\1/p'
        )"
        echo "${variant_name},${suite},${seed},${start_index},${depth},${count},${status:-Unknown},${optimal:-false},${move_count:-0},${elapsed:-0},${nodes:-0},${adaptive_reason:-unknown},${output_file}" >> "${summary_file}"

        if (( command_status != 0 )); then
            echo "v6 conservative root probe failed: benchmark command failed for ${name}" >&2
            exit 1
        fi
        if [[ "${status}" != "Optimal" || "${optimal}" != "true" ]]; then
            echo "v6 conservative root probe failed: non-optimal result for ${name}" >&2
            exit 1
        fi
        if [[ "${adaptive_reason}" != "${expected_reason}" ]]; then
            echo "v6 conservative root probe failed: expected ${expected_reason}, got ${adaptive_reason:-unknown} for ${name}" >&2
            exit 1
        fi
    done < "${corpus_file}"
}

run_variant "default" ""
if [[ -n "${candidate_env}" ]]; then
    run_variant "candidate" "${candidate_env}"
fi

echo "v6 conservative root probe manifest: ${manifest_file}"
echo "v6 conservative root default summary: ${output_dir}/default/summary.csv"
if [[ -n "${candidate_env}" ]]; then
    echo "v6 conservative root candidate summary: ${output_dir}/candidate/summary.csv"
fi
```

- [ ] **Step 7: Make runner executable**

Run:

```bash
chmod +x scripts/run_v6_conservative_root_probe.sh
```

- [ ] **Step 8: Add CMake target**

Add near `rubik-benchmark-v6-tail-baseline-require-warm` in `CMakeLists.txt`:

```cmake
add_custom_target(rubik-benchmark-v6-conservative-root-probe
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_v6_conservative_root_probe.sh
        --build-dir ${CMAKE_CURRENT_BINARY_DIR}
        --cache-dir /tmp/rubik_cube_library_v6_tail_baseline_cache
        --output-dir ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results/v6-conservative-root-probe
        --corpus-file ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/v6_conservative_root_corpus.csv
        --threads 0
        --max-memory-mb 2048
        --cache-mode require-warm
    DEPENDS rubik-bench rubik-cache-setup
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running V6 conservative-root local optimal probe"
)
```

- [ ] **Step 9: Verify GREEN**

Run:

```bash
cmake --preset release-native-lto
ctest --test-dir out/release-native-lto -R 'run_v6_conservative_root_probe_rejects_missing_values|run_v6_conservative_root_probe_rejects_invalid_corpus' --output-on-failure
```

Expected: both tests pass.

- [ ] **Step 10: Commit runner**

Run:

```bash
git add CMakeLists.txt scripts/run_v6_conservative_root_probe.sh tests/run_v6_conservative_root_probe_rejects_missing_values.sh tests/run_v6_conservative_root_probe_rejects_invalid_corpus.sh
git commit -m "Add V6 conservative root probe runner"
```

## Task 4: Run Default Probe And Document Pass 40

**Files:**
- Create: `docs/v6-optimal-latency-pass40-2026-05-31.md`
- Modify: `docs/benchmarks.md`

- [ ] **Step 1: Run the default probe**

Run:

```bash
RUBIK_BENCH_COMMAND_TIMEOUT_MS=45000 \
  cmake --build out/release-native-lto \
  --target rubik-benchmark-v6-conservative-root-probe
```

Expected: runner exits zero, writes `out/release-native-lto/benchmark-results/v6-conservative-root-probe/default/summary.csv`, and every row has `status=Optimal`, `optimal=true`, `adaptive_reason=conservative_root`.

- [ ] **Step 2: Inspect summary**

Run:

```bash
cat out/release-native-lto/benchmark-results/v6-conservative-root-probe/default/summary.csv
```

Expected: 5 data rows.

- [ ] **Step 3: Generate pass 40 report from artifacts**

Run:

````bash
cache_csv="out/release-native-lto/benchmark-results/v6-conservative-root-probe/cache_setup.csv"
summary_csv="out/release-native-lto/benchmark-results/v6-conservative-root-probe/default/summary.csv"
report="docs/v6-optimal-latency-pass40-2026-05-31.md"

cache_value() {
    awk -F, -v key="$1" '$1 == "cache_setup" && $2 == key { print $3; exit }' "${cache_csv}"
}

status="$(cache_value status)"
effective_profile="$(cache_value effective_profile)"
payload_bytes="$(cache_value payload_bytes)"
cache_warm="$(cache_value cache_warm)"
bytes_missing="$(cache_value bytes_missing)"
message="$(cache_value message)"

cat > "${report}" <<EOF
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
| Status | \`${status}\` |
| Effective profile | \`${effective_profile}\` |
| Payload bytes | ${payload_bytes} |
| Cache warm | ${cache_warm} |
| Bytes missing | ${bytes_missing} |
| Message | \`${message}\` |

## Results

| Suite | Seed | Depth | Solver ms | Nodes | Adaptive reason |
| --- | ---: | ---: | ---: | ---: | --- |
EOF

awk -F, 'NR > 1 {
    printf "| `%s` | %s | %s | %s | %s | `%s` |\n", $2, $3, $5, $10, $11, $12
}' "${summary_csv}" >> "${report}"

cat >> "${report}" <<'EOF'

## Decision

Keep the default solver policy unchanged. The probe runner is ready for the
first explicit candidate experiment.
EOF
````

Expected: `docs/v6-optimal-latency-pass40-2026-05-31.md` is created with cache
state copied from `cache_setup.csv` and five result rows copied from
`default/summary.csv`.

- [ ] **Step 4: Verify report values**

Run:

```bash
rg -n '<[^>]+>' docs/v6-optimal-latency-pass40-2026-05-31.md
test "$(awk 'BEGIN { count = 0 } /^\| `.*` \| [0-9]+ \| [0-9]+ \| [0-9]+ \| [0-9]+ \| `.*` \|$/ { count++ } END { print count }' docs/v6-optimal-latency-pass40-2026-05-31.md)" = "5"
```

Expected: `rg` prints no output and `test` exits zero.

- [ ] **Step 5: Link pass 40 in benchmark docs**

Add one bullet in `docs/benchmarks.md` current profile comparison:

```markdown
- [V6 Conservative-Root Probe - 2026-05-31](v6-optimal-latency-pass40-2026-05-31.md)
```

- [ ] **Step 6: Verify docs**

Run:

```bash
ctest --test-dir out/release-native-lto -R 'public_docs_no_unverified_hardware_estimates|public_docs_current_version' --output-on-failure
```

Expected: pass.

- [ ] **Step 7: Commit docs**

Run:

```bash
git add docs/v6-optimal-latency-pass40-2026-05-31.md docs/benchmarks.md
git commit -m "Record V6 conservative root probe baseline"
```

## Task 5: Full Verification

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
