# V4 Adaptive Deep Split Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build and measure an experimental adaptive optimal scheduler that uses deep root splitting only when cheap local signals predict it should reduce tail latency.

**Architecture:** Keep the existing default optimal path unchanged. Add a small deterministic policy in `src/solver.cpp` that chooses `root` or `deep_split` when `RUBIK_EXPERIMENTAL_ADAPTIVE_DEEP_SPLIT=1` is set, expose the decision in `rootOrderingProfile`, and extend benchmark tooling to compare baseline, unconditional deep split, and adaptive deep split on the same V4 corpus.

**Tech Stack:** C++20, existing IDA* solver in `src/solver.cpp`, CTest, Bash benchmark wrappers, Python CSV comparison tooling.

---

## File Structure

- Modify `src/solver.cpp`: add adaptive env flag, decision struct, decision formatter, conservative policy, and route selection.
- Modify `tests/rubik_tests.cpp`: add adaptive diagnostics and correctness tests.
- Modify `scripts/run_v4_deep_split_ab.sh`: support a third adaptive run and compare all three variants.
- Modify `scripts/compare_v4_tail_runs.py`: include max elapsed and solved row counts in the summary so release decisions are not average-only.
- Modify `tests/fixtures/benchmark-results/v4_baseline.csv` and `tests/fixtures/benchmark-results/v4_candidate.csv` only if needed for comparison summary expectations.
- Modify `CMakeLists.txt`: add/adjust tests for the adaptive runner and comparison output.
- Create `docs/v4-adaptive-deep-split-results-2026-05-28.md`: record measured results and release decision.

## Task 1: Add Adaptive Decision Diagnostics Test

**Files:**
- Modify: `tests/rubik_tests.cpp`

- [ ] **Step 1: Add failing adaptive diagnostics test**

Add this test near `testExperimentalDeepRootSplitReportsDiagnostics()`:

```cpp
void testExperimentalAdaptiveDeepSplitReportsDecision()
{
    rubik::Cube cube = rubik::Cube::solved();
    cube.apply(rubik::parseMoves("R U F D"));

    const rubik::Solver solver;
    setenv("RUBIK_EXPERIMENTAL_ADAPTIVE_DEEP_SPLIT", "1", 1);
    const auto result = solver.solve(cube, {
        .mode = rubik::SolveMode::Optimal,
        .maxDepth = 8,
        .threads = 4,
        .profile = rubik::SolveProfile::Default,
        .collectDiagnostics = true,
    });
    unsetenv("RUBIK_EXPERIMENTAL_ADAPTIVE_DEEP_SPLIT");

    expect(result.status == rubik::SolveStatus::Optimal);
    expect(result.isOptimal);
    expect(result.moveCount == 4);
    expect(result.plan.rootOrderingProfile.find("scheduler=adaptive") != std::string::npos);
    expect(result.plan.rootOrderingProfile.find("adaptive_decision=") != std::string::npos);
    expect(result.plan.rootOrderingProfile.find("adaptive_reason=") != std::string::npos);
}
```

- [ ] **Step 2: Register the test**

In `main()`, add:

```cpp
testExperimentalAdaptiveDeepSplitReportsDecision();
```

Place it immediately after:

```cpp
testExperimentalDeepRootSplitReportsDiagnostics();
```

- [ ] **Step 3: Verify the test fails**

Run:

```bash
cmake --build out/release-native-lto --target rubik_tests
ctest --test-dir out/release-native-lto -R rubik_tests --output-on-failure
```

Expected: `rubik_tests` fails because `scheduler=adaptive` is not emitted yet.

## Task 2: Implement Conservative Adaptive Policy

**Files:**
- Modify: `src/solver.cpp`

- [ ] **Step 1: Add adaptive env flag**

Near `experimentalDeepRootSplitEnabled()`, add:

```cpp
bool experimentalAdaptiveDeepRootSplitEnabled()
{
    return environmentFlagEnabled("RUBIK_EXPERIMENTAL_ADAPTIVE_DEEP_SPLIT");
}
```

- [ ] **Step 2: Add decision types**

Near `RootOrderingProfile`, add:

```cpp
enum class OptimalSchedulerDecision {
    Root,
    DeepSplit,
};

struct AdaptiveDeepSplitDecision {
    OptimalSchedulerDecision scheduler = OptimalSchedulerDecision::Root;
    std::string reason = "default_root";
    int initialLowerBound = 0;
    int maxDepth = 0;
    unsigned int threads = 0;
    int strongMinCount = 0;
    bool firstMoveDiffers = false;
};
```

- [ ] **Step 3: Add formatter**

Near `formatDeepRootSplitProfile`, add:

```cpp
std::string formatAdaptiveDeepSplitDecision(const AdaptiveDeepSplitDecision& decision)
{
    std::ostringstream out;
    out << ";scheduler=adaptive"
        << ";adaptive_decision="
        << (decision.scheduler == OptimalSchedulerDecision::DeepSplit ? "deep_split" : "root")
        << ";adaptive_reason=" << decision.reason
        << ";adaptive_lb=" << decision.initialLowerBound
        << ";adaptive_max_depth=" << decision.maxDepth
        << ";adaptive_threads=" << decision.threads
        << ";adaptive_strong_min_count=" << decision.strongMinCount
        << ";adaptive_first_diff=" << (decision.firstMoveDiffers ? 1 : 0);
    return out.str();
}
```

- [ ] **Step 4: Add first conservative policy**

Near the formatter, add:

```cpp
AdaptiveDeepSplitDecision chooseAdaptiveDeepSplit(
    int initialLowerBound,
    const SolveOptions& options,
    const RootOrderingProfile& rootOrderingProfile)
{
    AdaptiveDeepSplitDecision decision{
        .scheduler = OptimalSchedulerDecision::Root,
        .reason = "conservative_root",
        .initialLowerBound = initialLowerBound,
        .maxDepth = options.maxDepth,
        .threads = options.threads,
        .strongMinCount = rootOrderingProfile.strongMinCount,
        .firstMoveDiffers = rootOrderingProfile.firstMoveDiffers,
    };

    const int remainingDepth = options.maxDepth - initialLowerBound;
    if (options.threads < 4) {
        decision.reason = "threads_lt_4";
        return decision;
    }
    if (remainingDepth < 5) {
        decision.reason = "remaining_depth_lt_5";
        return decision;
    }
    if (initialLowerBound >= 9 && rootOrderingProfile.strongMinCount >= 4) {
        decision.scheduler = OptimalSchedulerDecision::DeepSplit;
        decision.reason = "high_lb_broad_strong_min";
        return decision;
    }

    return decision;
}
```

This rule is deliberately conservative and deterministic. It is expected to be tuned after A/B results.

- [ ] **Step 5: Route adaptive scheduler**

In `Solver::solve`, after `useDeepRootSplit`, add:

```cpp
const bool useAdaptiveDeepRootSplit = effectiveOptions.mode == SolveMode::Optimal &&
    effectiveOptions.threads > 1 &&
    experimentalAdaptiveDeepRootSplitEnabled();
```

After `rootOrderingProfile` and move-ordering setup, compute:

```cpp
AdaptiveDeepSplitDecision adaptiveDeepSplitDecision;
if (useAdaptiveDeepRootSplit) {
    adaptiveDeepSplitDecision = chooseAdaptiveDeepSplit(
        initialLowerBound,
        effectiveOptions,
        rootOrderingProfile);
}
const bool useSelectedDeepRootSplit = useDeepRootSplit ||
    (useAdaptiveDeepRootSplit &&
     adaptiveDeepSplitDecision.scheduler == OptimalSchedulerDecision::DeepSplit);
```

Replace the solve-loop branch:

```cpp
const SearchState result = useDeepRootSplit
```

with:

```cpp
const SearchState result = useSelectedDeepRootSplit
```

When appending diagnostics after `solutionRootOrderingProfile(...)`, add:

```cpp
if (useAdaptiveDeepRootSplit) {
    plan.publicPlan.rootOrderingProfile += formatAdaptiveDeepSplitDecision(adaptiveDeepSplitDecision);
}
if (useSelectedDeepRootSplit) {
    plan.publicPlan.rootOrderingProfile += formatDeepRootSplitProfile(splitTaskCount);
}
```

and remove the older `if (useDeepRootSplit)` formatter block so `deep_root_split=enabled` follows the selected scheduler.

- [ ] **Step 6: Verify tests pass**

Run:

```bash
cmake --build out/release-native-lto --target rubik_tests
ctest --test-dir out/release-native-lto -R "rubik_tests|cli_bench_reports_root_ordering_profile" --output-on-failure
```

Expected: all selected tests pass.

- [ ] **Step 7: Commit policy**

Run:

```bash
git add src/solver.cpp tests/rubik_tests.cpp
git commit -m "Add experimental adaptive deep split policy"
```

Expected: commit succeeds only after tests pass.

## Task 3: Extend Benchmark Comparison Summary

**Files:**
- Modify: `scripts/compare_v4_tail_runs.py`
- Modify: `tests/fixtures/benchmark-results/v4_baseline.csv`
- Modify: `tests/fixtures/benchmark-results/v4_candidate.csv`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add failing expectation for max summary fields**

In `CMakeLists.txt`, update the `compare_v4_tail_runs_fixture` expectation from:

```cmake
PASS_REGULAR_EXPRESSION "__summary__,8411,7300,-1111,-13.21,59590724,52000000,-7590724,candidate"
```

to:

```cmake
PASS_REGULAR_EXPRESSION "__summary__,8411,7300,-1111,-13.21,9846,8600,-1246,59590724,52000000,-7590724,candidate"
```

- [ ] **Step 2: Verify it fails**

Run:

```bash
ctest --test-dir out/release-native-lto -R compare_v4_tail_runs_fixture --output-on-failure
```

Expected: fails because max elapsed columns are not emitted.

- [ ] **Step 3: Add max fields to comparison output**

In `scripts/compare_v4_tail_runs.py`, change each row dictionary in `compare()` to include:

```python
"baseline_max_elapsed_ms": baseline_elapsed,
"candidate_max_elapsed_ms": candidate_elapsed,
"max_elapsed_delta_ms": elapsed_delta,
```

For the `__summary__` row, compute:

```python
baseline_max_elapsed = max(row["baseline_elapsed_ms"] for row in rows)
candidate_max_elapsed = max(row["candidate_elapsed_ms"] for row in rows)
```

and add:

```python
"baseline_max_elapsed_ms": baseline_max_elapsed,
"candidate_max_elapsed_ms": candidate_max_elapsed,
"max_elapsed_delta_ms": candidate_max_elapsed - baseline_max_elapsed,
```

Update `fieldnames` to:

```python
fieldnames = [
    "seed",
    "baseline_elapsed_ms",
    "candidate_elapsed_ms",
    "elapsed_delta_ms",
    "elapsed_delta_percent",
    "baseline_max_elapsed_ms",
    "candidate_max_elapsed_ms",
    "max_elapsed_delta_ms",
    "baseline_nodes",
    "candidate_nodes",
    "nodes_delta",
    "winner",
]
```

- [ ] **Step 4: Run comparison tests**

Run:

```bash
ctest --test-dir out/release-native-lto -R "compare_v4_tail_runs|run_v4_deep_split_ab" --output-on-failure
```

Expected: all selected tests pass.

- [ ] **Step 5: Commit comparison summary**

Run:

```bash
git add scripts/compare_v4_tail_runs.py CMakeLists.txt
git commit -m "Report max elapsed in V4 tail comparisons"
```

Expected: commit succeeds.

## Task 4: Add Three-Way V4 Runner

**Files:**
- Modify: `scripts/run_v4_deep_split_ab.sh`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add help contract expectation**

In `CMakeLists.txt`, update `run_v4_deep_split_ab_help` to expect the variants:

```cmake
set_tests_properties(run_v4_deep_split_ab_help PROPERTIES
    PASS_REGULAR_EXPRESSION "Variants: baseline, deep-split, adaptive"
)
```

- [ ] **Step 2: Verify help test fails**

Run:

```bash
ctest --test-dir out/release-native-lto -R run_v4_deep_split_ab_help --output-on-failure
```

Expected: fails because the runner help does not mention variants yet.

- [ ] **Step 3: Update runner usage**

In `scripts/run_v4_deep_split_ab.sh`, add this line to `usage()` after options:

```bash
  Variants: baseline, deep-split, adaptive
```

- [ ] **Step 4: Add adaptive output directory and comparison file**

After:

```bash
candidate_dir="${output_dir}/deep-split"
comparison_file="${output_dir}/comparison.csv"
```

add:

```bash
adaptive_dir="${output_dir}/adaptive"
adaptive_comparison_file="${output_dir}/adaptive-comparison.csv"
deep_split_comparison_file="${output_dir}/deep-split-comparison.csv"
```

Use `deep_split_comparison_file` for baseline vs unconditional deep split, and use `adaptive_comparison_file` for baseline vs adaptive.

- [ ] **Step 5: Add adaptive run**

After the unconditional deep-split run, add:

```bash
RUBIK_EXPERIMENTAL_ADAPTIVE_DEEP_SPLIT=1 "${script_dir}/run_v4_tail_corpus.sh" \
    --cases-file "${cases_file}" \
    --build-dir "${build_dir}" \
    --cache-dir "${cache_dir}" \
    --output-dir "${adaptive_dir}" \
    --timeout-ms "${timeout_ms}" \
    --max-depth "${max_depth}" \
    --threads "${threads}" \
    --max-memory-mb "${max_memory_mb}" \
    --slowest-limit "${slowest_limit}" \
    --cache-mode reuse
```

- [ ] **Step 6: Compare both candidates**

Replace the existing comparison call with:

```bash
"${script_dir}/compare_v4_tail_runs.py" \
    --baseline "${baseline_dir}/summary.csv" \
    --candidate "${candidate_dir}/summary.csv" \
    --output "${deep_split_comparison_file}"

"${script_dir}/compare_v4_tail_runs.py" \
    --baseline "${baseline_dir}/summary.csv" \
    --candidate "${adaptive_dir}/summary.csv" \
    --output "${adaptive_comparison_file}"

cp "${adaptive_comparison_file}" "${comparison_file}"
```

Keep `comparison.csv` as an alias for the adaptive comparison because adaptive is the V4 candidate.

- [ ] **Step 7: Update final output messages**

At the end of the script, print:

```bash
echo "v4 deep split baseline summary: ${baseline_dir}/summary.csv"
echo "v4 deep split candidate summary: ${candidate_dir}/summary.csv"
echo "v4 adaptive summary: ${adaptive_dir}/summary.csv"
echo "v4 deep split comparison: ${deep_split_comparison_file}"
echo "v4 adaptive comparison: ${adaptive_comparison_file}"
echo "v4 active comparison alias: ${comparison_file}"
```

- [ ] **Step 8: Run runner contract tests**

Run:

```bash
ctest --test-dir out/release-native-lto -R "run_v4_deep_split_ab|compare_v4_tail_runs" --output-on-failure
```

Expected: all selected tests pass.

- [ ] **Step 9: Commit runner**

Run:

```bash
git add scripts/run_v4_deep_split_ab.sh CMakeLists.txt
git commit -m "Add adaptive variant to V4 deep split runner"
```

Expected: commit succeeds.

## Task 5: Run Three-Way V4 Benchmark

**Files:**
- Create: `docs/v4-adaptive-deep-split-results-2026-05-28.md`

- [ ] **Step 1: Run the benchmark**

Run:

```bash
scripts/run_v4_deep_split_ab.sh \
  --cases-file out/release-native-lto/benchmark-results/v4-tail-discovery/slowest.csv \
  --build-dir out/release-native-lto \
  --cache-dir /tmp/rubik_cube_library_v4_adaptive_deep_split_cache \
  --output-dir out/release-native-lto/benchmark-results/v4-adaptive-deep-split \
  --threads 0 \
  --max-memory-mb 2048 \
  --cache-mode warm
```

Expected:

- `baseline/summary.csv` exists;
- `deep-split/summary.csv` exists;
- `adaptive/summary.csv` exists;
- `deep-split-comparison.csv` exists;
- `adaptive-comparison.csv` exists;
- every row in all summaries reports `Optimal,true`.

- [ ] **Step 2: Inspect results**

Run:

```bash
cat out/release-native-lto/benchmark-results/v4-adaptive-deep-split/adaptive-comparison.csv
cat out/release-native-lto/benchmark-results/v4-adaptive-deep-split/deep-split-comparison.csv
grep -h "scheduler=adaptive" out/release-native-lto/benchmark-results/v4-adaptive-deep-split/adaptive/*.csv | head
```

Expected: adaptive output includes `scheduler=adaptive`, `adaptive_decision=...`, and `adaptive_reason=...`.

- [ ] **Step 3: Document benchmark result**

Create `docs/v4-adaptive-deep-split-results-2026-05-28.md`. Use this structure,
and fill each comparison section with the full CSV emitted by the matching
command:

```bash
cat out/release-native-lto/benchmark-results/v4-adaptive-deep-split/adaptive-comparison.csv
cat out/release-native-lto/benchmark-results/v4-adaptive-deep-split/deep-split-comparison.csv
```

```markdown
# V4 Adaptive Deep Split Results - 2026-05-28

## Scope

- Runner: `scripts/run_v4_deep_split_ab.sh`
- Output directory: `out/release-native-lto/benchmark-results/v4-adaptive-deep-split`
- Variants: baseline, unconditional deep split, adaptive deep split
- Threads: `0` auto
- Memory limit: `2048` MB
- Timeout: `30000` ms

This is a local desktop benchmark only. It contains no Raspberry Pi, Jetson,
Orin, or other external hardware measurements.

## Adaptive Comparison

## Unconditional Deep Split Comparison

## Decision

Decision: choose exactly one of `promote_adaptive`, `tune_once`,
`keep_experimental`, or `remove_experiment`.

Rationale:

- Average latency result.
- Max latency result.
- Correctness result.
- Regression result.
```

Before committing the result document, confirm both CSV headers exist in the
document:

```bash
grep -c "seed,baseline_elapsed_ms,candidate_elapsed_ms" docs/v4-adaptive-deep-split-results-2026-05-28.md
```

Expected: `2`.

- [ ] **Step 4: Validate docs and tests**

Run:

```bash
ctest --test-dir out/release-native-lto -R "rubik_tests|run_v4_deep_split_ab|compare_v4_tail_runs|public_docs_no_unverified_hardware_estimates" --output-on-failure
```

Expected: all selected tests pass.

- [ ] **Step 5: Commit results**

Run:

```bash
git add docs/v4-adaptive-deep-split-results-2026-05-28.md
git commit -m "Record V4 adaptive deep split benchmark results"
```

Expected: commit succeeds.

## Task 6: Tune Or Promote Based On Data

**Files:**
- Modify if tuning: `src/solver.cpp`, `tests/rubik_tests.cpp`, `docs/v4-adaptive-deep-split-results-2026-05-28.md`
- Modify if promoting: `src/solver.cpp`, `docs/roadmap.md`, `docs/v4-adaptive-deep-split-results-2026-05-28.md`

- [ ] **Step 1: Decide using measured criteria**

Use `adaptive-comparison.csv`.

Promote adaptive only if:

- `__summary__` candidate average is less than or equal to baseline average;
- `__summary__` candidate max elapsed is less than baseline max elapsed;
- all adaptive rows are `Optimal,true`;
- no individual adaptive regression is severe enough to invalidate the release goal.

If the first policy fails, tune thresholds once using the measured failing seeds and repeat Task 5. Do not tune indefinitely in the same release step.

- [ ] **Step 2A: If tuning once**

Adjust only constants in `chooseAdaptiveDeepSplit(...)`, for example:

```cpp
if (initialLowerBound >= 9 && rootOrderingProfile.strongMinCount >= 6) {
    decision.scheduler = OptimalSchedulerDecision::DeepSplit;
    decision.reason = "high_lb_wide_strong_min";
    return decision;
}
```

Run:

```bash
cmake --build out/release-native-lto --target rubik_tests
ctest --test-dir out/release-native-lto -R rubik_tests --output-on-failure
```

Commit:

```bash
git add src/solver.cpp tests/rubik_tests.cpp
git commit -m "Tune V4 adaptive deep split policy"
```

Then repeat Task 5.

- [ ] **Step 2B: If promoting**

Keep `RUBIK_EXPERIMENTAL_ADAPTIVE_DEEP_SPLIT=1` available, but enable the adaptive policy automatically only for `SolveMode::Optimal`, `SolveProfile::Auto`, and `threads > 1`.

Record the promoted behavior in docs. Do not make embedded or external hardware claims.

Run:

```bash
ctest --test-dir out/release-native-lto -R "rubik_tests|public_docs_no_unverified_hardware_estimates" --output-on-failure
```

Commit:

```bash
git add src/solver.cpp docs/roadmap.md docs/v4-adaptive-deep-split-results-2026-05-28.md
git commit -m "Promote V4 adaptive deep split policy"
```

- [ ] **Step 2C: If rejecting**

Leave adaptive behind the experimental flag only if it remains useful for future diagnostics and has zero default runtime impact. Otherwise remove it.

Commit one of:

```bash
git commit -m "Keep V4 adaptive deep split experimental"
git commit -m "Remove rejected V4 adaptive deep split experiment"
```

## Task 7: V4 Release Gate Pass

**Files:**
- Modify: `docs/roadmap.md`
- Create: `docs/release-4.0.0.md`
- Create: `docs/github-release-v4.0.0.md`

- [ ] **Step 1: Run correctness and packaging checks**

Run:

```bash
ctest --test-dir out/release-native-lto --output-on-failure
scripts/release_check.sh --profile quick
```

Expected: all tests and quick release checks pass.

- [ ] **Step 2: Run V4 benchmark gate**

Run the accepted V4 benchmark command from Task 5 or the final promoted benchmark target.

Expected:

- all rows `Optimal,true`;
- accepted average and max thresholds met;
- no timeout rows.

- [ ] **Step 3: Write release notes**

Create `docs/release-4.0.0.md` and `docs/github-release-v4.0.0.md`.

The notes must include:

- measured local benchmark improvement;
- exact benchmark scope;
- no Raspberry Pi, Jetson, Orin, GPU, or cloud claims;
- compatibility notes;
- validation commands run.

- [ ] **Step 4: Final docs validation**

Run:

```bash
ctest --test-dir out/release-native-lto -R public_docs_no_unverified_hardware_estimates --output-on-failure
```

Expected: pass.

- [ ] **Step 5: Commit release prep**

Run:

```bash
git add docs/roadmap.md docs/release-4.0.0.md docs/github-release-v4.0.0.md
git commit -m "Prepare V4 release notes"
```

Expected: commit succeeds.

## Success Criteria

- Default behavior is unchanged until measured promotion.
- Adaptive scheduler preserves `SolveStatus::Optimal` semantics.
- Benchmark tooling compares baseline, unconditional deep split, and adaptive.
- V4 decision is based on local measured average and max latency.
- Public docs include no unverified external hardware claims.
- Each implementation block is tested and committed before the next block starts.
