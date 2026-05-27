#include "rubik/cache.hpp"

#include "rubik/coordinates.hpp"
#include "rubik/detail/optimal_plan.hpp"
#include "rubik/detail/table_profiles.hpp"
#include "rubik/pruning_tables.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>

namespace rubik {
namespace {

class ScopedCacheDirectory {
public:
    explicit ScopedCacheDirectory(const std::filesystem::path& directory)
    {
        if (directory.empty()) {
            return;
        }
        active_ = true;
        if (const char* existing = std::getenv("RUBIK_TABLE_CACHE_DIR")) {
            previous_ = existing;
        }
        set(directory.string());
    }

    ~ScopedCacheDirectory()
    {
        if (!active_) {
            return;
        }
        if (previous_) {
            set(*previous_);
        } else {
            unset();
        }
    }

    ScopedCacheDirectory(const ScopedCacheDirectory&) = delete;
    ScopedCacheDirectory& operator=(const ScopedCacheDirectory&) = delete;

private:
    static void set(const std::string& value)
    {
#ifdef _WIN32
        _putenv_s("RUBIK_TABLE_CACHE_DIR", value.c_str());
#else
        setenv("RUBIK_TABLE_CACHE_DIR", value.c_str(), 1);
#endif
    }

    static void unset()
    {
#ifdef _WIN32
        _putenv_s("RUBIK_TABLE_CACHE_DIR", "");
#else
        unsetenv("RUBIK_TABLE_CACHE_DIR");
#endif
    }

    bool active_ = false;
    std::optional<std::string> previous_;
};

std::filesystem::path selectedCacheDirectory(const CacheSetupOptions& options)
{
    if (!options.cacheDirectory.empty()) {
        return options.cacheDirectory;
    }
    return pruning_tables::cacheDirectory();
}

std::size_t missingCacheBytes(const std::filesystem::path& directory, SolveProfile profile)
{
    std::size_t missing = 0;
    for (const detail::TableProfileEntry& entry : detail::optimalTableProfile(profile)) {
        if (!pruning_tables::cacheEntryReady(directory, entry.name, entry.entries)) {
            missing += entry.entries;
        }
    }
    const std::size_t cornerStateEntries =
        static_cast<std::size_t>(coordinates::corner_orientation_count) *
        static_cast<std::size_t>(coordinates::corner_permutation_count);
    if (!pruning_tables::cacheEntryReady(directory, "corner_orientation_permutation", cornerStateEntries)) {
        missing += cornerStateEntries;
    }
    if (profile == SolveProfile::LargeLocal) {
        const std::size_t cornerEdgeGroupEntries =
            static_cast<std::size_t>(coordinates::corner_permutation_count) *
            static_cast<std::size_t>(coordinates::edge_group_permutation_count);
        if (!pruning_tables::cacheEntryReady(
                directory,
                "corner_permutation_up_edge_permutation",
                cornerEdgeGroupEntries)) {
            missing += cornerEdgeGroupEntries;
        }
        if (!pruning_tables::cacheEntryReady(
                directory,
                "corner_permutation_down_edge_permutation",
                cornerEdgeGroupEntries)) {
            missing += cornerEdgeGroupEntries;
        }
    }
    return missing;
}

void warmCacheTables(SolveProfile profile)
{
    for (const detail::TableProfileEntry& entry : detail::optimalTableProfile(profile)) {
        (void)entry.table();
    }
    (void)pruning_tables::cornerOrientationPermutation();
    if (profile == SolveProfile::LargeLocal) {
        (void)pruning_tables::cornerPermutationUpEdgePermutation();
        (void)pruning_tables::cornerPermutationDownEdgePermutation();
    }
}

} // namespace

CacheSetupResult prepareCache(const CacheSetupOptions& options)
{
    const auto started = std::chrono::steady_clock::now();
    const ScopedCacheDirectory scopedCacheDirectory(options.cacheDirectory);

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
    } else {
        const std::filesystem::path cacheDirectory = selectedCacheDirectory(options);
        result.bytesMissing = result.plan.diskCacheEnabled ?
            missingCacheBytes(cacheDirectory, plan.publicPlan.effectiveProfile) :
            0;
        result.cacheWarm = result.plan.diskCacheEnabled && result.bytesMissing == 0;
        result.plan.diskCacheWarm = result.cacheWarm;
    }

    if (!plan.supported) {
    } else if (options.dryRun) {
        result.ready = true;
        result.bytesPrepared = 0;
        result.message = result.cacheWarm ?
            "dry run: cache warm" :
            "dry run: cache cold; run cache setup before latency-sensitive solving";
    } else if (!result.plan.diskCacheEnabled) {
        result.ready = true;
        result.bytesPrepared = 0;
        result.message = "cache disabled";
    } else if (options.cachePolicy == CachePolicy::RequireWarm && !result.cacheWarm) {
        result.ready = false;
        result.bytesPrepared = 0;
        result.message = "cache cold and warm cache is required";
    } else {
        warmCacheTables(plan.publicPlan.effectiveProfile);
        const std::filesystem::path cacheDirectory = selectedCacheDirectory(options);
        result.bytesMissing = missingCacheBytes(cacheDirectory, plan.publicPlan.effectiveProfile);
        result.cacheWarm = result.bytesMissing == 0;
        result.plan.diskCacheWarm = result.cacheWarm;
        result.ready = true;
        result.bytesPrepared = result.plan.estimatedTablePayloadBytes - result.bytesMissing;
        result.message = result.cacheWarm ?
            "cache setup completed" :
            "cache setup completed, but some cache files were not written";
    }

    result.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started);
    return result;
}

} // namespace rubik
