# V4 Adaptive Optimal Latency Design

## Goal

V4 is a local CPU performance release for `SolveMode::Optimal`. The release goal
is to reduce difficult-case optimal solve latency while preserving the certified
minimum-move HTM contract.

## Current State

The project already has:

- a deterministic V4 tail discovery runner;
- a repeatable V4 tail corpus;
- A/B comparison tooling;
- root search, worker search, and bound diagnostics;
- an experimental `RUBIK_EXPERIMENTAL_DEEP_ROOT_SPLIT=1` scheduler;
- measured evidence that unconditional deep root splitting improves several
  slowest cases but regresses the corpus average.

The latest deep-split result showed:

- baseline average solver time: `3909` ms;
- unconditional deep-split average solver time: `4489` ms;
- average regression: `14.84%`;
- candidate wins on the five slowest measured cases;
- severe candidate regressions on seeds `99`, `888`, and `666`.

## Design Direction

V4 should not promote unconditional deep root splitting. The next candidate
optimization is an adaptive policy that chooses between the existing root-level
parallel search and the deep split scheduler per solve depth/case.

The policy must be conservative by default:

- if the policy is uncertain, use the existing scheduler;
- deep split may be enabled only when cheap pre-search signals suggest likely
  root-worker imbalance or high-tail behavior;
- the policy must not weaken pruning bounds, cube validation, or optimality
  proof semantics.

## Candidate Signals

The adaptive policy may use only cheap signals available before expensive search:

- initial lower bound;
- root candidate count;
- root bound histogram;
- strong-bound minimum count;
- solution-rank proxy from root ordering diagnostics where available;
- selected profile and thread count;
- max depth and remaining depth;
- previous measured thresholds recorded by local benchmark data.

The first implementation should avoid complex learned models or persistent
runtime tuning. A simple deterministic rule is preferable until benchmark data
proves a more complex policy is needed.

## Scheduler Behavior

The solver should support three internal strategies:

- `root`: current default `parallelRootDfs`;
- `deep_split`: current experimental depth-2 task scheduler;
- `adaptive`: choose `root` or `deep_split` using the V4 policy.

The public default should remain unchanged until A/B data proves the adaptive
policy is better on both average and tail metrics. During development, the
adaptive path should be activated by an experimental environment variable, for
example `RUBIK_EXPERIMENTAL_ADAPTIVE_DEEP_SPLIT=1`.

Diagnostics should record the chosen strategy, the decision reason, and any
threshold values used. This makes benchmark regressions explainable.

## Benchmark Plan

Every candidate must be compared against:

- current baseline scheduler;
- unconditional deep split;
- adaptive deep split.

Required benchmark outputs:

- per-case solver elapsed time;
- wall elapsed time;
- node count;
- move count and `Optimal,true` status;
- chosen scheduler;
- decision reason;
- summary row with average, max, and winner.

The V4 candidate should be accepted only if:

- every case still solves as `Optimal,true`;
- average latency is not worse than the baseline;
- max/tail latency improves versus the baseline;
- no individual regression is large enough to undermine the release goal;
- public docs include only measurements actually run locally.

## Release Readiness

V4 is ready when:

- correctness tests pass;
- release checks pass;
- V4 benchmark gates pass;
- accepted optimization behavior is documented;
- experimental paths that are not part of the release are either clearly guarded
  or removed;
- no unverified hardware performance claims are present in public docs.

## Non-Goals

V4 does not include:

- GPU acceleration;
- remote/cloud solving;
- robot APIs;
- camera recognition;
- QTM or other new metrics;
- Raspberry Pi, Jetson, Orin, or other hardware claims without direct
  measurements.
