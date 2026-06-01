# Rubik Cube Library v6.0.0

V6 is a local optimal-latency release for Rubik Cube Library.

## Highlights

- Keeps optimal mode exact: `SolveStatus::Optimal` still means the returned HTM
  solution is proven minimal for the requested options.
- Promotes the measured V6 conservative root-ordering policy for local optimal
  solving.
- Adds benchmark tooling for V6 transition-corpus extraction and replay
  aggregation.
- Keeps fast mode available as an experimental non-optimal solving path.
- Strengthens release hygiene around public docs, examples, archive checks, and
  versioned release documents.

## Benchmark Notes

Published benchmark claims are limited to results measured in this repository on
the local benchmark machine. Hardware-specific Raspberry Pi, Jetson, Orin, GPU,
and cloud performance is not claimed in this release.

The V6 release benchmark evidence is documented in:

- `docs/v6-optimal-latency-pass75-78-2026-06-01.md`
- `docs/v6-optimal-latency-pass80-82-2026-06-01.md`
- `docs/v6-optimal-latency-pass83-85-2026-06-01.md`

## Compatibility

The V5 certified optimality and public-usability contracts are unchanged. The
stable public API is documented in `docs/api-stability-6.0.0.md`. Headers under
`rubik/detail/` and `rubik/experimental/` remain outside the compatibility
contract.

## Verification

Final local release verification must be recorded in `docs/release-6.0.0.md`
before publishing the tag.
