# V4 Deep Root Splitting Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Test whether splitting expensive optimal root searches into deeper CPU tasks reduces V4 tail latency, especially seed `1009`, without changing the certified optimality contract.

**Architecture:** Add an experimental parallel search path guarded by an environment variable. The default solver remains unchanged; the experiment expands root candidates into depth-2 prefix tasks, schedules those tasks dynamically across workers, aggregates diagnostics back to the original root profile, and compares the result against the existing V4 corpus.

**Tech Stack:** C++20, existing `src/solver.cpp` IDA* search, CTest, `rubik-bench`, V4 corpus scripts, Python CSV analysis tooling.

---

## File Structure

- `src/solver.cpp`: add the experimental deep-split scheduler, env flag, and diagnostics.
- `tests/rubik_tests.cpp`: add correctness and diagnostics coverage for the experimental path.
- `scripts/run_v4_deep_split_ab.sh`: run baseline vs deep-split over the V4 corpus cases.
- `tests/run_v4_deep_split_ab_rejects_missing_values.sh`: validate script argument handling.
- `CMakeLists.txt`: register tests and optional benchmark target.
- `docs/v4-deep-root-splitting-results-2026-05-28.md`: record measured results and decision.

## Design Constraints

- Default behavior must not change.
- The experiment is enabled only with `RUBIK_EXPERIMENTAL_DEEP_ROOT_SPLIT=1`.
- `SolveStatus::Optimal` must still mean proven-minimal HTM solution.
- The experiment may change which optimal solution is returned when multiple minimal solutions exist, but `moveCount` must remain minimal.
- A candidate is accepted only if it improves V4 corpus tail latency without meaningful regressions.

## Task 1: Add Failing Experimental Diagnostics Test

**Files:**
- Modify: `tests/rubik_tests.cpp`

- [x] **Step 1: Add test function**

Add a test near the existing root diagnostics tests:

```cpp
void testExperimentalDeepRootSplitReportsDiagnostics()
{
    rubik::Cube cube = rubik::Cube::solved();
    cube.apply(rubik::parseMoves("R U F D"));

    const rubik::Solver solver;
    setenv("RUBIK_EXPERIMENTAL_DEEP_ROOT_SPLIT", "1", 1);
    const auto result = solver.solve(cube, {
        .mode = rubik::SolveMode::Optimal,
        .maxDepth = 8,
        .threads = 4,
        .profile = rubik::SolveProfile::Default,
        .collectDiagnostics = true,
    });
    unsetenv("RUBIK_EXPERIMENTAL_DEEP_ROOT_SPLIT");

    expect(result.status == rubik::SolveStatus::Optimal);
    expect(result.isOptimal);
    expect(result.moveCount == 4);
    expect(result.plan.rootOrderingProfile.find("deep_root_split=enabled") != std::string::npos);
    expect(result.plan.rootOrderingProfile.find("split_tasks=") != std::string::npos);
    expect(result.plan.rootOrderingProfile.find("worker_search=") != std::string::npos);
}
```

- [x] **Step 2: Register test call**

Add this call in `main()` with the other solver diagnostics tests:

```cpp
testExperimentalDeepRootSplitReportsDiagnostics();
```

- [x] **Step 3: Run and verify failure**

Run:

```bash
cmake --build out/release-native-lto --target rubik_tests
ctest --test-dir out/release-native-lto -R rubik_tests --output-on-failure
```

Expected: `rubik_tests` fails because `deep_root_split=enabled` is not reported yet.

## Task 2: Add Deep-Split Experimental Flag

**Files:**
- Modify: `src/solver.cpp`

- [x] **Step 1: Add flag reader**

Add a helper near the other environment-flag helpers:

```cpp
bool experimentalDeepRootSplitEnabled()
{
    return envFlagEnabled("RUBIK_EXPERIMENTAL_DEEP_ROOT_SPLIT");
}
```

- [x] **Step 2: Thread flag into solve**

In `Solver::solve`, read:

```cpp
const bool useDeepRootSplit = experimentalDeepRootSplitEnabled() && effectiveOptions.threads > 1;
```

Use this only to choose between the existing `parallelRootDfs` and the new experimental function added in Task 3.

- [x] **Step 3: Run compile check**

Run:

```bash
cmake --build out/release-native-lto --target rubik_tests
```

Expected: build passes; test still fails until Task 3.

## Task 3: Implement Experimental Depth-2 Root Task Scheduler

**Files:**
- Modify: `src/solver.cpp`

- [x] **Step 1: Add task type**

Add near `WorkerSearchProfileEntry`:

```cpp
struct DeepRootTask {
    int rootIndex = 0;
    Move rootMove = Move::U;
    SearchNode node;
    std::vector<Move> prefix;
};
```

- [x] **Step 2: Build split tasks**

Add a helper that receives the already sorted root candidates and expands each root by one more legal move using `collectCandidateMoves`. For each accepted child, create a `DeepRootTask` with prefix `{rootMove, childMove}` and depth `2`.

If a root has no depth-2 child tasks, add one fallback task with prefix `{rootMove}` and depth `1`. This preserves completeness when pruning removes all depth-2 children.

- [x] **Step 3: Add `parallelDeepRootDfs`**

Implement a sibling of `parallelRootDfs` with this behavior:

- it performs the same root candidate collection and sorting as `parallelRootDfs`;
- it builds `DeepRootTask` entries;
- workers use `std::atomic_int nextTask`;
- each task calls existing `dfs(task.node, task.prefix.size(), limit, ...)`;
- found solution sets `stopRequested`;
- timeout sets `timedOut`;
- root diagnostics aggregate per original root index;
- worker diagnostics aggregate per worker;
- `rootOrderingProfile` appends:

```text
;deep_root_split=enabled;split_depth=2;split_tasks=<N>
```

- [x] **Step 4: Route experimental path**

In `Solver::solve`, use:

```cpp
const SearchState result = effectiveOptions.threads > 1
    ? (useDeepRootSplit
        ? parallelDeepRootDfs(...)
        : parallelRootDfs(...))
    : dfs(...);
```

Expected: default path is byte-for-byte behaviorally unchanged unless the env var is set.

- [x] **Step 5: Run correctness tests**

Run:

```bash
ctest --test-dir out/release-native-lto -R "rubik_tests|cli_solve_accepts_threads|cli_bench_reports_root_ordering_profile" --output-on-failure
```

Expected: all pass.

- [x] **Step 6: Commit experimental scheduler**

Run:

```bash
git add src/solver.cpp tests/rubik_tests.cpp
git commit -m "Add experimental deep root splitting"
```

Expected: commit succeeds only if tests pass.

## Task 4: Add V4 Deep-Split A/B Runner

**Files:**
- Create: `scripts/run_v4_deep_split_ab.sh`
- Create: `tests/run_v4_deep_split_ab_rejects_missing_values.sh`
- Modify: `CMakeLists.txt`

- [x] **Step 1: Add missing-value test**

Create `tests/run_v4_deep_split_ab_rejects_missing_values.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail

script="${1:-scripts/run_v4_deep_split_ab.sh}"

"${script}" --cases-file > /tmp/run_v4_deep_split_ab_missing_value.out 2>&1 && {
    cat /tmp/run_v4_deep_split_ab_missing_value.out >&2
    exit 1
}

grep -q "Usage: scripts/run_v4_deep_split_ab.sh" /tmp/run_v4_deep_split_ab_missing_value.out
```

- [x] **Step 2: Create runner**

Create `scripts/run_v4_deep_split_ab.sh`. It should:

- accept `--cases-file`, `--build-dir`, `--cache-dir`, `--output-dir`, `--threads`, `--max-memory-mb`, `--timeout-ms`;
- run `scripts/run_v4_tail_corpus.sh` once as baseline with no experimental env var;
- run it once as candidate with `RUBIK_EXPERIMENTAL_DEEP_ROOT_SPLIT=1`;
- compare `baseline/summary.csv` and `candidate/summary.csv` using `scripts/compare_v4_tail_runs.py`;
- write `comparison.csv` at the A/B output root.

Expected output files:

```text
<output-dir>/baseline/summary.csv
<output-dir>/deep-split/summary.csv
<output-dir>/comparison.csv
```

- [x] **Step 3: Register CTest**

Add:

```cmake
add_test(
    NAME run_v4_deep_split_ab_rejects_missing_values
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_v4_deep_split_ab_rejects_missing_values.sh
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_v4_deep_split_ab.sh
)
```

- [x] **Step 4: Validate script**

Run:

```bash
chmod +x scripts/run_v4_deep_split_ab.sh tests/run_v4_deep_split_ab_rejects_missing_values.sh
cmake --preset release-native-lto
ctest --test-dir out/release-native-lto -R run_v4_deep_split_ab_rejects_missing_values --output-on-failure
scripts/run_v4_deep_split_ab.sh --help
```

Expected: CTest passes and help prints the usage line.

- [x] **Step 5: Commit runner**

Run:

```bash
git add scripts/run_v4_deep_split_ab.sh tests/run_v4_deep_split_ab_rejects_missing_values.sh CMakeLists.txt
git commit -m "Add V4 deep split A/B runner"
```

Expected: commit succeeds.

## Task 5: Run A/B On V4 Corpus

**Files:**
- Create: `docs/v4-deep-root-splitting-results-2026-05-28.md`

- [x] **Step 1: Run benchmark**

Run:

```bash
scripts/run_v4_deep_split_ab.sh \
  --build-dir out/release-native-lto \
  --cases-file out/release-native-lto/benchmark-results/v4-tail-discovery/slowest.csv \
  --cache-dir /tmp/rubik_cube_library_v4_tail_discovery_cache \
  --output-dir out/release-native-lto/benchmark-results/v4-deep-split-ab \
  --threads 0 \
  --max-memory-mb 2048 \
  --timeout-ms 30000
```

Expected: all baseline and candidate cases solve optimally.

- [x] **Step 2: Inspect comparison**

Run:

```bash
sed -n '1,40p' out/release-native-lto/benchmark-results/v4-deep-split-ab/comparison.csv
```

Acceptance target:

- candidate improves seed `1009`;
- candidate improves or ties max solver latency;
- candidate does not regress average latency by more than `5%`;
- every candidate row remains `Optimal,true`.

- [x] **Step 3: Document result**

Create `docs/v4-deep-root-splitting-results-2026-05-28.md` with:

- command;
- configuration;
- comparison table;
- decision: promote, keep experimental, or reject;
- statement that no embedded hardware measurements are included.

- [x] **Step 4: Run validation**

Run:

```bash
ctest --test-dir out/release-native-lto -R "rubik_tests|run_v4|compare_v4|public_docs_no_unverified_hardware_estimates" --output-on-failure
```

Expected: all pass.

- [x] **Step 5: Commit result**

Run:

```bash
git add docs/v4-deep-root-splitting-results-2026-05-28.md
git commit -m "Record V4 deep root splitting experiment"
```

Expected: commit succeeds.

## Task 6: Decide Promotion Or Rejection

**Files:**
- Modify if promoted: `src/solver.cpp`, `docs/v4-tail-corpus-2026-05-27.md`, `docs/roadmap.md`
- Modify if rejected: `docs/v4-deep-root-splitting-results-2026-05-28.md`

- [x] **Step 1: If promoted**

Keep the feature behind `RUBIK_EXPERIMENTAL_DEEP_ROOT_SPLIT=1` unless the A/B result is clearly better on max and average latency. If clearly better, consider enabling it for `SolveProfile::Auto` only in desktop tail conditions.

- [x] **Step 2: If rejected**

Leave the code behind the experimental flag only if it is useful for future research and does not add runtime overhead to default solving. Otherwise remove the experimental code and keep only the documented negative result.

- [x] **Step 3: Final targeted validation**

Run:

```bash
ctest --test-dir out/release-native-lto -R "rubik_tests|public_docs_no_unverified_hardware_estimates" --output-on-failure
```

Expected: all pass.

- [x] **Step 4: Commit decision**

Run one of:

```bash
git commit -m "Promote V4 deep root splitting policy"
git commit -m "Keep V4 deep root splitting experimental"
git commit -m "Remove rejected V4 deep root splitting experiment"
```

Expected: commit message matches the measured decision.

## Success Criteria

- Default solver behavior remains unchanged before a promotion decision.
- Experimental path preserves optimality on tests and corpus cases.
- A/B data determines promotion or rejection.
- Public docs include only measured local desktop data.
- No Raspberry Pi, Jetson, Orin, or other hardware claims are introduced.
