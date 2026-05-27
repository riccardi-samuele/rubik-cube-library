#include "rubik/detail/optimal_plan.hpp"

#include "rubik/coordinates.hpp"
#include "rubik/detail/auto_plan.hpp"
#include "rubik/detail/cache_status.hpp"
#include "rubik/detail/table_profiles.hpp"

#include <vector>

namespace rubik::detail {
namespace {

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

bool autoStrongMoveOrderingEnabled(
    const SolveOptions& requestedOptions,
    const SolveOptions& effectiveOptions,
    int initialLowerBound,
    bool autoOrderingAllowed,
    bool rootFirstMoveDiffers,
    int rootStrongMinCount)
{
    return autoOrderingAllowed &&
        requestedOptions.mode == SolveMode::Optimal &&
        requestedOptions.metric == Metric::HTM &&
        requestedOptions.profile == SolveProfile::Auto &&
        effectiveOptions.profile == SolveProfile::LargeLocal &&
        (initialLowerBound == 9 ||
         (initialLowerBound == 8 && rootFirstMoveDiffers && rootStrongMinCount <= 6));
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
    plan.publicPlan.diskCacheEnabled = options.cachePolicy != CachePolicy::Disabled;

    if (options.profile == SolveProfile::Auto) {
        AutoPlanDecision autoPlan = makeAutoPlan(options);
        if (autoPlan.supported &&
            (options.cachePolicy == CachePolicy::RequireWarm || options.timeout.count() > 0)) {
            const bool selectedProfileCacheWarm =
                plan.publicPlan.diskCacheEnabled && profileCacheWarm(autoPlan.effectiveProfile);
            autoPlan = makeAutoPlan(options, selectedProfileCacheWarm);
        }

        plan.effectiveOptions.profile = autoPlan.effectiveProfile;
        plan.effectiveOptions.maxMemoryBytes = autoPlan.effectiveMaxMemoryBytes;
        plan.effectiveOptions.threads = autoPlan.effectiveThreads;

        plan.publicPlan.effectiveProfile = autoPlan.effectiveProfile;
        plan.publicPlan.effectiveMaxMemoryBytes = autoPlan.effectiveMaxMemoryBytes;
        plan.publicPlan.effectiveThreads = autoPlan.effectiveThreads;
        plan.publicPlan.estimatedTablePayloadBytes = autoPlan.estimatedTablePayloadBytes;
        plan.publicPlan.boundsUsed = autoPlan.boundsUsed;
        plan.publicPlan.strategyName = autoPlan.strategyName;
        plan.publicPlan.diskCacheWarm = autoPlan.supported && options.cachePolicy == CachePolicy::RequireWarm;

        if (!autoPlan.supported) {
            plan.supported = false;
            plan.status = autoPlan.status;
            return plan;
        }
    } else {
        plan.effectiveOptions.profile = options.profile;
        plan.effectiveOptions.maxMemoryBytes = options.maxMemoryBytes;
        plan.effectiveOptions.threads = options.threads;

        plan.publicPlan.effectiveProfile = options.profile;
        plan.publicPlan.effectiveMaxMemoryBytes = options.maxMemoryBytes;
        plan.publicPlan.effectiveThreads = options.threads;
        plan.publicPlan.estimatedTablePayloadBytes = estimatedPayloadForProfile(options.profile);
        plan.publicPlan.boundsUsed = boundsForProfile(options.profile);
        plan.publicPlan.strategyName = "manual_profile";
    }
    plan.supported = true;
    plan.status = SolveStatus::Found;
    return plan;
}

} // namespace rubik::detail
