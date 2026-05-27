# V4 CPU Tail-Latency Design - 2026-05-27

This document defines the V4 development direction for the Rubik cube solver
library. V4 is a CPU-only performance release focused on reducing optimal-mode
tail latency while preserving the certified minimum-move contract.

## Goal

V4 improves local `SolveMode::Optimal` latency on difficult cubes. The release
target is not a broader feature expansion: it is a measured performance release
for the existing optimal solver.

When a solve result reports `SolveStatus::Optimal`, the solution must still be
proven minimal in HTM. No V4 optimization may weaken admissible pruning,
physical validation, or the optimality proof.

## Non-Goals

V4 does not include:

- GPU acceleration;
- cloud or remote solving;
- QTM or additional move metrics;
- camera recognition;
- robot or hardware-control APIs;
- Raspberry Pi, Jetson, Orin, or other hardware claims without direct
  measurements on those devices.

## Baseline

The V4 baseline is the released V3 state. Before changing solver behavior, the
current V3 benchmark gates must be rerun locally and recorded so that V4
improvements can be compared against a clean starting point.

The relevant V3 local tail reference is:

- Auto large-local known depth-15 tail gate: `7/7` solved;
- highest recorded solver time in that gate: about `9.4 s`;
- highest recorded wall time in that gate: about `10.0 s`;
- current difficult seed to watch: `1009`.

These numbers are local desktop measurements only.

## Workstream 1: Tail Discovery

The first V4 task is to discover a broader set of difficult optimal cases before
optimizing individual seeds. The discovery run should generate deterministic
depth-15 scrambles, measure optimal solve latency, and retain the slowest cases
with enough metadata to replay them exactly.

Depth-16 probes may be added only if they can be bounded by timeouts and kept
separate from mandatory release gates. Depth-16 discovery must not block V4 if
the cost is too high.

Expected output:

- deterministic seed and scramble records;
- solver elapsed time and wall elapsed time;
- node counts;
- effective profile and thread count;
- cache state;
- selected slowest cases for repeatable replay.

## Workstream 2: V4 Tail Corpus

The slowest discovered cases become a repeatable V4 tail corpus. This corpus is
the main benchmark surface for V4 CPU optimizations.

The corpus should separate:

- short smoke coverage for quick correctness checks;
- fixed tail cases for local development;
- long discovery runs that are not required for every commit;
- release gates used before tagging V4.

The corpus must be deterministic. A candidate optimization is not accepted only
because it improves one known seed; it must be evaluated against the corpus.

## Workstream 3: Solver Diagnostics

V4 needs enough diagnostics to explain slow cases instead of treating latency as
a black box. Diagnostics should stay useful for benchmark tooling without making
the common public API noisy.

Useful diagnostic fields include:

- total expanded nodes;
- root candidate count;
- slowest roots;
- root ordering position of the eventual solution;
- per-root or per-bucket elapsed time where practical;
- worker utilization or imbalance indicators;
- effective profile, table profile, memory setting, and thread count.

Diagnostics must not change solver results.

## Workstream 4: CPU Optimization

V4 optimization should focus on areas that can reduce long-tail optimal latency:

1. Root scheduling: reduce worker imbalance when a small number of roots are
   much more expensive than the rest.
2. Root ordering: try more promising roots earlier without weakening optimality.
3. Auto policy: select more aggressive local CPU settings on capable desktops
   while keeping smaller systems conservative.
4. Low-level search efficiency: reduce avoidable overhead in hot paths only
   when benchmark data identifies a real bottleneck.

Any heuristic change must be validated against correctness tests and A/B
benchmarks. Root ordering may change latency, but it must not change the set of
solutions considered or the minimum-depth proof.

## Workstream 5: A/B Benchmarking

V4 needs an automatic comparison path that can compare:

- V3 baseline;
- current V4 candidate;
- best known V4 candidate.

The comparison should report median, p95, p99 where applicable, max latency,
node counts, and regressions by seed. A candidate should be rejected or revised
if it improves one tail case by causing broad regressions elsewhere.

Initial acceptance targets:

- reduce known depth-15 tail max below `8 s` solver time if the expanded corpus
  shows that target is realistic;
- improve tail-corpus average latency by at least `20%` versus the V3 baseline;
- avoid significant known-case regressions;
- preserve all optimality and validation guarantees.

The exact final gates should be set after the first expanded discovery run.

## Testing And Release Validation

V4 release validation must include:

- existing unit and integration tests;
- existing release check script;
- V3 benchmark gates to catch regressions;
- V4 tail corpus gates;
- consumer smoke test;
- source archive validation;
- public documentation check for unmeasured hardware claims.

The final release notes must include only measurements actually run on the
current host or on hardware that was directly tested.

## Public API Impact

V4 should avoid major API expansion unless diagnostics or benchmark
reproducibility require it. Existing common solve calls should remain simple.

Any new diagnostics should be additive, optional, and documented. Existing
`SolveMode::Optimal`, `SolveProfile::Auto`, and `SolveStatus::Optimal`
semantics must remain stable.

## Success Criteria

V4 is ready to release when:

- all correctness and release validation tests pass;
- the expanded V4 tail corpus is deterministic and documented;
- accepted CPU optimizations improve measured tail latency versus V3;
- `SolveStatus::Optimal` remains a certified minimum-move result;
- release documentation avoids unverified hardware claims;
- the repository contains no release-blocking cleanup items.
