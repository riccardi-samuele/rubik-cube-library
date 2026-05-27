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
bool autoStrongMoveOrderingEnabled(
    const SolveOptions& requestedOptions,
    const SolveOptions& effectiveOptions,
    int initialLowerBound,
    bool autoOrderingAllowed = true,
    bool rootFirstMoveDiffers = false,
    int rootStrongMinCount = 0);
OptimalPlan makeOptimalPlan(const SolveOptions& options);

} // namespace rubik::detail
