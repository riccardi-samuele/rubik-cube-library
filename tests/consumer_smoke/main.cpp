#include <rubik/cube.hpp>
#include <rubik/move.hpp>
#include <rubik/solver.hpp>
#include <rubik/version.hpp>

#include <chrono>
#include <iostream>

int main()
{
    static_assert(rubik::version_major == 1);
    static_assert(rubik::version_minor == 0);
    static_assert(rubik::version_patch == 0);

    auto parsed = rubik::Cube::fromStickers(
        "UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB");
    if (!parsed) {
        std::cerr << "failed to parse solved cube\n";
        return 1;
    }

    rubik::Cube cube = parsed.cube;
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

    if (result.status != rubik::SolveStatus::Optimal || !result.isOptimal) {
        std::cerr << "expected an optimal solution from installed package\n";
        return 1;
    }

    cube.apply(result.moves);
    if (!cube.isSolved()) {
        std::cerr << "installed package returned a non-solving sequence\n";
        return 1;
    }

    std::cout << "consumer smoke passed: " << rubik::formatMoves(result.moves) << "\n";
    return 0;
}
