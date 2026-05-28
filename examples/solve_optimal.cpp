#include "rubik/cube.hpp"
#include "rubik/move.hpp"
#include "rubik/solver.hpp"

#include <chrono>
#include <iostream>

namespace {

const char* profileName(rubik::SolveProfile profile)
{
    switch (profile) {
    case rubik::SolveProfile::Embedded:
        return "embedded";
    case rubik::SolveProfile::Default:
        return "default";
    case rubik::SolveProfile::Performance:
        return "performance";
    case rubik::SolveProfile::LargeLocal:
        return "large-local";
    case rubik::SolveProfile::Auto:
        return "auto";
    }
    return "unknown";
}

} // namespace

int main()
{
    const auto parsed = rubik::Cube::fromStickers(
        "UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB");
    if (!parsed) {
        std::cerr << "invalid cube: " << parsed.error.message << "\n";
        return 1;
    }

    rubik::Cube cube = parsed.cube;
    cube.apply(rubik::parseMoves("R U F"));

    rubik::Solver solver;
    const rubik::SolveResult result = solver.solve(cube, {
        .mode = rubik::SolveMode::Optimal,
        .metric = rubik::Metric::HTM,
        .maxDepth = 6,
        .timeout = std::chrono::seconds(30),
        .threads = 0,
        .profile = rubik::SolveProfile::Auto,
        .cachePolicy = rubik::CachePolicy::Auto,
    });

    if (result.status != rubik::SolveStatus::Optimal &&
        result.status != rubik::SolveStatus::Solved) {
        std::cerr << "no certified optimal solution found\n";
        return 1;
    }

    std::cout << "optimal: true\n";
    std::cout << "moves: " << result.moveCount << "\n";
    std::cout << "solution: " << rubik::formatMoves(result.moves) << "\n";
    std::cout << "effective_profile: " << profileName(result.plan.effectiveProfile) << "\n";
    std::cout << "effective_threads: " << result.plan.effectiveThreads << "\n";
    std::cout << "strategy: " << result.plan.strategyName << "\n";
    return 0;
}
