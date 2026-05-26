#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "rubik/cube.hpp"
#include "rubik/move.hpp"

namespace rubik {

enum class Metric {
    HTM,
    QTM
};

enum class SolveMode {
    Optimal,
    Fast,
    Balanced
};

enum class SolveProfile {
    Embedded,
    Default,
    Performance,
    LargeLocal
};

enum class SolveStatus {
    Solved,
    Found,
    Optimal,
    Timeout,
    MemoryLimitExceeded,
    DepthLimitExceeded,
    InvalidCube,
    UnsupportedOptions,
    InternalError
};

struct SolveOptions {
    SolveMode mode = SolveMode::Optimal;
    Metric metric = Metric::HTM;
    int maxDepth = 20;
    std::chrono::milliseconds timeout{0};
    std::size_t maxMemoryBytes = 0;
    unsigned int threads = 1;
    SolveProfile profile = SolveProfile::Default;
    bool collectDiagnostics = false;
};

struct SolveBoundDiagnostics {
    std::uint64_t cheapNodePrunes = 0;
    std::uint64_t threePhaseNodeChecks = 0;
    std::uint64_t threePhaseNodePrunes = 0;
    std::uint64_t cheapCandidatePrunes = 0;
    std::uint64_t threePhaseCandidateChecks = 0;
    std::uint64_t threePhaseCandidatePrunes = 0;
};

struct SolveResult {
    SolveStatus status = SolveStatus::InvalidCube;
    std::vector<Move> moves;
    int moveCount = -1;
    bool isOptimal = false;
    Metric metric = Metric::HTM;
    std::chrono::milliseconds elapsed{0};
    std::uint64_t nodesExpanded = 0;
    std::size_t memoryUsedBytes = 0;
    std::vector<std::uint64_t> nodesByDepth;
    SolveBoundDiagnostics boundDiagnostics;
};

class Solver {
public:
    SolveResult solve(const Cube& cube, const SolveOptions& options = {}) const;
    int lowerBound(
        const Cube& cube,
        Metric metric = Metric::HTM,
        SolveProfile profile = SolveProfile::Default) const;
};

} // namespace rubik
