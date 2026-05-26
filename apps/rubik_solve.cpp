#include "rubik/pruning_tables.hpp"
#include "rubik/solver.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>

namespace {

std::string statusName(rubik::SolveStatus status)
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
    case rubik::SolveStatus::InternalError:
        return "InternalError";
    }
    return "Unknown";
}

void printUsage(const char* program)
{
    std::cerr
        << "Usage: " << program << " <54-stickers> [--mode optimal|fast] [--timeout-ms N] [--max-depth N] [--profile default|embedded|performance|large-local]\n"
        << "Input order: U R F D L B, each face left-to-right top-to-bottom.\n";
}

std::optional<rubik::SolveProfile> parseProfile(const std::string& value)
{
    if (value == "embedded") {
        return rubik::SolveProfile::Embedded;
    }
    if (value == "default") {
        return rubik::SolveProfile::Default;
    }
    if (value == "performance") {
        return rubik::SolveProfile::Performance;
    }
    if (value == "large-local" || value == "large_local") {
        return rubik::SolveProfile::LargeLocal;
    }
    return std::nullopt;
}

std::optional<rubik::SolveMode> parseMode(const std::string& value)
{
    if (value == "optimal") {
        return rubik::SolveMode::Optimal;
    }
    if (value == "fast") {
        return rubik::SolveMode::Fast;
    }
    return std::nullopt;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2) {
        printUsage(argv[0]);
        return 2;
    }

    std::string stickers = argv[1];
    rubik::SolveOptions options{
        .mode = rubik::SolveMode::Optimal,
        .metric = rubik::Metric::HTM,
        .maxDepth = 20,
        .timeout = std::chrono::seconds(30),
        .maxMemoryBytes = 1024ull * 1024 * 1024,
        .threads = 1,
    };

    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--timeout-ms" && i + 1 < argc) {
            options.timeout = std::chrono::milliseconds(std::strtoll(argv[++i], nullptr, 10));
        } else if (arg == "--max-depth" && i + 1 < argc) {
            options.maxDepth = static_cast<int>(std::strtol(argv[++i], nullptr, 10));
        } else if (arg == "--profile" && i + 1 < argc) {
            const std::string value = argv[++i];
            const auto profile = parseProfile(value);
            if (!profile) {
                std::cerr << "Invalid profile: " << value << "\n";
                printUsage(argv[0]);
                return 2;
            }
            options.profile = *profile;
        } else if (arg == "--mode" && i + 1 < argc) {
            const std::string value = argv[++i];
            const auto mode = parseMode(value);
            if (!mode) {
                std::cerr << "Invalid mode: " << value << "\n";
                printUsage(argv[0]);
                return 2;
            }
            options.mode = *mode;
        } else {
            printUsage(argv[0]);
            return 2;
        }
    }

    const auto cube = rubik::Cube::fromStickers(stickers);
    if (!cube) {
        std::cerr << "Invalid cube: " << cube.error.message << "\n";
        return 1;
    }

    rubik::Solver solver;
    const rubik::SolveResult result = solver.solve(cube.cube, options);

    std::cout << "status: " << statusName(result.status) << "\n";
    std::cout << "optimal: " << (result.isOptimal ? "true" : "false") << "\n";
    std::cout << "moves: " << result.moveCount << "\n";
    std::cout << "solution: " << rubik::formatMoves(result.moves) << "\n";
    std::cout << "elapsed_ms: " << result.elapsed.count() << "\n";
    std::cout << "nodes_expanded: " << result.nodesExpanded << "\n";
    std::cout << "cache_dir: " << rubik::pruning_tables::cacheDirectory() << "\n";

    return result.status == rubik::SolveStatus::Optimal ||
            result.status == rubik::SolveStatus::Found ||
            result.status == rubik::SolveStatus::Solved
        ? 0
        : 1;
}
