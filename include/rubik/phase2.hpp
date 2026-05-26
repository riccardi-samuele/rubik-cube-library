#pragma once

#include <chrono>
#include <cstdint>
#include <vector>

#include "rubik/cube.hpp"
#include "rubik/move.hpp"
#include "rubik/solver.hpp"

namespace rubik {

struct Phase2Options {
    int maxDepth = 18;
    std::chrono::milliseconds timeout{0};
    SolveProfile profile = SolveProfile::Default;
};

struct Phase2Result {
    SolveStatus status = SolveStatus::InvalidCube;
    std::vector<Move> moves;
    int moveCount = -1;
    std::chrono::milliseconds elapsed{0};
    std::uint64_t nodesExpanded = 0;
    std::vector<std::uint64_t> nodesByDepth;
};

Phase2Result solvePhase2(const Cube& cube, const Phase2Options& options = {});

namespace experimental {

using ::rubik::Phase2Options;
using ::rubik::Phase2Result;

inline Phase2Result solvePhase2(const Cube& cube, const Phase2Options& options = {})
{
    return ::rubik::solvePhase2(cube, options);
}

} // namespace experimental

} // namespace rubik
