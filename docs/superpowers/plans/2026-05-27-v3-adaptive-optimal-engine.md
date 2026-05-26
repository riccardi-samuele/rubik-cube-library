# V3 Adaptive Optimal Engine Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the V3 adaptive optimal engine foundation: `SolveProfile::Auto`, cache policy controls, public solve-plan diagnostics, cache setup workflows, and desktop-first Auto benchmark gates.

**Architecture:** Add a small public planning surface in `include/rubik/solver.hpp`, then implement an internal planner in focused new files so `src/solver.cpp` does not absorb all policy logic. Keep the first Auto implementation conservative: it selects from already-supported table/profile policies, reports exactly what it selected, and only promotes more aggressive behavior after benchmark gates prove it.

**Tech Stack:** C++20, CMake, existing single-binary assert tests in `tests/rubik_tests.cpp`, existing CLI style in `apps/rubik_solve.cpp` and `apps/rubik_bench.cpp`, shell benchmark wrappers in `scripts/`.

---

## File Structure

- Modify `include/rubik/solver.hpp`: add `SolveProfile::Auto`, `CachePolicy`, `SolvePlan`, `SolveOptions::cachePolicy`, and `SolveResult::plan`.
- Create `include/rubik/cache.hpp`: public cache setup API declarations.
- Create `include/rubik/detail/optimal_plan.hpp`: internal normalized plan type and planner function declaration.
- Create `src/optimal_plan.cpp`: Auto planning policy, memory/thread/cache selection, and plan-to-public-summary mapping.
- Modify `src/solver.cpp`: call the planner once, reject unsupported Auto combinations, use the effective profile/thread/memory, and attach `SolvePlan` to every result.
- Create `src/cache.cpp`: minimal `prepareCache()` implementation backed by the planner and existing table warmup calls.
- Create `apps/rubik_cache_setup.cpp`: dedicated cache setup CLI.
- Modify `apps/rubik_solve.cpp`: parse `--profile auto`, `--cache-policy`, `--threads 0`, and print plan diagnostics.
- Modify `apps/rubik_bench.cpp`: support `profile=auto`, cache policy metadata, and Auto benchmark rows.
- Modify `CMakeLists.txt`: add new source files, CLI executable, install target, and Auto benchmark/gate targets.
- Modify `tests/rubik_tests.cpp`: add focused API/planner/cache tests.
- Modify `tests/consumer_smoke/main.cpp`: compile against the new public API.
- Modify `docs/api.md`, `docs/runtime.md`, `docs/benchmarks.md`, `docs/roadmap.md`, and `README.md`: document Auto as the recommended optimal profile.
- Modify `docs/api-stability-2.0.0.md` only by adding a note that V3 has its own future contract; do not rewrite the V2 contract.

## Task 1: Public API Skeleton

**Files:**
- Modify: `include/rubik/solver.hpp`
- Modify: `tests/rubik_tests.cpp`

- [ ] **Step 1: Add failing tests for API defaults**

Add this function near the existing solver tests in `tests/rubik_tests.cpp`:

```cpp
void testV3AdaptiveApiDefaults()
{
    rubik::SolveOptions options;
    assert(options.profile == rubik::SolveProfile::Default);
    assert(options.cachePolicy == rubik::CachePolicy::Auto);

    rubik::SolvePlan plan;
    assert(plan.requestedProfile == rubik::SolveProfile::Default);
    assert(plan.effectiveProfile == rubik::SolveProfile::Default);
    assert(plan.cachePolicy == rubik::CachePolicy::Auto);
    assert(plan.requestedThreads == 1);
    assert(plan.effectiveThreads == 1);
    assert(plan.requestedMaxMemoryBytes == 0);
    assert(plan.effectiveMaxMemoryBytes == 0);
    assert(plan.estimatedTablePayloadBytes == 0);
    assert(!plan.diskCacheEnabled);
    assert(!plan.diskCacheWarm);
    assert(!plan.builtCacheDuringSolve);
    assert(plan.strategyName.empty());
    assert(plan.boundsUsed.empty());

    rubik::SolveResult result;
    assert(result.plan.cachePolicy == rubik::CachePolicy::Auto);
}
```

Call it from `main()`:

```cpp
testV3AdaptiveApiDefaults();
```

- [ ] **Step 2: Run the test and verify it fails**

Run:

```sh
cmake --build --preset release --target rubik_tests
out/release/rubik_tests
```

Expected: compile fails because `rubik::CachePolicy` and `rubik::SolvePlan` do not exist.

- [ ] **Step 3: Add the public types**

Edit `include/rubik/solver.hpp`:

```cpp
#include <string>
```

Extend `SolveProfile`:

```cpp
enum class SolveProfile {
    Embedded,
    Default,
    Performance,
    LargeLocal,
    Auto
};
```

Add after `SolveStatus`:

```cpp
enum class CachePolicy {
    Auto,
    RequireWarm,
    AllowBuild,
    Disabled
};

struct SolvePlan {
    SolveProfile requestedProfile = SolveProfile::Default;
    SolveProfile effectiveProfile = SolveProfile::Default;
    SolveMode mode = SolveMode::Optimal;
    Metric metric = Metric::HTM;
    std::size_t requestedMaxMemoryBytes = 0;
    std::size_t effectiveMaxMemoryBytes = 0;
    std::size_t estimatedTablePayloadBytes = 0;
    unsigned int requestedThreads = 1;
    unsigned int effectiveThreads = 1;
    CachePolicy cachePolicy = CachePolicy::Auto;
    bool diskCacheEnabled = false;
    bool diskCacheWarm = false;
    bool builtCacheDuringSolve = false;
    std::vector<std::string> boundsUsed;
    std::string strategyName;
};
```

Add to `SolveOptions`:

```cpp
    CachePolicy cachePolicy = CachePolicy::Auto;
```

Add to `SolveResult`:

```cpp
    SolvePlan plan;
```

- [ ] **Step 4: Run the API test and verify it passes**

Run:

```sh
cmake --build --preset release --target rubik_tests
out/release/rubik_tests
```

Expected: `rubik_tests` exits with code 0.

- [ ] **Step 5: Commit**

```sh
git add include/rubik/solver.hpp tests/rubik_tests.cpp
git commit -m "Add V3 adaptive API skeleton"
```

## Task 2: Internal Optimal Planner

**Files:**
- Create: `include/rubik/detail/optimal_plan.hpp`
- Create: `src/optimal_plan.cpp`
- Modify: `CMakeLists.txt`
- Modify: `tests/rubik_tests.cpp`

- [ ] **Step 1: Add failing planner tests**

Add include:

```cpp
#include "rubik/detail/optimal_plan.hpp"
```

Add tests:

```cpp
void testAutoPlannerRejectsFastMode()
{
    rubik::SolveOptions options;
    options.mode = rubik::SolveMode::Fast;
    options.profile = rubik::SolveProfile::Auto;

    const auto plan = rubik::detail::makeOptimalPlan(options);
    assert(!plan.supported);
    assert(plan.status == rubik::SolveStatus::UnsupportedOptions);
}

void testAutoPlannerDesktopDefaults()
{
    rubik::SolveOptions options;
    options.mode = rubik::SolveMode::Optimal;
    options.metric = rubik::Metric::HTM;
    options.profile = rubik::SolveProfile::Auto;
    options.maxMemoryBytes = 0;
    options.threads = 0;

    const auto plan = rubik::detail::makeOptimalPlan(options);
    assert(plan.supported);
    assert(plan.publicPlan.requestedProfile == rubik::SolveProfile::Auto);
    assert(plan.publicPlan.effectiveProfile == rubik::SolveProfile::LargeLocal);
    assert(plan.publicPlan.effectiveMaxMemoryBytes == 2ull * 1024ull * 1024ull * 1024ull);
    assert(plan.publicPlan.effectiveThreads >= 1);
    assert(plan.publicPlan.effectiveThreads <= 8);
    assert(plan.publicPlan.strategyName == "auto_desktop_tail");
    assert(!plan.publicPlan.boundsUsed.empty());
}
```

Call both from `main()`.

- [ ] **Step 2: Run and verify failure**

Run:

```sh
cmake --build --preset release --target rubik_tests
```

Expected: compile fails because `rubik/detail/optimal_plan.hpp` does not exist.

- [ ] **Step 3: Create planner header**

Create `include/rubik/detail/optimal_plan.hpp`:

```cpp
#pragma once

#include "rubik/solver.hpp"

#include <cstddef>

namespace rubik::detail {

struct OptimalPlan {
    bool supported = true;
    SolveStatus status = SolveStatus::InternalError;
    SolveOptions effectiveOptions;
    SolvePlan publicPlan;
};

std::size_t autoMemoryBudgetBytes(const SolveOptions& options);
unsigned int autoThreadCount(const SolveOptions& options);
OptimalPlan makeOptimalPlan(const SolveOptions& options);

} // namespace rubik::detail
```

- [ ] **Step 4: Create planner implementation**

Create `src/optimal_plan.cpp`:

```cpp
#include "rubik/detail/optimal_plan.hpp"

#include "rubik/detail/table_profiles.hpp"

#include <algorithm>
#include <thread>

namespace rubik::detail {
namespace {

constexpr std::size_t auto_default_memory_bytes = 2ull * 1024ull * 1024ull * 1024ull;
constexpr unsigned int auto_default_thread_cap = 8;

std::vector<std::string> boundsForProfile(SolveProfile profile)
{
    if (profile == SolveProfile::LargeLocal) {
        return {
            "corner_state",
            "corner_orientation_up_edge_permutation",
            "corner_orientation_down_edge_permutation",
        };
    }
    return {"corner_state"};
}

std::size_t estimatedPayloadForProfile(SolveProfile profile)
{
    return optimalTablePayloadBytes(profile);
}

} // namespace

std::size_t autoMemoryBudgetBytes(const SolveOptions& options)
{
    if (options.maxMemoryBytes != 0) {
        return options.maxMemoryBytes;
    }
    return auto_default_memory_bytes;
}

unsigned int autoThreadCount(const SolveOptions& options)
{
    if (options.threads != 0) {
        return options.threads;
    }

    const unsigned int hardware = std::thread::hardware_concurrency();
    if (hardware == 0) {
        return 1;
    }
    return std::max(1u, std::min(hardware, auto_default_thread_cap));
}

OptimalPlan makeOptimalPlan(const SolveOptions& options)
{
    OptimalPlan plan;
    plan.effectiveOptions = options;
    plan.status = SolveStatus::InternalError;

    plan.publicPlan.requestedProfile = options.profile;
    plan.publicPlan.mode = options.mode;
    plan.publicPlan.metric = options.metric;
    plan.publicPlan.requestedMaxMemoryBytes = options.maxMemoryBytes;
    plan.publicPlan.requestedThreads = options.threads;
    plan.publicPlan.cachePolicy = options.cachePolicy;

    if (options.profile == SolveProfile::Auto &&
        (options.mode != SolveMode::Optimal || options.metric != Metric::HTM)) {
        plan.supported = false;
        plan.status = SolveStatus::UnsupportedOptions;
        return plan;
    }

    const SolveProfile effectiveProfile =
        options.profile == SolveProfile::Auto ? SolveProfile::LargeLocal : options.profile;
    const std::size_t effectiveMemory =
        options.profile == SolveProfile::Auto ? autoMemoryBudgetBytes(options) : options.maxMemoryBytes;
    const unsigned int effectiveThreads =
        options.profile == SolveProfile::Auto ? autoThreadCount(options) : options.threads;

    plan.effectiveOptions.profile = effectiveProfile;
    plan.effectiveOptions.maxMemoryBytes = effectiveMemory;
    plan.effectiveOptions.threads = effectiveThreads;

    plan.publicPlan.effectiveProfile = effectiveProfile;
    plan.publicPlan.effectiveMaxMemoryBytes = effectiveMemory;
    plan.publicPlan.effectiveThreads = effectiveThreads;
    plan.publicPlan.estimatedTablePayloadBytes = estimatedPayloadForProfile(effectiveProfile);
    plan.publicPlan.boundsUsed = boundsForProfile(effectiveProfile);
    plan.publicPlan.strategyName =
        options.profile == SolveProfile::Auto ? "auto_desktop_tail" : "manual_profile";
    plan.publicPlan.diskCacheEnabled = options.cachePolicy != CachePolicy::Disabled;

    plan.supported = true;
    plan.status = SolveStatus::Found;
    return plan;
}

} // namespace rubik::detail
```

- [ ] **Step 5: Register source in CMake**

Add `src/optimal_plan.cpp` to the `add_library(rubik ...)` source list in `CMakeLists.txt`.

- [ ] **Step 6: Run tests**

Run:

```sh
cmake --preset release
cmake --build --preset release --target rubik_tests
out/release/rubik_tests
```

Expected: pass.

- [ ] **Step 7: Commit**

```sh
git add include/rubik/detail/optimal_plan.hpp src/optimal_plan.cpp CMakeLists.txt tests/rubik_tests.cpp
git commit -m "Add adaptive optimal planner"
```

## Task 3: Wire Planner Into Solver

**Files:**
- Modify: `src/solver.cpp`
- Modify: `tests/rubik_tests.cpp`

- [ ] **Step 1: Add failing solver behavior tests**

Add:

```cpp
void testAutoSolveReportsPlan()
{
    rubik::Solver solver;
    rubik::SolveOptions options;
    options.mode = rubik::SolveMode::Optimal;
    options.metric = rubik::Metric::HTM;
    options.profile = rubik::SolveProfile::Auto;
    options.maxDepth = 0;
    options.timeout = std::chrono::milliseconds{1000};
    options.threads = 0;

    const rubik::SolveResult result = solver.solve(rubik::Cube::solved(), options);
    assert(result.status == rubik::SolveStatus::Solved);
    assert(result.plan.requestedProfile == rubik::SolveProfile::Auto);
    assert(result.plan.effectiveProfile == rubik::SolveProfile::LargeLocal);
    assert(result.plan.effectiveThreads >= 1);
    assert(result.plan.effectiveMaxMemoryBytes == 2ull * 1024ull * 1024ull * 1024ull);
    assert(result.plan.strategyName == "auto_desktop_tail");
}

void testAutoFastSolveIsUnsupported()
{
    rubik::Solver solver;
    rubik::SolveOptions options;
    options.mode = rubik::SolveMode::Fast;
    options.profile = rubik::SolveProfile::Auto;

    const rubik::SolveResult result = solver.solve(rubik::Cube::solved(), options);
    assert(result.status == rubik::SolveStatus::UnsupportedOptions);
    assert(result.plan.requestedProfile == rubik::SolveProfile::Auto);
}
```

Call both from `main()`.

- [ ] **Step 2: Run and verify failure**

Run:

```sh
cmake --build --preset release --target rubik_tests
out/release/rubik_tests
```

Expected: tests fail because solver does not attach the plan and does not reject Auto/Fast.

- [ ] **Step 3: Use planner at solver entry**

In `src/solver.cpp`, include:

```cpp
#include "rubik/detail/optimal_plan.hpp"
```

At the start of `Solver::solve`, before option validation uses the original options, add:

```cpp
    const detail::OptimalPlan plan = detail::makeOptimalPlan(options);
    if (!plan.supported) {
        SolveResult result;
        result.status = plan.status;
        result.metric = options.metric;
        result.plan = plan.publicPlan;
        return result;
    }

    const SolveOptions effectiveOptions = plan.effectiveOptions;
```

Then replace subsequent reads of `options.profile`, `options.threads`,
`options.maxMemoryBytes`, `options.timeout`, `options.maxDepth`, and
`options.metric` inside `Solver::solve` with `effectiveOptions.<field>`.

Before each return from `Solver::solve`, set:

```cpp
    result.plan = plan.publicPlan;
```

If the function currently has many direct returns, introduce a small helper:

```cpp
    const auto withPlan = [&](SolveResult result) {
        result.plan = plan.publicPlan;
        return result;
    };
```

and return `withPlan(result)`.

- [ ] **Step 4: Run tests**

Run:

```sh
cmake --build --preset release --target rubik_tests
out/release/rubik_tests
```

Expected: pass.

- [ ] **Step 5: Commit**

```sh
git add src/solver.cpp tests/rubik_tests.cpp
git commit -m "Use adaptive planner in solver"
```

## Task 4: Cache Setup API

**Files:**
- Create: `include/rubik/cache.hpp`
- Create: `src/cache.cpp`
- Modify: `CMakeLists.txt`
- Modify: `tests/rubik_tests.cpp`

- [ ] **Step 1: Add failing cache API test**

Add include:

```cpp
#include "rubik/cache.hpp"
```

Add:

```cpp
void testPrepareCacheDryRun()
{
    rubik::CacheSetupOptions options;
    options.profile = rubik::SolveProfile::Auto;
    options.cachePolicy = rubik::CachePolicy::AllowBuild;
    options.maxMemoryBytes = 0;
    options.threads = 0;
    options.dryRun = true;

    const rubik::CacheSetupResult result = rubik::prepareCache(options);
    assert(result.ready);
    assert(result.plan.requestedProfile == rubik::SolveProfile::Auto);
    assert(result.plan.effectiveProfile == rubik::SolveProfile::LargeLocal);
    assert(result.bytesPrepared == 0);
    assert(!result.message.empty());
}
```

Call it from `main()`.

- [ ] **Step 2: Run and verify failure**

Run:

```sh
cmake --build --preset release --target rubik_tests
```

Expected: compile fails because `rubik/cache.hpp` does not exist.

- [ ] **Step 3: Add cache API header**

Create `include/rubik/cache.hpp`:

```cpp
#pragma once

#include "rubik/solver.hpp"

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <string>

namespace rubik {

struct CacheSetupOptions {
    SolveProfile profile = SolveProfile::Auto;
    CachePolicy cachePolicy = CachePolicy::AllowBuild;
    std::size_t maxMemoryBytes = 0;
    unsigned int threads = 0;
    std::filesystem::path cacheDirectory;
    bool dryRun = false;
};

struct CacheSetupResult {
    bool ready = false;
    SolvePlan plan;
    std::size_t bytesPrepared = 0;
    std::chrono::milliseconds elapsed{0};
    std::string message;
};

CacheSetupResult prepareCache(const CacheSetupOptions& options);

} // namespace rubik
```

- [ ] **Step 4: Add minimal implementation**

Create `src/cache.cpp`:

```cpp
#include "rubik/cache.hpp"

#include "rubik/detail/optimal_plan.hpp"
#include "rubik/pruning_tables.hpp"

#include <chrono>

namespace rubik {

CacheSetupResult prepareCache(const CacheSetupOptions& options)
{
    const auto started = std::chrono::steady_clock::now();

    SolveOptions solveOptions;
    solveOptions.mode = SolveMode::Optimal;
    solveOptions.metric = Metric::HTM;
    solveOptions.profile = options.profile;
    solveOptions.cachePolicy = options.cachePolicy;
    solveOptions.maxMemoryBytes = options.maxMemoryBytes;
    solveOptions.threads = options.threads;

    const detail::OptimalPlan plan = detail::makeOptimalPlan(solveOptions);

    CacheSetupResult result;
    result.plan = plan.publicPlan;
    if (!plan.supported) {
        result.ready = false;
        result.message = "unsupported cache setup options";
    } else if (options.dryRun) {
        result.ready = true;
        result.bytesPrepared = 0;
        result.message = "dry run: cache plan selected";
    } else {
        (void)pruning_tables::cornerOrientation();
        (void)pruning_tables::edgeOrientation();
        (void)pruning_tables::sliceEdges();
        result.ready = true;
        result.bytesPrepared = result.plan.estimatedTablePayloadBytes;
        result.message = "cache setup completed";
    }

    result.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started);
    return result;
}

} // namespace rubik
```

- [ ] **Step 5: Register source and installed header**

Add `src/cache.cpp` to `add_library(rubik ...)` in `CMakeLists.txt`. Ensure install rules include `include/rubik/cache.hpp`; if the project installs the whole include directory, no extra install line is needed.

- [ ] **Step 6: Run tests**

Run:

```sh
cmake --preset release
cmake --build --preset release --target rubik_tests
out/release/rubik_tests
```

Expected: pass.

- [ ] **Step 7: Commit**

```sh
git add include/rubik/cache.hpp src/cache.cpp CMakeLists.txt tests/rubik_tests.cpp
git commit -m "Add cache setup API"
```

## Task 5: `rubik-cache-setup` CLI

**Files:**
- Create: `apps/rubik_cache_setup.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add CTest entries before implementation**

Add to `CMakeLists.txt` after the `rubik-bench` target block:

```cmake
add_executable(rubik-cache-setup
    apps/rubik_cache_setup.cpp
)
target_link_libraries(rubik-cache-setup PRIVATE rubik)
add_test(
    NAME cli_cache_setup_dry_run
    COMMAND rubik-cache-setup --profile auto --dry-run
)
set_tests_properties(cli_cache_setup_dry_run PROPERTIES
    PASS_REGULAR_EXPRESSION "status: Ready"
)
add_test(
    NAME cli_cache_setup_rejects_invalid_profile
    COMMAND rubik-cache-setup --profile invalid --dry-run
)
set_tests_properties(cli_cache_setup_rejects_invalid_profile PROPERTIES
    WILL_FAIL TRUE
)
```

- [ ] **Step 2: Run and verify failure**

Run:

```sh
cmake --preset release
cmake --build --preset release --target rubik-cache-setup
```

Expected: build fails because `apps/rubik_cache_setup.cpp` does not exist.

- [ ] **Step 3: Add CLI implementation**

Create `apps/rubik_cache_setup.cpp`:

```cpp
#include "rubik/cache.hpp"
#include "rubik/version.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

bool parseProfile(const std::string& value, rubik::SolveProfile& profile)
{
    if (value == "auto") { profile = rubik::SolveProfile::Auto; return true; }
    if (value == "embedded") { profile = rubik::SolveProfile::Embedded; return true; }
    if (value == "default") { profile = rubik::SolveProfile::Default; return true; }
    if (value == "performance") { profile = rubik::SolveProfile::Performance; return true; }
    if (value == "large-local") { profile = rubik::SolveProfile::LargeLocal; return true; }
    return false;
}

void printUsage()
{
    std::cout
        << "rubik-cache-setup " << rubik::version_string << "\n"
        << "Usage: rubik-cache-setup [--profile auto|embedded|default|performance|large-local]\n"
        << "                         [--max-memory-mb N] [--threads N]\n"
        << "                         [--cache-dir PATH] [--dry-run]\n";
}

} // namespace

int main(int argc, char** argv)
{
    rubik::CacheSetupOptions options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help") {
            printUsage();
            return 0;
        }
        if (arg == "--dry-run") {
            options.dryRun = true;
            continue;
        }
        if (arg == "--profile" && i + 1 < argc) {
            if (!parseProfile(argv[++i], options.profile)) {
                std::cerr << "invalid profile\n";
                return 2;
            }
            continue;
        }
        if (arg == "--max-memory-mb" && i + 1 < argc) {
            options.maxMemoryBytes = static_cast<std::size_t>(std::strtoull(argv[++i], nullptr, 10)) *
                1024ull * 1024ull;
            continue;
        }
        if (arg == "--threads" && i + 1 < argc) {
            options.threads = static_cast<unsigned int>(std::strtoul(argv[++i], nullptr, 10));
            continue;
        }
        if (arg == "--cache-dir" && i + 1 < argc) {
            options.cacheDirectory = argv[++i];
            continue;
        }
        std::cerr << "unknown or incomplete argument: " << arg << "\n";
        return 2;
    }

    const rubik::CacheSetupResult result = rubik::prepareCache(options);
    std::cout << "status: " << (result.ready ? "Ready" : "Failed") << "\n";
    std::cout << "profile: " << static_cast<int>(result.plan.effectiveProfile) << "\n";
    std::cout << "threads: " << result.plan.effectiveThreads << "\n";
    std::cout << "memory-bytes: " << result.plan.effectiveMaxMemoryBytes << "\n";
    std::cout << "payload-bytes: " << result.plan.estimatedTablePayloadBytes << "\n";
    std::cout << "message: " << result.message << "\n";
    return result.ready ? 0 : 1;
}
```

- [ ] **Step 4: Run CLI tests**

Run:

```sh
cmake --preset release
cmake --build --preset release --target rubik-cache-setup
ctest --preset release --output-on-failure -R "cli_cache_setup"
```

Expected: `cli_cache_setup_dry_run` and `cli_cache_setup_rejects_invalid_profile` pass.

- [ ] **Step 5: Commit**

```sh
git add apps/rubik_cache_setup.cpp CMakeLists.txt
git commit -m "Add cache setup CLI"
```

## Task 6: CLI Auto Profile and Cache Policy

**Files:**
- Modify: `apps/rubik_solve.cpp`
- Modify: `apps/rubik_bench.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add CLI tests**

Add CTest entries:

```cmake
add_test(
    NAME cli_solve_accepts_auto_profile
    COMMAND rubik-solve UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB --profile auto --threads 0 --timeout-ms 1000
)
set_tests_properties(cli_solve_accepts_auto_profile PROPERTIES
    PASS_REGULAR_EXPRESSION "status: Solved"
)
add_test(
    NAME cli_solve_rejects_auto_fast
    COMMAND rubik-solve UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB --mode fast --profile auto --timeout-ms 1000
)
set_tests_properties(cli_solve_rejects_auto_fast PROPERTIES
    WILL_FAIL TRUE
)
add_test(
    NAME cli_solve_accepts_cache_policy
    COMMAND rubik-solve UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB --profile auto --cache-policy disabled --timeout-ms 1000
)
set_tests_properties(cli_solve_accepts_cache_policy PROPERTIES
    PASS_REGULAR_EXPRESSION "status: Solved"
)
```

- [ ] **Step 2: Run and verify failure**

Run:

```sh
cmake --preset release
cmake --build --preset release --target rubik-solve
ctest --preset release --output-on-failure -R "cli_solve_accepts_auto_profile|cli_solve_rejects_auto_fast|cli_solve_accepts_cache_policy"
```

Expected: at least profile/cache-policy tests fail because parsing is missing.

- [ ] **Step 3: Update parsers**

In both `apps/rubik_solve.cpp` and `apps/rubik_bench.cpp`, extend profile parsing:

```cpp
if (value == "auto") {
    return rubik::SolveProfile::Auto;
}
```

Add cache-policy parsing:

```cpp
rubik::CachePolicy parseCachePolicy(const std::string& value)
{
    if (value == "auto") return rubik::CachePolicy::Auto;
    if (value == "require-warm") return rubik::CachePolicy::RequireWarm;
    if (value == "allow-build") return rubik::CachePolicy::AllowBuild;
    if (value == "disabled") return rubik::CachePolicy::Disabled;
    throw std::invalid_argument("invalid cache policy");
}
```

When parsing `--cache-policy VALUE`, set:

```cpp
options.cachePolicy = parseCachePolicy(argv[++i]);
```

Ensure `--threads 0` is accepted instead of rejected.

- [ ] **Step 4: Print plan diagnostics in `rubik-solve`**

After solve output, add:

```cpp
std::cout << "effective-profile: " << static_cast<int>(result.plan.effectiveProfile) << "\n";
std::cout << "effective-threads: " << result.plan.effectiveThreads << "\n";
std::cout << "effective-memory-bytes: " << result.plan.effectiveMaxMemoryBytes << "\n";
std::cout << "strategy: " << result.plan.strategyName << "\n";
```

- [ ] **Step 5: Run CLI tests**

Run:

```sh
cmake --build --preset release --target rubik-solve rubik-bench
ctest --preset release --output-on-failure -R "cli_solve_accepts_auto_profile|cli_solve_rejects_auto_fast|cli_solve_accepts_cache_policy"
```

Expected: pass.

- [ ] **Step 6: Commit**

```sh
git add apps/rubik_solve.cpp apps/rubik_bench.cpp CMakeLists.txt
git commit -m "Add Auto profile CLI controls"
```

## Task 7: Auto Benchmark Gates

**Files:**
- Modify: `scripts/run_benchmark_suite.sh`
- Modify: `CMakeLists.txt`
- Modify: `docs/benchmarks.md`

- [ ] **Step 1: Add benchmark target and gate first**

Add to `CMakeLists.txt`:

```cmake
add_custom_target(rubik-benchmark-auto-profile
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_benchmark_suite.sh
        --suite profile-realistic
        --cache-mode warm
        --build-dir ${CMAKE_CURRENT_BINARY_DIR}
        --cache-dir /tmp/rubik_cube_library_auto_profile_cache
        --output-dir ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results/auto-profile
        --realistic-fast-count 0
        --realistic-opt12-count 10
        --realistic-opt13-count 10
        --profile auto
        --threads 0
        --max-memory-mb 2048
    DEPENDS rubik-bench
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running Auto optimal profile benchmark suite"
)
add_custom_target(rubik-benchmark-auto-profile-gates
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_benchmark_gates.sh
        --summary-file ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results/auto-profile/warm_profile_realistic_summary.csv
        --gate auto,optimal,random_depth_13_count_10,10,2500,2500,2500
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Checking Auto optimal profile benchmark gates"
)
```

- [ ] **Step 2: Run and verify failure**

Run:

```sh
cmake --preset release-native-lto
cmake --build --preset release-native-lto --target rubik-benchmark-auto-profile
```

Expected: fails if `run_benchmark_suite.sh` does not accept `--profile auto` or zero fast-count.

- [ ] **Step 3: Update benchmark script**

In `scripts/run_benchmark_suite.sh`, extend profile validation to include `auto`. Ensure profile-realistic emits profile label `auto` when requested and skips fast runs when `--realistic-fast-count 0`.

- [ ] **Step 4: Run Auto benchmark and gate**

Run:

```sh
cmake --build --preset release-native-lto --target rubik-benchmark-auto-profile
cmake --build --preset release-native-lto --target rubik-benchmark-auto-profile-gates
```

Expected: both targets pass.

- [ ] **Step 5: Document benchmark command**

Add to `docs/benchmarks.md` quick suite:

```sh
cmake --build build --target rubik-benchmark-auto-profile
cmake --build build --target rubik-benchmark-auto-profile-gates
```

- [ ] **Step 6: Commit**

```sh
git add CMakeLists.txt scripts/run_benchmark_suite.sh docs/benchmarks.md
git commit -m "Add Auto profile benchmark gates"
```

## Task 8: Consumer Smoke and Install Surface

**Files:**
- Modify: `tests/consumer_smoke/main.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Update consumer smoke**

Add to `tests/consumer_smoke/main.cpp`:

```cpp
#include <rubik/cache.hpp>
```

In `main()`, add a compile/runtime smoke:

```cpp
rubik::CacheSetupOptions cacheOptions;
cacheOptions.profile = rubik::SolveProfile::Auto;
cacheOptions.dryRun = true;
const rubik::CacheSetupResult cacheResult = rubik::prepareCache(cacheOptions);
if (!cacheResult.ready) {
    return 2;
}
```

- [ ] **Step 2: Run install consumer check**

Run:

```sh
cmake --preset release
cmake --build out/release --target rubik-check-install-consumer
```

Expected: consumer build and smoke executable pass.

- [ ] **Step 3: Commit**

```sh
git add tests/consumer_smoke/main.cpp
git commit -m "Exercise V3 cache API in consumer smoke"
```

## Task 9: Documentation Pass

**Files:**
- Modify: `README.md`
- Modify: `docs/api.md`
- Modify: `docs/runtime.md`
- Modify: `docs/roadmap.md`
- Create: `docs/api-stability-3.0.0.md`

- [ ] **Step 1: Add V3 stability draft**

Create `docs/api-stability-3.0.0.md` with:

```markdown
# API Stability - 3.0.0 Draft

V3 keeps the V2 certified HTM optimality contract.

New stable public concepts:

- `SolveProfile::Auto`
- `CachePolicy`
- `SolvePlan`
- `CacheSetupOptions`
- `CacheSetupResult`
- `prepareCache()`
- CLI `rubik-cache-setup`

`SolveProfile::Auto` is valid for `SolveMode::Optimal` with `Metric::HTM`.
Unsupported combinations return `SolveStatus::UnsupportedOptions`.

The exact internal table set selected by `Auto` is not stable. The public
contract is that the selected plan respects user limits and is reported through
`SolvePlan`.
```

- [ ] **Step 2: Update README quick start**

Replace the recommended optimal options in `README.md` with:

```cpp
rubik::SolveResult result = solver.solve(cube, {
    .mode = rubik::SolveMode::Optimal,
    .metric = rubik::Metric::HTM,
    .profile = rubik::SolveProfile::Auto,
    .cachePolicy = rubik::CachePolicy::Auto,
    .threads = 0,
    .maxMemoryBytes = 0,
    .timeout = std::chrono::seconds(30),
});
```

Add:

```markdown
`SolveProfile::Auto` is the recommended profile for adaptive certified HTM
optimal solving. Explicit profiles remain available when an application needs a
fixed memory/performance policy.
```

- [ ] **Step 3: Update runtime/cache docs**

Add to `docs/runtime.md`:

```markdown
## Adaptive Cache Policy

`CachePolicy::Auto` uses compatible warm cache data when available. It may avoid
building heavy cache data during a short solve. Use `rubik-cache-setup` or
`prepareCache()` to prepare cache before latency-sensitive solving.
```

- [ ] **Step 4: Run doc gate**

Run:

```sh
tests/public_docs_no_unverified_hardware_estimates.sh
git diff --check
```

Expected: both pass.

- [ ] **Step 5: Commit**

```sh
git add README.md docs/api.md docs/runtime.md docs/roadmap.md docs/api-stability-3.0.0.md
git commit -m "Document V3 adaptive optimal workflow"
```

## Task 10: Full Verification

**Files:**
- No manual edits unless a verification failure requires a fix.

- [ ] **Step 1: Run standard release check**

Run:

```sh
scripts/release_check.sh --profile standard
```

Expected: `release_check,status,passed`.

- [ ] **Step 2: Run full benchmark check**

Run:

```sh
scripts/release_check.sh --profile full --with-benchmarks
```

Expected: `release_check,status,passed`.

- [ ] **Step 3: Run Auto benchmark gates**

Run:

```sh
cmake --preset release-native-lto
cmake --build --preset release-native-lto --target rubik-benchmark-auto-profile
cmake --build --preset release-native-lto --target rubik-benchmark-auto-profile-gates
```

Expected: all targets pass.

- [ ] **Step 4: Commit validation notes**

If new benchmark documentation is generated, add a short V3 validation report under `docs/` and commit it:

```sh
git add docs/benchmarks.md docs/v3-*.md
git commit -m "Record V3 adaptive optimal validation"
```

If no files changed, do not create an empty commit.

## Self-Review Notes

- Spec coverage: this plan covers `SolveProfile::Auto`, `CachePolicy`, `SolvePlan`, cache setup CLI/API, planner architecture, docs, benchmarks, and no embedded performance claims.
- Deliberate deferral: GPU, QTM, cloud, camera, robot control, and persistent learning stay out of V3.
- First implementation is intentionally planner-first. Tail optimization beyond existing LargeLocal bounds should start only after Auto has benchmark gates and diagnostics.
