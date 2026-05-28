#include "rubik/cube.hpp"
#include "rubik/move.hpp"
#include "rubik/solver.hpp"

#include <chrono>
#include <iostream>

namespace {

const char* statusName(rubik::SolveStatus status)
{
    switch (status) {
    case rubik::SolveStatus::Solved:
        return "Solved";
    case rubik::SolveStatus::Found:
        return "Found";
    case rubik::SolveStatus::Optimal:
        return "Optimal";
    case rubik::SolveStatus::Timeout:
        return "Timeout";
    case rubik::SolveStatus::MemoryLimitExceeded:
        return "MemoryLimitExceeded";
    case rubik::SolveStatus::DepthLimitExceeded:
        return "DepthLimitExceeded";
    case rubik::SolveStatus::InvalidCube:
        return "InvalidCube";
    case rubik::SolveStatus::UnsupportedOptions:
        return "UnsupportedOptions";
    case rubik::SolveStatus::CacheNotReady:
        return "CacheNotReady";
    case rubik::SolveStatus::InternalError:
        return "InternalError";
    }
    return "Unknown";
}

} // namespace

int main()
{
    rubik::Cube cube = rubik::Cube::solved();
    cube.apply(rubik::parseMoves("R U R' U' F2 D L2 B R2 F"));

    rubik::Solver solver;
    const rubik::SolveResult result = solver.solve(cube, {
        .mode = rubik::SolveMode::Fast,
        .metric = rubik::Metric::HTM,
        .maxDepth = 24,
        .timeout = std::chrono::seconds(5),
        .maxMemoryBytes = 1024ull * 1024 * 1024,
        .threads = 1,
        .profile = rubik::SolveProfile::Default,
    });

    if (result.status != rubik::SolveStatus::Found &&
        result.status != rubik::SolveStatus::Solved) {
        std::cerr << "no fast solution found\n";
        return 1;
    }

    std::cout << "non_optimal: true\n";
    std::cout << "status: " << statusName(result.status) << "\n";
    std::cout << "moves: " << result.moveCount << "\n";
    std::cout << "solution: " << rubik::formatMoves(result.moves) << "\n";
    std::cout << "elapsed_ms: " << result.elapsed.count() << "\n";
    return 0;
}
