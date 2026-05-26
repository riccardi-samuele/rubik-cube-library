#include "rubik/pruning_tables.hpp"
#include "rubik/solver.hpp"

#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <string>

#ifndef RUBIK_VERSION
#define RUBIK_VERSION "unknown"
#endif

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
        << "Usage: " << program << " <54-stickers> [--mode optimal|fast] [--timeout-ms N] [--max-depth N]"
        << " [--max-memory-mb N] [--threads N] [--profile default|embedded|performance|large-local]\n"
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

std::optional<long long> parseInteger(const std::string& value, long long minValue, long long maxValue)
{
    if (value.empty()) {
        return std::nullopt;
    }
    char* end = nullptr;
    errno = 0;
    const long long parsed = std::strtoll(value.c_str(), &end, 10);
    if (errno != 0 || end == value.c_str() || *end != '\0' || parsed < minValue || parsed > maxValue) {
        return std::nullopt;
    }
    return parsed;
}

std::optional<std::uint64_t> parseUnsignedInteger(const std::string& value)
{
    if (value.empty() || value[0] == '-') {
        return std::nullopt;
    }
    char* end = nullptr;
    errno = 0;
    const unsigned long long parsed = std::strtoull(value.c_str(), &end, 10);
    if (errno != 0 || end == value.c_str() || *end != '\0') {
        return std::nullopt;
    }
    return static_cast<std::uint64_t>(parsed);
}

} // namespace

int main(int argc, char** argv)
{
    if (argc == 2) {
        const std::string firstArg = argv[1];
        if (firstArg == "--help" || firstArg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        if (firstArg == "--version" || firstArg == "-V") {
            std::cout << "rubik-solve " << RUBIK_VERSION << "\n";
            return 0;
        }
    }

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

    const auto requireValue = [&](const std::string& option, int& index) -> std::optional<std::string> {
        if (index + 1 >= argc) {
            std::cerr << "Missing value for " << option << "\n";
            printUsage(argv[0]);
            return std::nullopt;
        }
        return std::string(argv[++index]);
    };

    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--timeout-ms") {
            const auto value = requireValue(arg, i);
            if (!value) {
                return 2;
            }
            const auto parsed = parseInteger(*value, 0, std::numeric_limits<long long>::max());
            if (!parsed) {
                std::cerr << "Invalid timeout-ms: " << *value << "\n";
                printUsage(argv[0]);
                return 2;
            }
            options.timeout = std::chrono::milliseconds(*parsed);
        } else if (arg == "--max-depth") {
            const auto value = requireValue(arg, i);
            if (!value) {
                return 2;
            }
            const auto parsed = parseInteger(*value, 0, 1000);
            if (!parsed) {
                std::cerr << "Invalid max-depth: " << *value << "\n";
                printUsage(argv[0]);
                return 2;
            }
            options.maxDepth = static_cast<int>(*parsed);
        } else if (arg == "--max-memory-mb") {
            const auto value = requireValue(arg, i);
            if (!value) {
                return 2;
            }
            const auto parsed = parseUnsignedInteger(*value);
            if (!parsed || *parsed > std::numeric_limits<std::size_t>::max() / (1024ull * 1024ull)) {
                std::cerr << "Invalid max-memory-mb: " << *value << "\n";
                printUsage(argv[0]);
                return 2;
            }
            options.maxMemoryBytes = static_cast<std::size_t>(*parsed) * 1024ull * 1024ull;
        } else if (arg == "--threads") {
            const auto value = requireValue(arg, i);
            if (!value) {
                return 2;
            }
            const auto parsed = parseInteger(*value, 1, std::numeric_limits<unsigned int>::max());
            if (!parsed) {
                std::cerr << "Invalid threads: " << *value << "\n";
                printUsage(argv[0]);
                return 2;
            }
            options.threads = static_cast<unsigned int>(*parsed);
        } else if (arg == "--profile") {
            const auto value = requireValue(arg, i);
            if (!value) {
                return 2;
            }
            const auto profile = parseProfile(*value);
            if (!profile) {
                std::cerr << "Invalid profile: " << *value << "\n";
                printUsage(argv[0]);
                return 2;
            }
            options.profile = *profile;
        } else if (arg == "--mode") {
            const auto value = requireValue(arg, i);
            if (!value) {
                return 2;
            }
            const auto mode = parseMode(*value);
            if (!mode) {
                std::cerr << "Invalid mode: " << *value << "\n";
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
