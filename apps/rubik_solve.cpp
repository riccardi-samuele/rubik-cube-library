#include "rubik/detail/move_restrictions.hpp"
#include "rubik/pruning_tables.hpp"
#include "rubik/solver.hpp"
#include "rubik/version.hpp"

#include <cctype>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <limits>
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
    case rubik::SolveStatus::CacheNotReady:
        return "CacheNotReady";
    case rubik::SolveStatus::InternalError:
        return "InternalError";
    }
    return "Unknown";
}

void printUsage(const char* program)
{
    std::cerr
        << "Usage: " << program << " <54-stickers> [--mode optimal|fast] [--timeout-ms N] [--max-depth N]"
        << " [--max-memory-mb N] [--threads N] [--profile auto|default|embedded|performance|large-local]"
        << " [--cache-policy auto|require-warm|allow-build|disabled] [--blocked-faces FACE[,FACE]]\n"
        << "Input order: U R F D L B, each face left-to-right top-to-bottom.\n";
}

std::optional<rubik::SolveProfile> parseProfile(const std::string& value)
{
    if (value == "auto") {
        return rubik::SolveProfile::Auto;
    }
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

std::optional<rubik::CachePolicy> parseCachePolicy(const std::string& value)
{
    if (value == "auto") {
        return rubik::CachePolicy::Auto;
    }
    if (value == "require-warm") {
        return rubik::CachePolicy::RequireWarm;
    }
    if (value == "allow-build") {
        return rubik::CachePolicy::AllowBuild;
    }
    if (value == "disabled") {
        return rubik::CachePolicy::Disabled;
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

std::string_view trimAsciiWhitespace(std::string_view value)
{
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.remove_prefix(1);
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.remove_suffix(1);
    }
    return value;
}

std::optional<rubik::Face> parseFaceToken(std::string_view value)
{
    value = trimAsciiWhitespace(value);
    if (value.size() != 1) {
        return std::nullopt;
    }

    switch (std::toupper(static_cast<unsigned char>(value.front()))) {
    case 'U':
        return rubik::Face::U;
    case 'R':
        return rubik::Face::R;
    case 'F':
        return rubik::Face::F;
    case 'D':
        return rubik::Face::D;
    case 'L':
        return rubik::Face::L;
    case 'B':
        return rubik::Face::B;
    default:
        return std::nullopt;
    }
}

struct BlockedFacesParseResult {
    bool ok = false;
    std::vector<rubik::Face> faces;
    std::string invalidToken;
};

BlockedFacesParseResult parseBlockedFaces(const std::string& value)
{
    BlockedFacesParseResult result;
    std::size_t start = 0;

    while (start <= value.size()) {
        const std::size_t comma = value.find(',', start);
        const std::string_view token = comma == std::string::npos
            ? std::string_view(value).substr(start)
            : std::string_view(value).substr(start, comma - start);
        const auto face = parseFaceToken(token);
        if (!face) {
            result.invalidToken = std::string(trimAsciiWhitespace(token));
            return result;
        }
        result.faces.push_back(*face);

        if (comma == std::string::npos) {
            result.ok = true;
            return result;
        }
        start = comma + 1;
    }

    return result;
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
            std::cout << "rubik-solve " << rubik::version_string << "\n";
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
            const auto parsed = parseInteger(*value, 0, std::numeric_limits<unsigned int>::max());
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
        } else if (arg == "--cache-policy") {
            const auto value = requireValue(arg, i);
            if (!value) {
                return 2;
            }
            const auto cachePolicy = parseCachePolicy(*value);
            if (!cachePolicy) {
                std::cerr << "Invalid cache policy: " << *value << "\n";
                printUsage(argv[0]);
                return 2;
            }
            options.cachePolicy = *cachePolicy;
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
        } else if (arg == "--blocked-faces") {
            const auto value = requireValue(arg, i);
            if (!value) {
                return 2;
            }
            const BlockedFacesParseResult parsed = parseBlockedFaces(*value);
            if (!parsed.ok) {
                std::cerr << "Invalid blocked face: "
                          << (parsed.invalidToken.empty() ? *value : parsed.invalidToken) << "\n";
                printUsage(argv[0]);
                return 2;
            }
            options.blockedFaces = parsed.faces;
        } else {
            printUsage(argv[0]);
            return 2;
        }
    }

    if (rubik::detail::allowedMovesForBlockedFaces(options.blockedFaces).status != rubik::SolveStatus::Found) {
        std::cerr << "Blocked faces must contain 1 face or 2 opposite faces\n";
        printUsage(argv[0]);
        return 2;
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
    std::cout << "effective-profile: " << static_cast<int>(result.plan.effectiveProfile) << "\n";
    std::cout << "effective-threads: " << result.plan.effectiveThreads << "\n";
    std::cout << "effective-memory-bytes: " << result.plan.effectiveMaxMemoryBytes << "\n";
    std::cout << "strategy: " << result.plan.strategyName << "\n";
    std::cout << "optimal-move-ordering: " << result.plan.optimalMoveOrdering << "\n";

    return result.status == rubik::SolveStatus::Optimal ||
            result.status == rubik::SolveStatus::Found ||
            result.status == rubik::SolveStatus::Solved
        ? 0
        : 1;
}
