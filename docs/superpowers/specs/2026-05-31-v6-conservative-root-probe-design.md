# V6 Conservative-Root Probe Design

## Purpose

V6 has reduced the worst measured local optimal tail latency, but pass 39 shows
that the remaining slow group is still `conservative_root` depth-15. Pass 35
already rejected broad forced deep splitting for that group, so the next step is
not to promote another scheduler policy directly. The next step is to build a
small, repeatable probe that can test narrow hypotheses on the exact slow
cluster.

This design keeps the certified HTM optimality contract unchanged.

## Scope

In scope:

- derive a replayable `conservative_root` probe corpus from the measured pass 39
  slow cases;
- add a script or benchmark target that replays the corpus with the current
  default solver policy;
- support one experimental variant at a time through an explicit environment
  flag or script option;
- compare default and candidate output with existing V6 comparison tooling;
- document the result as a V6 pass report before promoting any solver change.

Out of scope:

- changing the default solver policy in this step;
- adding hardware-specific latency claims;
- changing the public API;
- changing the optimality contract;
- adding GPU, cloud, QTM, camera, UI, or robot-control features.

## Current Evidence

Pass 39 refreshed the local V6 baseline through the `require-warm` benchmark
target. The current slowest case is hardening seed `42`, depth 15, at `2700 ms`.

The reason-group comparison against pass 20 shows:

| Reason | Pass 20 ms | Current ms | Delta |
| --- | ---: | ---: | ---: |
| `lb8_stable_mid_strong_min` | 7468 | 3559 | -3909 |
| `conservative_root` | 10371 | 11961 | +1590 |
| `lb9_mid_strong_min` | 4357 | 4957 | +600 |
| `lb9_low_strong_min` | 3320 | 3854 | +534 |
| `depth14_conservative_root` | 1967 | 2226 | +259 |

Pass 35 rejected broad forced deep root splitting for `conservative_root`
because it increased the measured target subset from `4411 ms` to `8251 ms`.

## Proposed Approach

Create a narrow V6 probe workflow for `conservative_root` instead of modifying
the default scheduler immediately.

The workflow has three pieces:

1. A small corpus file listing the pass 39 slow `conservative_root` cases by
   suite, seed, depth, case count, and expected reason.
2. A runner that executes the corpus twice: default policy and one explicit
   experimental candidate.
3. A comparison/report step that groups by adaptive reason and records accept or
   reject evidence.

The first candidate should be selected after the corpus runner exists. Candidate
examples include a narrower root ordering tie-break or a stricter deep-split
gate, but the runner must land first so each candidate can be rejected cheaply.

## Data Flow

Input:

- warm large-local cache at `/tmp/rubik_cube_library_v6_tail_baseline_cache`;
- release-native-LTO build;
- fixed corpus file in the repository;
- optional experimental environment flag for a candidate variant.

Execution:

1. Require warm cache before solving.
2. Run the default corpus replay into a baseline output directory.
3. Run the candidate corpus replay into a candidate output directory.
4. Compare outputs with `scripts/compare_v6_latency.py`.
5. Write a pass report from the CSV output.

Output:

- per-case CSV files;
- summary CSV files;
- comparison CSV;
- pass report documenting the decision.

Generated benchmark CSV files stay outside git.

## Acceptance Gate

A candidate may be promoted only if all conditions are met on the corpus:

- every case remains `SolveStatus::Optimal`;
- common case count matches the default run;
- total `conservative_root` solver time improves by at least 10%;
- max `conservative_root` solver time does not regress;
- p95 or equivalent high-tail measure does not regress;
- total node count does not increase by more than 5%;
- no new public hardware claims are added.

If a candidate fails any gate, the pass report must say rejected and the default
solver policy remains unchanged.

## Error Handling

The runner should fail early when:

- the cache is not warm;
- the build directory is missing required binaries;
- a corpus row has an unsupported suite, depth, or seed format;
- comparison finds no common cases;
- a candidate run times out or returns non-optimal status.

Failure output should point to the generated CSV or log artifact that explains
the reason.

## Testing

Add focused tests before implementation:

- missing-value rejection for the new runner;
- invalid corpus row rejection;
- fixture comparison for the corpus format if a parser is introduced;
- existing V6 comparison tests must remain green;
- full CTest suite must pass before any commit that changes tooling or solver
  behavior.

No solver behavior changes are allowed in the first implementation task.

## Documentation

The first implementation pass should add a V6 pass report that records only:

- commands run;
- cache state;
- local workstation measurements;
- accept or reject decision;
- next candidate if applicable.

The report must not include non-library goals, private planning details, or
unmeasured hardware estimates.
