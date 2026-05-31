# V6 Conservative-Root Diagnostics Design

## Purpose

Pass 41 rejected broad root-ordering promotion for `conservative_root`.
`phase2_tiebreak` was close enough to deserve inspection: it reduced max solver
time and nodes, but total solver time still regressed slightly. The next step
is to analyze root-search diagnostics from the pass 41 artifacts to identify a
stronger discriminator before any solver policy change.

This step is measurement-only. It must not change solver behavior.

## Scope

In scope:

- analyze pass 41 default and `phase2_tiebreak` candidate artifacts;
- use `scripts/analyze_root_search_profile.py` to extract per-case root
  summaries and per-root rows;
- compare solution rank, before-solution work, max-root work, worker imbalance,
  and root pruning counters;
- document whether the diagnostics suggest a narrow future policy experiment;
- keep generated CSV artifacts outside git.

Out of scope:

- changing root ordering policy;
- adding new solver heuristics;
- changing public API, examples, versioning, or release docs;
- adding unmeasured hardware claims or non-library planning notes.

## Inputs

The pass uses existing artifacts from:

```text
out/release-native-lto/benchmark-results/v6-conservative-root-ordering-sweep/phase2_tiebreak/default
out/release-native-lto/benchmark-results/v6-conservative-root-ordering-sweep/phase2_tiebreak/candidate
```

The inputs are valid only if the corresponding pass 41 cache setup recorded
`cache_warm=true` and `bytes_missing=0`.

## Proposed Approach

Run the existing analyzer twice:

1. default root policy summary and detailed root rows;
2. `phase2_tiebreak` summary and detailed root rows.

Then create a compact comparison CSV keyed by `case_name`. The comparison
should include:

- solver elapsed delta;
- solution root elapsed delta;
- before-solution elapsed delta;
- max-root elapsed delta;
- before-solution node share delta;
- solution root node share delta;
- worker imbalance delta.

The report should highlight only measured patterns that can support or reject a
future candidate.

## Acceptance Gate

This pass cannot promote a policy. It can only recommend a future experiment
when the diagnostics show a repeatable-looking pattern, such as:

- `phase2_tiebreak` wins when before-solution elapsed share is high;
- `phase2_tiebreak` loses when solution-root elapsed dominates;
- the same seed/suite profile behaves consistently across default and
  candidate output.

If the diagnostics are mixed, the report must say that no discriminator is
ready and recommend collecting more targeted data.

## Error Handling

The diagnostic script should fail if:

- either input directory is missing;
- analyzer output has no rows;
- default and candidate summaries have no common `case_name`;
- cache state is absent or not warm;
- comparison generation would overwrite a tracked source file.

## Testing

Because this pass uses existing analyzer tooling, the required checks are:

- run `ctest -R '^analyze_root_search_profile'`;
- run the analyzer on pass 41 default and candidate directories;
- verify the generated comparison has five common cases;
- run public documentation tests before committing the report;
- run full CTest before declaring the block complete.

## Documentation

Create `docs/v6-optimal-latency-pass42-2026-05-31.md` with:

- command lines used;
- cache state;
- diagnostics summary table;
- decision for the next pass.

The report must describe only measured library behavior.
