# Runtime Behavior

This note documents runtime guarantees that matter when embedding the library in
applications, services, or other local software.

## Thread Safety

`rubik::Solver` is currently stateless. It is safe to create separate
`rubik::Solver` instances on different threads and call `solve` concurrently.

It is also safe to call `solve` concurrently on the same `rubik::Solver`
instance as long as the input `rubik::Cube` objects are not being mutated by
other threads at the same time.

Pruning tables and move tables are initialized lazily through function-local
static objects. C++ guarantees that this initialization is thread-safe inside one
process. The first thread that touches a missing table may pay the construction
or load cost; later calls reuse the same immutable table.

`SolveOptions::threads` controls internal root-level parallelism for
`SolveMode::Optimal`. This is independent from application-level concurrency:
running many solver calls concurrently while each call also uses multiple worker
threads can oversubscribe the CPU. For services, prefer either:

- one solve per process/thread with `threads > 1`; or
- many concurrent solves with `threads = 1`.

Benchmark the chosen policy on the target hardware.

## Cube Mutation

`rubik::Cube::apply` mutates the cube. Do not mutate the same `rubik::Cube`
object from multiple threads without external synchronization.

`Solver::solve` takes `const Cube&` and does not mutate the caller's cube.

## Table Cache

Pruning tables are cached as `.rpt` files. The cache directory is:

- `RUBIK_TABLE_CACHE_DIR`, when set to a non-empty value;
- otherwise the system temporary directory plus `rubik_cube_library`.

Each cache file contains:

- a magic value identifying Rubik pruning-table files;
- a cache format version;
- the expected table byte size;
- the raw pruning-table payload.

When loading a table, the library validates the magic value, cache format
version, and expected size. If any check fails, the cache file is ignored and the
table is rebuilt.

## Cache Compatibility

The current cache format version is internal. A future change to table encoding,
coordinate indexing, or binary layout must bump the cache version or change the
table file name. This prevents old files from silently being interpreted with
new semantics.

Adding a new table is compatible: it creates a new cache file and does not
invalidate existing files.

Changing the size of an existing table is compatible from a safety perspective:
the size check rejects the old file. If the table size stays the same but its
meaning changes, the cache version or file name must change.

## Shared Cache Directories

Multiple threads inside one process may share one cache directory.

Multiple processes may also reuse a warm cache directory after tables already
exist. Avoid launching several cold-start processes at the same time against the
same empty cache directory: the current writer path does not use an
interprocess lock, so competing builders may duplicate work and race on the
temporary file.

For production deployments, use one of these policies:

- prewarm the cache during installation or startup before concurrent workers
  are launched;
- give each process its own `RUBIK_TABLE_CACHE_DIR`;
- use an external process lock around the first cache warm-up.

## Cold Cache And Warm Cache

Cold-cache latency includes table construction and disk writes. Warm-cache
latency measures solving after table files already exist and have been loaded or
memory-mapped by the operating system cache.

Benchmark reports must state whether they are cold-cache or warm-cache runs.

## Adaptive Cache Policy

`CachePolicy::Auto` uses compatible warm cache data when available. It may avoid
building heavy cache data during a short solve. Use `rubik-cache-setup` or
`prepareCache()` to prepare cache before latency-sensitive solving.

`rubik-cache-setup --dry-run` checks the selected plan without building tables.
Its output includes `cache-warm` and `bytes-missing` so applications can detect
whether the next optimal solve may pay cold-cache setup cost.

`CachePolicy::RequireWarm` is strict: if the selected solve plan needs cache
files that are not present, `Solver::solve()` returns
`SolveStatus::CacheNotReady` before entering search.
