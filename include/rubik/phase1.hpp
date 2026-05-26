#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "rubik/cube.hpp"
#include "rubik/move.hpp"
#include "rubik/solver.hpp"

namespace rubik {

struct Phase1Options {
    int maxDepth = 12;
    std::chrono::milliseconds timeout{0};
    SolveProfile profile = SolveProfile::Default;
    std::size_t maxCandidates = 1;
};

struct Phase1Result {
    SolveStatus status = SolveStatus::InvalidCube;
    std::vector<Move> moves;
    int moveCount = -1;
    std::chrono::milliseconds elapsed{0};
    std::uint64_t nodesExpanded = 0;
    std::vector<std::uint64_t> nodesByDepth;
};

struct Phase1CandidatesResult {
    SolveStatus status = SolveStatus::InvalidCube;
    std::vector<std::vector<Move>> candidates;
    std::chrono::milliseconds elapsed{0};
    std::uint64_t nodesExpanded = 0;
    std::vector<std::uint64_t> nodesByDepth;
};

bool isPhase1Solved(const Cube& cube);
Phase1CandidatesResult findPhase1Candidates(const Cube& cube, const Phase1Options& options = {});
Phase1Result solvePhase1(const Cube& cube, const Phase1Options& options = {});

namespace experimental {

using ::rubik::Phase1CandidatesResult;
using ::rubik::Phase1Options;
using ::rubik::Phase1Result;

inline bool isPhase1Solved(const Cube& cube)
{
    return ::rubik::isPhase1Solved(cube);
}

inline Phase1CandidatesResult findPhase1Candidates(
    const Cube& cube,
    const Phase1Options& options = {})
{
    return ::rubik::findPhase1Candidates(cube, options);
}

inline Phase1Result solvePhase1(const Cube& cube, const Phase1Options& options = {})
{
    return ::rubik::solvePhase1(cube, options);
}

} // namespace experimental

} // namespace rubik
