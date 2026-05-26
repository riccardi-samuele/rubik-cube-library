#include "rubik/cube.hpp"
#include "rubik/move.hpp"
#include "rubik/solver.hpp"

#include <chrono>
#include <iostream>

int main()
{
    rubik::Cube cube = rubik::Cube::solved();
    cube.apply(rubik::parseMoves("R U F"));

    rubik::Solver solver;
    const rubik::SolveResult result = solver.solve(cube, {
        .mode = rubik::SolveMode::Optimal,
        .metric = rubik::Metric::HTM,
        .maxDepth = 6,
        .timeout = std::chrono::seconds(10),
        .maxMemoryBytes = 1024ull * 1024 * 1024,
        .threads = 1,
        .profile = rubik::SolveProfile::Default,
    });

    if (result.status != rubik::SolveStatus::Optimal &&
        result.status != rubik::SolveStatus::Solved) {
        std::cerr << "no optimal solution found\n";
        return 1;
    }

    std::cout << "optimal: true\n";
    std::cout << "moves: " << result.moveCount << "\n";
    std::cout << "solution: " << rubik::formatMoves(result.moves) << "\n";
    return 0;
}
