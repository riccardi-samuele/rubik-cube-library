#include "rubik/detail/auto_plan.hpp"

#include "rubik/coordinates.hpp"
#include "rubik/detail/table_profiles.hpp"

#include <algorithm>
#include <thread>

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

SolveProfile autoEffectiveProfile(const SolveOptions& options)
{
    if (options.maxDepth <= 13) {
        return SolveProfile::Performance;
    }
    return SolveProfile::LargeLocal;
}

const char* autoStrategyName(SolveProfile effectiveProfile)
{
    return effectiveProfile == SolveProfile::LargeLocal
        ? "auto_desktop_tail"
        : "auto_shallow_optimal";
}

bool fitsMemoryBudget(std::size_t payloadBytes, std::size_t memoryBudgetBytes)
{
    if (memoryBudgetBytes == 0) {
        return true;
    }
    return payloadBytes <= memoryBudgetBytes;
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

AutoPlanDecision makeAutoPlan(const SolveOptions& options)
{
    AutoPlanDecision decision;

    if (options.mode != SolveMode::Optimal || options.metric != Metric::HTM) {
        decision.supported = false;
        decision.status = SolveStatus::UnsupportedOptions;
        return decision;
    }

    decision.effectiveProfile = autoEffectiveProfile(options);
    decision.effectiveMaxMemoryBytes = autoMemoryBudgetBytes(options);
    decision.effectiveThreads = autoThreadCount(options);
    decision.estimatedTablePayloadBytes = estimatedPayloadForProfile(decision.effectiveProfile);
    decision.boundsUsed = boundsForProfile(decision.effectiveProfile);
    decision.strategyName = autoStrategyName(decision.effectiveProfile);

    if (!fitsMemoryBudget(decision.estimatedTablePayloadBytes, decision.effectiveMaxMemoryBytes)) {
        const std::size_t performancePayload = estimatedPayloadForProfile(SolveProfile::Performance);
        if (fitsMemoryBudget(performancePayload, decision.effectiveMaxMemoryBytes)) {
            decision.effectiveProfile = SolveProfile::Performance;
            decision.estimatedTablePayloadBytes = performancePayload;
            decision.boundsUsed = boundsForProfile(decision.effectiveProfile);
            decision.strategyName = "auto_memory_fallback";
            return decision;
        }

        decision.supported = false;
        decision.status = SolveStatus::MemoryLimitExceeded;
        return decision;
    }

    return decision;
}

} // namespace rubik::detail
