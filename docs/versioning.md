# Versioning And API Stability

The project uses semantic versioning for public releases:

- `MAJOR` changes for incompatible stable API changes.
- `MINOR` changes for new features and compatible API additions.
- `PATCH` changes for bug fixes, documentation fixes, and performance work that
  does not require user code changes.

The `1.x` line keeps the high-level solver API source-compatible. Incompatible
changes to stable APIs require a new major version.

## 1.x Stability Contract

Stable for `1.0.0`:

- `rubik::Cube`
- `rubik::CubieCube`
- `rubik::Move`
- `rubik::Solver`
- `rubik::SolveOptions`
- `rubik::SolveResult`
- `rubik::SolveBoundDiagnostics`
- `rubik::CubeError`
- `rubik/version.hpp` version constants
- CLI sticker input order: `U R F D L B`
- CMake imported target: `rubik::rubik`

The exact `1.0.0` freeze is documented in
[API Stability - 1.0.0](api-stability-1.0.0.md).

Experimental APIs:

- phase-1 and phase-2 APIs;
- pruning-table internals;
- coordinate APIs;
- benchmark random-case generation;
- environment-variable tuning flags;
- large-local optimal table combinations;
- `SolveMode::Fast` internals and solution quality.

## Compatibility Rules

`SolveStatus::Optimal` is a semantic guarantee: the returned HTM solution is
proven minimal within the requested options. Optimizations may change search
order, runtime, and diagnostics, but they must not weaken that guarantee.

The 54-sticker input order is frozen for `1.0.0`. Additional input formats
may be added later, but they should not change the meaning of the existing
format.

CMake package compatibility is generated from the project version with
`SameMajorVersion`.

The local release validation script also reads the archive version from the
CMake project version, so release metadata has a single version source.

## Release Notes

Every public release should update:

- `CHANGELOG.md`;
- `docs/release-<version>.md`;
- `docs/github-release-v<version>.md`;
- benchmark evidence for claims published in the README;
- any API stability changes in this file and `docs/api.md`.
