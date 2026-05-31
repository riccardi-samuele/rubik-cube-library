# V6 Conservative-Root Diagnostics Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Analyze pass 41 `conservative_root` root-search diagnostics and document whether `phase2_tiebreak` has a narrow future-policy discriminator.

**Architecture:** Use the existing `scripts/analyze_root_search_profile.py` tool on pass 41 default and `phase2_tiebreak` candidate output. Generate untracked diagnostic CSV artifacts under `out/release-native-lto/benchmark-results/v6-conservative-root-diagnostics`, then commit only the pass 42 markdown report and benchmark index link.

**Tech Stack:** Python analyzer CLI, Bash/awk CSV checks, Markdown docs, CTest.

---

## File Structure

- `out/release-native-lto/benchmark-results/v6-conservative-root-diagnostics/default-summary.csv`: generated, untracked analyzer summary for pass 41 default output.
- `out/release-native-lto/benchmark-results/v6-conservative-root-diagnostics/candidate-summary.csv`: generated, untracked analyzer summary for pass 41 `phase2_tiebreak` candidate output.
- `out/release-native-lto/benchmark-results/v6-conservative-root-diagnostics/default-roots.csv`: generated, untracked per-root default diagnostics.
- `out/release-native-lto/benchmark-results/v6-conservative-root-diagnostics/candidate-roots.csv`: generated, untracked per-root `phase2_tiebreak` diagnostics.
- `out/release-native-lto/benchmark-results/v6-conservative-root-diagnostics/comparison.csv`: generated, untracked case-level comparison.
- `docs/v6-optimal-latency-pass42-2026-05-31.md`: committed report.
- `docs/benchmarks.md`: link to pass 42 report.

## Task 1: Generate Diagnostics Artifacts

**Files:**
- Generated only under `out/release-native-lto/benchmark-results/v6-conservative-root-diagnostics`

- [ ] **Step 1: Verify analyzer tests**

Run:

```bash
ctest --test-dir out/release-native-lto -R '^analyze_root_search_profile' --output-on-failure
```

Expected: analyzer tests pass.

- [ ] **Step 2: Create output directory**

Run:

```bash
mkdir -p out/release-native-lto/benchmark-results/v6-conservative-root-diagnostics
```

Expected: directory exists.

- [ ] **Step 3: Generate default summary**

Run:

```bash
scripts/analyze_root_search_profile.py \
  --input-dir out/release-native-lto/benchmark-results/v6-conservative-root-ordering-sweep/phase2_tiebreak/default \
  --reason conservative_root \
  --summary \
  --sort-by solver_elapsed_ms \
  --sort-desc \
  --output out/release-native-lto/benchmark-results/v6-conservative-root-diagnostics/default-summary.csv
```

Expected: command exits zero and writes five data rows.

- [ ] **Step 4: Generate candidate summary**

Run:

```bash
scripts/analyze_root_search_profile.py \
  --input-dir out/release-native-lto/benchmark-results/v6-conservative-root-ordering-sweep/phase2_tiebreak/candidate \
  --reason conservative_root \
  --summary \
  --sort-by solver_elapsed_ms \
  --sort-desc \
  --output out/release-native-lto/benchmark-results/v6-conservative-root-diagnostics/candidate-summary.csv
```

Expected: command exits zero and writes five data rows.

- [ ] **Step 5: Generate detailed root rows**

Run:

```bash
scripts/analyze_root_search_profile.py \
  --input-dir out/release-native-lto/benchmark-results/v6-conservative-root-ordering-sweep/phase2_tiebreak/default \
  --reason conservative_root \
  --sort-by root_elapsed_ms \
  --sort-desc \
  --output out/release-native-lto/benchmark-results/v6-conservative-root-diagnostics/default-roots.csv

scripts/analyze_root_search_profile.py \
  --input-dir out/release-native-lto/benchmark-results/v6-conservative-root-ordering-sweep/phase2_tiebreak/candidate \
  --reason conservative_root \
  --sort-by root_elapsed_ms \
  --sort-desc \
  --output out/release-native-lto/benchmark-results/v6-conservative-root-diagnostics/candidate-roots.csv
```

Expected: both commands exit zero and write root-level CSV files.

- [ ] **Step 6: Build case comparison CSV**

Run:

```bash
python3 - <<'PY'
import csv
from pathlib import Path

root = Path("out/release-native-lto/benchmark-results/v6-conservative-root-diagnostics")
default_path = root / "default-summary.csv"
candidate_path = root / "candidate-summary.csv"
output_path = root / "comparison.csv"

def load(path):
    with path.open(newline="") as handle:
        rows = list(csv.DictReader(handle))
    if not rows:
        raise SystemExit(f"no rows in {path}")
    return {row["case_name"]: row for row in rows}

def integer(row, key):
    value = row.get(key, "")
    return int(value) if value else 0

default = load(default_path)
candidate = load(candidate_path)
common = sorted(set(default) & set(candidate))
if len(common) != 5:
    raise SystemExit(f"expected 5 common cases, got {len(common)}")

fieldnames = [
    "case_name",
    "default_solver_ms",
    "candidate_solver_ms",
    "solver_delta_ms",
    "default_solution_root_ms",
    "candidate_solution_root_ms",
    "solution_root_delta_ms",
    "default_before_solution_ms",
    "candidate_before_solution_ms",
    "before_solution_delta_ms",
    "default_max_root_ms",
    "candidate_max_root_ms",
    "max_root_delta_ms",
    "default_before_solution_elapsed_share_ppm",
    "candidate_before_solution_elapsed_share_ppm",
    "before_solution_elapsed_share_delta_ppm",
    "default_solution_root_nodes_share_ppm",
    "candidate_solution_root_nodes_share_ppm",
    "solution_root_nodes_share_delta_ppm",
    "default_worker_imbalance_ms",
    "candidate_worker_imbalance_ms",
    "worker_imbalance_delta_ms",
]

with output_path.open("w", newline="") as handle:
    writer = csv.DictWriter(handle, fieldnames=fieldnames, lineterminator="\n")
    writer.writeheader()
    for case_name in common:
        base = default[case_name]
        cand = candidate[case_name]
        row = {
            "case_name": case_name,
            "default_solver_ms": integer(base, "solver_elapsed_ms"),
            "candidate_solver_ms": integer(cand, "solver_elapsed_ms"),
            "default_solution_root_ms": integer(base, "solution_root_elapsed_ms"),
            "candidate_solution_root_ms": integer(cand, "solution_root_elapsed_ms"),
            "default_before_solution_ms": integer(base, "before_solution_root_elapsed_ms"),
            "candidate_before_solution_ms": integer(cand, "before_solution_root_elapsed_ms"),
            "default_max_root_ms": integer(base, "max_root_elapsed_ms"),
            "candidate_max_root_ms": integer(cand, "max_root_elapsed_ms"),
            "default_before_solution_elapsed_share_ppm": integer(base, "before_solution_elapsed_share_ppm"),
            "candidate_before_solution_elapsed_share_ppm": integer(cand, "before_solution_elapsed_share_ppm"),
            "default_solution_root_nodes_share_ppm": integer(base, "solution_root_nodes_share_ppm"),
            "candidate_solution_root_nodes_share_ppm": integer(cand, "solution_root_nodes_share_ppm"),
            "default_worker_imbalance_ms": integer(base, "worker_elapsed_imbalance_ms"),
            "candidate_worker_imbalance_ms": integer(cand, "worker_elapsed_imbalance_ms"),
        }
        row["solver_delta_ms"] = row["candidate_solver_ms"] - row["default_solver_ms"]
        row["solution_root_delta_ms"] = row["candidate_solution_root_ms"] - row["default_solution_root_ms"]
        row["before_solution_delta_ms"] = row["candidate_before_solution_ms"] - row["default_before_solution_ms"]
        row["max_root_delta_ms"] = row["candidate_max_root_ms"] - row["default_max_root_ms"]
        row["before_solution_elapsed_share_delta_ppm"] = (
            row["candidate_before_solution_elapsed_share_ppm"] -
            row["default_before_solution_elapsed_share_ppm"]
        )
        row["solution_root_nodes_share_delta_ppm"] = (
            row["candidate_solution_root_nodes_share_ppm"] -
            row["default_solution_root_nodes_share_ppm"]
        )
        row["worker_imbalance_delta_ms"] = (
            row["candidate_worker_imbalance_ms"] -
            row["default_worker_imbalance_ms"]
        )
        writer.writerow(row)
print(f"diagnostic comparison: {output_path}")
PY
```

Expected: command exits zero and prints the comparison path.

- [ ] **Step 7: Verify generated CSV counts**

Run:

```bash
for file in default-summary.csv candidate-summary.csv comparison.csv; do
  test "$(($(wc -l < "out/release-native-lto/benchmark-results/v6-conservative-root-diagnostics/${file}") - 1))" = "5"
done
```

Expected: command exits zero.

## Task 2: Document Pass 42

**Files:**
- Create: `docs/v6-optimal-latency-pass42-2026-05-31.md`
- Modify: `docs/benchmarks.md`

- [ ] **Step 1: Inspect comparison**

Run:

```bash
cat out/release-native-lto/benchmark-results/v6-conservative-root-diagnostics/comparison.csv
```

Expected: five data rows.

- [ ] **Step 2: Create pass report**

Use the exact values from `comparison.csv` to create `docs/v6-optimal-latency-pass42-2026-05-31.md` with:

```markdown
# V6 optimal latency pass 42

## Goal

Inspect root-search diagnostics for the pass 41 `phase2_tiebreak` near miss on
the `conservative_root` depth-15 probe.

## Commands

```bash
scripts/analyze_root_search_profile.py \
  --input-dir out/release-native-lto/benchmark-results/v6-conservative-root-ordering-sweep/phase2_tiebreak/default \
  --reason conservative_root \
  --summary \
  --sort-by solver_elapsed_ms \
  --sort-desc \
  --output out/release-native-lto/benchmark-results/v6-conservative-root-diagnostics/default-summary.csv

scripts/analyze_root_search_profile.py \
  --input-dir out/release-native-lto/benchmark-results/v6-conservative-root-ordering-sweep/phase2_tiebreak/candidate \
  --reason conservative_root \
  --summary \
  --sort-by solver_elapsed_ms \
  --sort-desc \
  --output out/release-native-lto/benchmark-results/v6-conservative-root-diagnostics/candidate-summary.csv
```

## Cache State

The input artifacts come from pass 41. The pass 41 sweep recorded a warm
`large-local` cache with `bytes_missing=0` for the `phase2_tiebreak` default and
candidate runs.

## Diagnostic Comparison

| Case | Default ms | Candidate ms | Delta ms | Default before solution ms | Candidate before solution ms | Before delta ms | Default solution root ms | Candidate solution root ms | Solution delta ms | Max root delta ms | Worker imbalance delta ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
```

Append one row per `comparison.csv` row with the exact measured values.

- [ ] **Step 3: Add decision section**

Append:

```markdown
## Decision

Do not promote a `phase2_tiebreak` policy from these diagnostics alone. The pass
41 aggregate was still slower by `19 ms`, and the per-case diagnostics must
show a stable discriminator before a policy experiment is justified.

The next useful step is to mine the detailed root rows for cases where
before-solution work dominates and then create a smaller candidate rule only if
that pattern is consistent.
```

- [ ] **Step 4: Verify report**

Run:

```bash
test "$(awk 'BEGIN { count = 0 } /^\| `.*` \| [0-9]+ \| [0-9]+ \| -?[0-9]+ \| [0-9]+ \| [0-9]+ \| -?[0-9]+ \| [0-9]+ \| [0-9]+ \| -?[0-9]+ \| -?[0-9]+ \| -?[0-9]+ \|$/ { count++ } END { print count }' docs/v6-optimal-latency-pass42-2026-05-31.md)" = "5"
```

Expected: `test` exits zero.

- [ ] **Step 5: Link pass 42**

Add one bullet in `docs/benchmarks.md` under current profile comparison:

```markdown
- [V6 Conservative-Root Diagnostics - 2026-05-31](v6-optimal-latency-pass42-2026-05-31.md)
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
git add docs/v6-optimal-latency-pass42-2026-05-31.md docs/benchmarks.md
git commit -m "Record V6 conservative root diagnostics"
```

## Task 3: Full Verification

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
