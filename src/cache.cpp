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
