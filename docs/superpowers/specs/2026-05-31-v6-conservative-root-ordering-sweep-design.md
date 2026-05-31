# V6 Conservative-Root Ordering Sweep Design

## Purpose

Pass 40 added a narrow `conservative_root` depth-15 probe and measured five
slow local optimal cases. The next step is to use that probe to test root
ordering candidates that already exist behind experimental flags, before
changing any default solver policy.

This step is measurement-only. It must preserve the optimality contract and
must not promote a new policy unless a later pass validates a clearly winning
candidate on a broader corpus.

## Scope

In scope:

- run the pass 40 `conservative_root` probe with the current default policy;
- run the same probe with each existing experimental root ordering:
  `reverse_tie`, `high_bound_first`, and `phase2_tiebreak`;
- compare each candidate against the same default run;
- document measured solver time, node count, max latency, and decision;
- keep all generated benchmark artifacts outside git.

Out of scope:

- changing `chooseAdaptiveRootOrdering()` in this step;
- changing deep-root split policy;
- adding new root-ordering algorithms;
- changing public APIs, versioning, examples, or release docs;
- adding unmeasured hardware estimates or non-library planning notes.

## Current Evidence

The pass 40 default probe measured:

| Suite | Seed | Depth | Solver ms | Nodes | Reason |
| --- | ---: | ---: | ---: | ---: | --- |
| `hardening` | 42 | 15 | 4086 | 13450281 | `conservative_root` |
| `tail` | 424242 | 15 | 2430 | 10252468 | `conservative_root` |
| `hardening` | 424242 | 15 | 2382 | 10225559 | `conservative_root` |
| `tail` | 99 | 15 | 2560 | 6094772 | `conservative_root` |
| `hardening` | 99 | 15 | 2573 | 6205122 | `conservative_root` |

Earlier passes provide guardrails:

- pass 23 rejected a narrow `phase2_tiebreak` promotion for `lb=9`,
  `strongMinCount=14` after the full corpus regressed;
- pass 31 showed broad `high_bound_first` can help one bucket but regress
  `conservative_root`;
- pass 34 rejected broad `high_bound_first` for `lb9_low_strong_min`;
- pass 35 rejected broad forced deep splitting for `conservative_root`.

Therefore this sweep can select a candidate for deeper study, but it cannot
alone justify changing the default policy.

## Proposed Approach

Use the existing probe runner as the measurement harness. Add one small shell
runner that:

1. verifies the large-local cache is warm through the probe's existing
   `require-warm` path;
2. runs the default probe once;
3. runs three candidate probes, each using `--candidate-env` with one
   `RUBIK_EXPERIMENTAL_ROOT_ORDERING` value;
4. compares `default/` and `candidate/` output with `scripts/compare_v6_latency.py`;
5. writes one summary CSV that ranks candidates by total solver-time delta and
   max solver-time delta.

The runner should not modify solver behavior. It is a benchmark orchestration
layer around existing flags.

## Candidate Set

The sweep includes exactly these candidates:

| Candidate | Environment |
| --- | --- |
| `reverse_tie` | `RUBIK_EXPERIMENTAL_ROOT_ORDERING=reverse_tie` |
| `high_bound_first` | `RUBIK_EXPERIMENTAL_ROOT_ORDERING=high_bound_first` |
| `phase2_tiebreak` | `RUBIK_EXPERIMENTAL_ROOT_ORDERING=phase2_tiebreak` |

No new experimental flag is introduced in this step.

## Data Flow

Input:

- `benchmarks/v6_conservative_root_corpus.csv`;
- warm `/tmp/rubik_cube_library_v6_tail_baseline_cache`;
- `out/release-native-lto` build;
- `scripts/run_v6_conservative_root_probe.sh`;
- `scripts/compare_v6_latency.py`.

Generated output:

- `out/release-native-lto/benchmark-results/v6-conservative-root-ordering-sweep/{candidate}/default/`;
- `out/release-native-lto/benchmark-results/v6-conservative-root-ordering-sweep/{candidate}/candidate/`;
- per-candidate comparison CSV files;
- one aggregate sweep summary CSV;
- one pass report in `docs/`.

Generated benchmark directories remain untracked.

## Acceptance Gate

A candidate is only eligible for a later policy experiment if all conditions
are true on the five-case probe:

- all candidate rows are `Optimal` and `optimal=true`;
- common case count is five;
- total solver time is lower than default by at least 10%;
- max solver time does not increase;
- total node count does not increase by more than 5%;
- no individual case regresses by more than 15%.

If no candidate satisfies every condition, the pass report must reject all
ordering candidates and leave the default solver policy unchanged.

If one candidate satisfies every condition, the pass report may recommend a
separate later policy experiment. That later experiment must still be tested
against a broader V6 corpus before promotion.

## Error Handling

The sweep runner should fail early when:

- any required argument is missing;
- the candidate list contains an unsupported root-ordering value;
- the probe runner exits non-zero;
- comparison output has no `__summary__` row;
- a candidate produces non-optimal or timed-out rows.

Failure messages should include the candidate name and the artifact path.

## Testing

Add focused tests before running the real sweep:

- missing-value rejection for the sweep runner;
- unsupported candidate rejection;
- fixture or smoke validation that the runner invokes the existing probe with
  `--candidate-env RUBIK_EXPERIMENTAL_ROOT_ORDERING={candidate}`;
- existing comparison tests stay green;
- full CTest suite passes before committing tooling changes.

The real sweep is a benchmark target, not a gated CTest.

## Documentation

The pass report should include:

- command run;
- cache state from the probe output;
- one table per candidate with total solver delta, max solver delta, node
  delta, and winner;
- the acceptance-gate decision;
- a clear next step.

The report must only describe measured library behavior.
