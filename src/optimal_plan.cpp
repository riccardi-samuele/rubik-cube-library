#include "rubik/detail/optimal_plan.hpp"

#include "rubik/coordinates.hpp"
#include "rubik/detail/table_profiles.hpp"

#include <algorithm>
#include <thread>
#include <vector>

namespace rubik::detail {
namespace {

constexpr std::size_t auto_default_memory_bytes = 2ull * 1024ull * 1024ull * 1024ull;
constexpr unsigned int auto_default_thread_cap = 16;

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
    std::size_t bytes = optimalTablePayloadBytes(profile);
    bytes += static_cast<std::size_t>(coordinates::corner_orientation_count) *
        static_cast<std::size_t>(coordinates::corner_permutation_count);
    if (profile == SolveProfile::LargeLocal) {
        bytes += 2ull *
            static_cast<std::size_t>(coordinates::corner_permutation_count) *
            static_cast<std::size_t>(coordinates::edge_group_permutation_count);
    }
    return bytes;
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
