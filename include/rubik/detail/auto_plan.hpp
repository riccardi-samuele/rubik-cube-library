#pragma once

#include "rubik/solver.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace rubik::detail {

struct AutoPlanDecision {
    bool supported = true;
    SolveStatus status = SolveStatus::Found;
    SolveProfile effectiveProfile = SolveProfile::Default;
    std::size_t effectiveMaxMemoryBytes = 0;
    unsigned int effectiveThreads = 1;
    std::size_t estimatedTablePayloadBytes = 0;
    std::vector<std::string> boundsUsed;
    const char* strategyName = "manual_profile";
};

std::size_t autoMemoryBudgetBytes(const SolveOptions& options);
unsigned int autoThreadCount(const SolveOptions& options);
AutoPlanDecision makeAutoPlan(const SolveOptions& options, bool selectedProfileCacheWarm = true);

} // namespace rubik::detail
