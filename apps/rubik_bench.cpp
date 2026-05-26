#include "rubik/coordinates.hpp"
#include "rubik/cubie_cube.hpp"
#include "rubik/detail/symmetry_coordinates.hpp"
#include "rubik/detail/symmetry_pruning.hpp"
#include "rubik/detail/table_profiles.hpp"
#include "rubik/pruning_tables.hpp"
#include "rubik/phase1.hpp"
#include "rubik/phase2.hpp"
#include "rubik/solver.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <random>
#include <span>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct Case {
    std::string name;
    int depth = 0;
    std::string scramble;
};

struct BenchmarkRow {
    Case item;
    rubik::SolveStatus status = rubik::SolveStatus::InternalError;
    bool isOptimal = false;
    int moveCount = -1;
    int initialLowerBound = -1;
    std::int64_t elapsedMs = 0;
    std::uint64_t nodesExpanded = 0;
    double nodesPerMs = 0.0;
    std::string nodesByDepth;
    std::string solution;
};

struct FastAttemptOptions {
    std::string name;
    std::size_t phase1CandidateLimit = 1;
    std::size_t phase2CandidateLimit = 0;
    std::chrono::milliseconds phase1Timeout{0};
    std::chrono::milliseconds phase2Timeout{0};
};

enum class CaseSet {
    Deterministic,
    Random,
    Both
};

std::vector<Case> benchmarkCases()
{
    return {
        {"depth_1", 1, "R"},
        {"depth_2", 2, "R U"},
        {"depth_3", 3, "R U F"},
        {"depth_4", 4, "R U R' U'"},
        {"depth_5", 5, "F R U R' U'"},
        {"depth_6", 6, "R U R' U' F2 D"},
        {"depth_7", 7, "R U R' U' F2 D L2"},
        {"depth_8", 8, "R U R' U' F2 D L2 B"},
        {"depth_9", 9, "R U R' U' F2 D L2 B R2"},
        {"depth_10", 10, "R U R' U' F2 D L2 B R2 F"},
        {"depth_11", 11, "R U R' U' F2 D L2 B R2 F D'"},
        {"depth_12", 12, "R U R' U' F2 D L2 B R2 F D' L"},
        {"depth_13", 13, "R U R' U' F2 D L2 B R2 F D' L U2"},
        {"depth_14", 14, "R U R' U' F2 D L2 B R2 F D' L U2 B'"},
        {"depth_15", 15, "R U R' U' F2 D L2 B R2 F D' L U2 B' R"},
        {"depth_16", 16, "R U R' U' F2 D L2 B R2 F D' L U2 B' R F2"},
    };
}

bool isRedundantScrambleMove(rubik::Move previous, rubik::Move current)
{
    return rubik::faceOf(previous) == rubik::faceOf(current);
}

std::string generateRandomScramble(std::mt19937_64& rng, int depth)
{
    std::vector<rubik::Move> moves;
    moves.reserve(static_cast<std::size_t>(depth));

    const auto& allMoves = rubik::allMoves();
    std::uniform_int_distribution<std::size_t> distribution(0, allMoves.size() - 1);

    while (static_cast<int>(moves.size()) < depth) {
        const rubik::Move move = allMoves[distribution(rng)];
        if (!moves.empty() && isRedundantScrambleMove(moves.back(), move)) {
            continue;
        }
        moves.push_back(move);
    }

    return rubik::formatMoves(moves);
}

std::vector<Case> randomBenchmarkCases(int count, int depth, std::uint64_t seed, int startIndex)
{
    std::vector<Case> cases;
    cases.reserve(static_cast<std::size_t>(std::max(0, count)));

    std::mt19937_64 rng(seed);
    for (int i = 1; i < startIndex; ++i) {
        (void)generateRandomScramble(rng, depth);
    }

    for (int i = 0; i < count; ++i) {
        const int caseIndex = startIndex + i;
        cases.push_back({
            .name = "random_" + std::to_string(seed) + "_" + std::to_string(caseIndex),
            .depth = depth,
            .scramble = generateRandomScramble(rng, depth),
        });
    }

    return cases;
}

std::vector<Case> selectedCases(
    CaseSet caseSet,
    int maxCaseDepth,
    int randomCount,
    int randomDepth,
    std::uint64_t randomSeed,
    int randomStartIndex)
{
    std::vector<Case> cases;

    if (caseSet == CaseSet::Deterministic || caseSet == CaseSet::Both) {
        for (const Case& item : benchmarkCases()) {
            if (item.depth <= maxCaseDepth) {
                cases.push_back(item);
            }
        }
    }

    if (caseSet == CaseSet::Random || caseSet == CaseSet::Both) {
        std::vector<Case> randomCases = randomBenchmarkCases(randomCount, randomDepth, randomSeed, randomStartIndex);
        cases.insert(cases.end(), randomCases.begin(), randomCases.end());
    }

    return cases;
}

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
    if (value == "balanced") {
        return rubik::SolveMode::Balanced;
    }
    return std::nullopt;
}

std::optional<CaseSet> parseCaseSet(const std::string& value)
{
    if (value == "deterministic") {
        return CaseSet::Deterministic;
    }
    if (value == "random") {
        return CaseSet::Random;
    }
    if (value == "both") {
        return CaseSet::Both;
    }
    return std::nullopt;
}

std::string modeName(rubik::SolveMode mode)
{
    switch (mode) {
    case rubik::SolveMode::Optimal:
        return "optimal";
    case rubik::SolveMode::Fast:
        return "fast";
    case rubik::SolveMode::Balanced:
        return "balanced";
    }
    return "unknown";
}

std::string profileName(rubik::SolveProfile profile)
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
    }
    return "unknown";
}

bool environmentFlagEnabled(const char* name)
{
    const char* value = std::getenv(name);
    return value != nullptr && value[0] != '\0' && value[0] != '0';
}

struct ThreePhase1Policy {
    std::string name;
    bool effective = false;
};

ThreePhase1Policy threePhase1SolvePolicy(const rubik::SolveOptions& options)
{
    if (environmentFlagEnabled("RUBIK_DISABLE_THREE_PHASE1_BOUNDS")) {
        return {.name = "disabled_by_env", .effective = false};
    }
    if (environmentFlagEnabled("RUBIK_EXPERIMENTAL_THREE_PHASE1_BOUNDS")) {
        return {.name = "forced_by_env", .effective = true};
    }

    const bool enabled = options.mode == rubik::SolveMode::Optimal;
    return {
        .name = enabled ? "default_enabled" : "default_disabled",
        .effective = enabled,
    };
}

ThreePhase1Policy threePhase1LowerBoundPolicy(rubik::SolveProfile profile)
{
    if (environmentFlagEnabled("RUBIK_DISABLE_THREE_PHASE1_BOUNDS")) {
        return {.name = "disabled_by_env", .effective = false};
    }
    if (environmentFlagEnabled("RUBIK_EXPERIMENTAL_THREE_PHASE1_BOUNDS")) {
        return {.name = "forced_by_env", .effective = true};
    }

    (void)profile;
    const bool enabled = true;
    return {
        .name = enabled ? "default_enabled" : "default_disabled",
        .effective = enabled,
    };
}

std::string cornerStateBoundsPolicy()
{
    if (environmentFlagEnabled("RUBIK_DISABLE_CORNER_STATE_BOUNDS")) {
        return "disabled_by_env";
    }
    if (environmentFlagEnabled("RUBIK_EXPERIMENTAL_CORNER_STATE_BOUNDS")) {
        return "forced_by_env";
    }
    return "default_enabled";
}

bool cornerStateBoundsEnabled()
{
    return cornerStateBoundsPolicy() != "disabled_by_env";
}

bool largeLocalOptimalProfile(const rubik::SolveOptions& options)
{
    return options.mode == rubik::SolveMode::Optimal && options.profile == rubik::SolveProfile::LargeLocal;
}

std::string cornerUpEdgeBoundsPolicy(const rubik::SolveOptions& options)
{
    if (largeLocalOptimalProfile(options)) {
        return "profile_enabled";
    }
    return environmentFlagEnabled("RUBIK_EXPERIMENTAL_CORNER_UP_EDGE_BOUNDS") ? "enabled" : "disabled";
}

std::string cornerDownEdgeBoundsPolicy(const rubik::SolveOptions& options)
{
    if (largeLocalOptimalProfile(options)) {
        return "profile_enabled";
    }
    return environmentFlagEnabled("RUBIK_EXPERIMENTAL_CORNER_DOWN_EDGE_BOUNDS") ? "enabled" : "disabled";
}

bool cornerUpEdgeBoundsEnabled(const rubik::SolveOptions& options)
{
    return cornerUpEdgeBoundsPolicy(options) != "disabled";
}

bool cornerDownEdgeBoundsEnabled(const rubik::SolveOptions& options)
{
    return cornerDownEdgeBoundsPolicy(options) != "disabled";
}

void printBenchmarkPolicyRows(const rubik::SolveOptions& options, bool lowerBoundOnly)
{
    const ThreePhase1Policy policy = lowerBoundOnly
        ? threePhase1LowerBoundPolicy(options.profile)
        : threePhase1SolvePolicy(options);

    std::cout << "benchmark,mode," << modeName(options.mode) << "\n";
    std::cout << "benchmark,profile," << profileName(options.profile) << "\n";
    std::cout << "benchmark,threads," << options.threads << "\n";
    std::cout << "benchmark,three_phase1_policy," << policy.name << "\n";
    std::cout << "benchmark,three_phase1_effective," << (policy.effective ? "true" : "false") << "\n";
    std::cout << "benchmark,optimal_move_ordering,"
              << (environmentFlagEnabled("RUBIK_EXPERIMENTAL_STRONG_OPTIMAL_ORDERING")
                      ? "strong_bound"
                      : environmentFlagEnabled("RUBIK_EXPERIMENTAL_PHASE2_OPTIMAL_ORDERING")
                      ? "phase2_tiebreak"
                      : "base_bound")
              << "\n";
    std::cout << "benchmark,corner_state_bounds,"
              << cornerStateBoundsPolicy() << "\n";
    std::cout << "benchmark,corner_up_edge_bounds,"
              << cornerUpEdgeBoundsPolicy(options)
              << "\n";
    std::cout << "benchmark,corner_down_edge_bounds,"
              << cornerDownEdgeBoundsPolicy(options)
              << "\n";
    const char* goalTableDepth = std::getenv("RUBIK_EXPERIMENTAL_OPTIMAL_GOAL_TABLE_DEPTH");
    std::cout << "benchmark,optimal_goal_table_depth,"
              << (goalTableDepth != nullptr && goalTableDepth[0] != '\0' ? goalTableDepth : "0")
              << "\n";
}

void printUsage(const char* program)
{
    std::cerr
        << "Usage: " << program
        << " [--mode optimal|fast] [--timeout-ms N] [--max-depth N]"
        << " [--max-memory-mb N]"
        << " [--threads N]"
        << " [--max-case-depth N] [--profile default|embedded|performance|large-local]"
        << " [--case-set deterministic|random|both] [--random-count N]"
        << " [--random-depth N] [--random-seed N] [--random-start-index N]"
        << " [--slowest-count N] [--diagnose-fast] [--diagnose-optimal]"
        << " [--report-symmetry] [--report-cache] [--report-memory] [--report-policy]"
        << " [--benchmark-lower-bound] [--lower-bound-iterations N]\n";
}

std::string formatNodesByDepth(const std::vector<std::uint64_t>& values)
{
    std::ostringstream output;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) {
            output << '|';
        }
        output << values[i];
    }
    return output.str();
}

std::int64_t nearestRankPercentile(std::vector<std::int64_t> values, int percentile)
{
    if (values.empty()) {
        return 0;
    }

    std::sort(values.begin(), values.end());
    const auto count = static_cast<std::int64_t>(values.size());
    const auto rank = std::clamp<std::int64_t>((count * percentile + 99) / 100, 1, count);
    return values[static_cast<std::size_t>(rank - 1)];
}

bool isSolvedStatus(rubik::SolveStatus status)
{
    return status == rubik::SolveStatus::Optimal ||
        status == rubik::SolveStatus::Found ||
        status == rubik::SolveStatus::Solved;
}

int phase2CandidateLowerBound(const rubik::Cube& cube)
{
    const rubik::CubieParseResult parsed = rubik::CubieCube::fromCube(cube);
    if (!parsed) {
        return 0;
    }

    const std::uint32_t cornerPermutation = rubik::coordinates::cornerPermutation(parsed.cube);
    const std::uint32_t upEdgePermutation = rubik::coordinates::upEdgePermutation(parsed.cube);
    const std::uint32_t downEdgePermutation = rubik::coordinates::downEdgePermutation(parsed.cube);
    const std::uint32_t sliceEdgePermutation = rubik::coordinates::sliceEdgePermutation(parsed.cube);

    const auto cornerSliceIndex =
        cornerPermutation * rubik::coordinates::slice_edge_permutation_count + sliceEdgePermutation;
    const auto upEdgeSliceIndex =
        upEdgePermutation * rubik::coordinates::slice_edge_permutation_count + sliceEdgePermutation;
    const auto downEdgeSliceIndex =
        downEdgePermutation * rubik::coordinates::slice_edge_permutation_count + sliceEdgePermutation;

    return std::max({
        static_cast<int>(rubik::pruning_tables::cornerPermutation()[cornerPermutation]),
        static_cast<int>(rubik::pruning_tables::upEdgePermutation()[upEdgePermutation]),
        static_cast<int>(rubik::pruning_tables::downEdgePermutation()[downEdgePermutation]),
        static_cast<int>(rubik::pruning_tables::phase2CornerSlicePermutation()[cornerSliceIndex]),
        static_cast<int>(rubik::pruning_tables::phase2UpEdgeSlicePermutation()[upEdgeSliceIndex]),
        static_cast<int>(rubik::pruning_tables::phase2DownEdgeSlicePermutation()[downEdgeSliceIndex]),
    });
}

struct DiagnosticPhase1Candidate {
    std::vector<rubik::Move> moves;
    int phase2LowerBound = 0;
};

void printCaseRow(const BenchmarkRow& row, const rubik::SolveOptions& options)
{
    std::cout
        << row.item.name << ','
        << row.item.depth << ','
        << '"' << row.item.scramble << '"' << ','
        << statusName(row.status) << ','
        << (row.isOptimal ? "true" : "false") << ','
        << row.moveCount << ','
        << row.initialLowerBound << ','
        << row.elapsedMs << ','
        << row.nodesExpanded << ','
        << std::fixed << std::setprecision(2) << row.nodesPerMs << ','
        << options.maxDepth << ','
        << options.timeout.count() << ','
        << '"' << row.nodesByDepth << '"' << ','
        << '"' << row.solution << '"'
        << "\n";
}

void printFailureRow(const BenchmarkRow& row)
{
    std::cout
        << "failure,"
        << row.item.name << ','
        << row.item.depth << ','
        << '"' << row.item.scramble << '"' << ','
        << statusName(row.status) << ','
        << row.moveCount << ','
        << row.initialLowerBound << ','
        << row.elapsedMs << ','
        << row.nodesExpanded
        << "\n";
}

void printSlowestRow(const BenchmarkRow& row, int rank)
{
    std::cout
        << "slowest,"
        << rank << ','
        << row.item.name << ','
        << row.item.depth << ','
        << '"' << row.item.scramble << '"' << ','
        << statusName(row.status) << ','
        << row.moveCount << ','
        << row.elapsedMs << ','
        << row.nodesExpanded << ','
        << '"' << row.solution << '"'
        << "\n";
}

std::vector<FastAttemptOptions> fastAttemptOptions(const rubik::SolveOptions& options)
{
    const auto quickPhaseTimeout = [&]() {
        switch (options.profile) {
        case rubik::SolveProfile::Embedded:
            return std::chrono::milliseconds{500};
        case rubik::SolveProfile::Performance:
        case rubik::SolveProfile::LargeLocal:
            return std::chrono::milliseconds{5000};
        case rubik::SolveProfile::Default:
            return std::chrono::milliseconds{2000};
        }
        return std::chrono::milliseconds{2000};
    };

    const auto phase1CandidateLimit = [&]() -> std::size_t {
        switch (options.profile) {
        case rubik::SolveProfile::Embedded:
            return 4;
        case rubik::SolveProfile::Performance:
        case rubik::SolveProfile::LargeLocal:
            return 64;
        case rubik::SolveProfile::Default:
            return 16;
        }
        return 16;
    };

    const auto quickPhase1CandidateLimit = [&]() -> std::size_t {
        switch (options.profile) {
        case rubik::SolveProfile::Embedded:
            return 4;
        case rubik::SolveProfile::Performance:
        case rubik::SolveProfile::LargeLocal:
            return 8;
        case rubik::SolveProfile::Default:
            return 4;
        }
        return 4;
    };

    const auto phase2CandidateTimeout = [&]() {
        switch (options.profile) {
        case rubik::SolveProfile::Embedded:
            return std::chrono::milliseconds{100};
        case rubik::SolveProfile::Performance:
        case rubik::SolveProfile::LargeLocal:
            return std::chrono::milliseconds{750};
        case rubik::SolveProfile::Default:
            return std::chrono::milliseconds{150};
        }
        return std::chrono::milliseconds{150};
    };

    const auto quickPhase1Timeout = [&]() {
        return options.profile == rubik::SolveProfile::Performance || options.profile == rubik::SolveProfile::LargeLocal
            ? std::chrono::milliseconds{500}
            : std::chrono::milliseconds{250};
    };

    const auto quickPhase2Timeout = [&]() {
        return options.profile == rubik::SolveProfile::Performance || options.profile == rubik::SolveProfile::LargeLocal
            ? std::chrono::milliseconds{150}
            : std::chrono::milliseconds{75};
    };

    std::vector<FastAttemptOptions> attempts;
    if (options.profile != rubik::SolveProfile::Embedded) {
        attempts.push_back({
            .name = "quick",
            .phase1CandidateLimit = quickPhase1CandidateLimit(),
            .phase1Timeout = quickPhase1Timeout(),
            .phase2Timeout = quickPhase2Timeout(),
        });
    }

    attempts.push_back({
        .name = "robust",
        .phase1CandidateLimit = options.profile == rubik::SolveProfile::Embedded
            ? std::size_t{16}
            : phase1CandidateLimit(),
        .phase2CandidateLimit = options.profile == rubik::SolveProfile::Embedded
            ? std::size_t{4}
            : std::size_t{0},
        .phase1Timeout = quickPhaseTimeout(),
        .phase2Timeout = phase2CandidateTimeout(),
    });
    if (options.profile == rubik::SolveProfile::Embedded) {
        attempts.push_back({
            .name = "tail",
            .phase1CandidateLimit = 16,
            .phase2CandidateLimit = 0,
            .phase1Timeout = std::chrono::milliseconds{500},
            .phase2Timeout = std::chrono::milliseconds{150},
        });
    }

    return attempts;
}

void printFastDiagnostics(
    const Case& item,
    const rubik::Cube& cube,
    const rubik::SolveOptions& options)
{
    if (options.mode != rubik::SolveMode::Fast) {
        return;
    }

    const std::vector<FastAttemptOptions> attempts = fastAttemptOptions(options);
    for (const FastAttemptOptions& attempt : attempts) {
        const rubik::Phase1CandidatesResult phase1 = rubik::experimental::findPhase1Candidates(cube, {
            .maxDepth = std::min(options.maxDepth, 12),
            .timeout = attempt.phase1Timeout,
            .profile = options.profile,
            .maxCandidates = attempt.phase1CandidateLimit,
        });

        std::cout
            << "diagnostic_phase1,"
            << item.name << ','
            << attempt.name << ','
            << attempt.phase1CandidateLimit << ','
            << attempt.phase1Timeout.count() << ','
            << attempt.phase2Timeout.count() << ','
            << statusName(phase1.status) << ','
            << phase1.candidates.size() << ','
            << phase1.elapsed.count() << ','
            << phase1.nodesExpanded << ','
            << '"' << formatNodesByDepth(phase1.nodesByDepth) << '"'
            << "\n";

        std::vector<DiagnosticPhase1Candidate> candidates;
        candidates.reserve(phase1.candidates.size());
        for (const std::vector<rubik::Move>& phase1Moves : phase1.candidates) {
            rubik::Cube phase2Cube = cube;
            phase2Cube.apply(phase1Moves);
            candidates.push_back({
                .moves = phase1Moves,
                .phase2LowerBound = phase2CandidateLowerBound(phase2Cube),
            });
        }
        std::sort(candidates.begin(), candidates.end(), [](const DiagnosticPhase1Candidate& lhs, const DiagnosticPhase1Candidate& rhs) {
            const int lhsTotalLowerBound = static_cast<int>(lhs.moves.size()) + lhs.phase2LowerBound;
            const int rhsTotalLowerBound = static_cast<int>(rhs.moves.size()) + rhs.phase2LowerBound;
            if (lhsTotalLowerBound != rhsTotalLowerBound) {
                return lhsTotalLowerBound < rhsTotalLowerBound;
            }
            if (lhs.phase2LowerBound != rhs.phase2LowerBound) {
                return lhs.phase2LowerBound < rhs.phase2LowerBound;
            }
            return lhs.moves < rhs.moves;
        });

        const std::size_t phase2CandidateLimit = attempt.phase2CandidateLimit == 0
            ? candidates.size()
            : std::min(attempt.phase2CandidateLimit, candidates.size());
        for (std::size_t i = 0; i < phase2CandidateLimit; ++i) {
            const std::vector<rubik::Move>& phase1Moves = candidates[i].moves;
            rubik::Cube phase2Cube = cube;
            phase2Cube.apply(phase1Moves);

            const rubik::Phase2Result phase2 = rubik::experimental::solvePhase2(phase2Cube, {
                .maxDepth = options.maxDepth - static_cast<int>(phase1Moves.size()),
                .timeout = attempt.phase2Timeout,
                .profile = options.profile,
            });

            std::vector<rubik::Move> totalMoves = phase1Moves;
            if (isSolvedStatus(phase2.status)) {
                totalMoves.insert(totalMoves.end(), phase2.moves.begin(), phase2.moves.end());
            }

            std::cout
                << "diagnostic_phase2,"
                << item.name << ','
                << attempt.name << ','
                << (i + 1) << ','
                << phase1Moves.size() << ','
                << '"' << rubik::formatMoves(phase1Moves) << '"' << ','
                << statusName(phase2.status) << ','
                << (isSolvedStatus(phase2.status) ? static_cast<int>(totalMoves.size()) : -1) << ','
                << phase2.elapsed.count() << ','
                << phase2.nodesExpanded << ','
                << '"' << formatNodesByDepth(phase2.nodesByDepth) << '"' << ','
                << '"' << (isSolvedStatus(phase2.status) ? rubik::formatMoves(totalMoves) : std::string{}) << '"'
                << "\n";
        }
    }
}

void printOptimalDiagnostics(const Case& item, const rubik::SolveResult& result)
{
    const rubik::SolveBoundDiagnostics& diagnostics = result.boundDiagnostics;
    std::cout
        << "diagnostic_optimal_bounds,"
        << item.name << ','
        << diagnostics.cheapNodePrunes << ','
        << diagnostics.threePhaseNodeChecks << ','
        << diagnostics.threePhaseNodePrunes << ','
        << diagnostics.cheapCandidatePrunes << ','
        << diagnostics.threePhaseCandidateChecks << ','
        << diagnostics.threePhaseCandidatePrunes
        << "\n";
}

template <typename Container>
std::size_t bytesFor(const Container& container)
{
    return container.size() * sizeof(typename Container::value_type);
}

void printReductionRow(
    const std::string& name,
    std::uint32_t stateCount,
    const rubik::detail::CoordinateSymmetryTable& table,
    const rubik::detail::CoordinateSymmetryReduction* reduction)
{
    std::cout
        << "symmetry_coordinate,"
        << name << ','
        << stateCount << ','
        << table.size() << ','
        << bytesFor(table) << ',';

    if (reduction != nullptr) {
        const std::size_t reductionBytes =
            bytesFor(reduction->canonicalState) +
            bytesFor(reduction->canonicalSymmetry) +
            bytesFor(reduction->orbitIndex);
        const double compression = reduction->orbitCount > 0
            ? static_cast<double>(stateCount) / static_cast<double>(reduction->orbitCount)
            : 0.0;

        std::cout
            << reduction->orbitCount << ','
            << std::fixed << std::setprecision(2) << compression << ','
            << reductionBytes;
    } else {
        std::cout << ",,";
    }

    std::cout << "\n";
}

std::uint8_t maxPruningDepth(const rubik::pruning_tables::PruningTable& table)
{
    std::uint8_t maximum = 0;
    for (std::uint8_t value : table) {
        if (value != 0xff) {
            maximum = std::max(maximum, value);
        }
    }
    return maximum;
}

void printReducedPruningRow(
    const std::string& name,
    const rubik::pruning_tables::PruningTable& reduced,
    const rubik::pruning_tables::PruningTable& baseline)
{
    std::cout
        << "symmetry_pruning,"
        << name << ','
        << reduced.size() << ','
        << bytesFor(reduced) << ','
        << static_cast<int>(maxPruningDepth(reduced)) << ','
        << baseline.size() << ','
        << bytesFor(baseline) << ','
        << static_cast<int>(maxPruningDepth(baseline))
        << "\n";
}

void printCombinedReductionRow(
    const std::string& name,
    std::uint32_t stateCount,
    const rubik::detail::CoordinateSymmetryReduction& reduction)
{
    const std::size_t reductionBytes =
        bytesFor(reduction.canonicalState) +
        bytesFor(reduction.canonicalSymmetry) +
        bytesFor(reduction.orbitIndex) +
        bytesFor(reduction.orbitRepresentative);
    const double compression = reduction.orbitCount > 0
        ? static_cast<double>(stateCount) / static_cast<double>(reduction.orbitCount)
        : 0.0;

    std::cout
        << "symmetry_combined_coordinate,"
        << name << ','
        << stateCount << ','
        << reduction.orbitCount << ','
        << std::fixed << std::setprecision(2) << compression << ','
        << reductionBytes
        << "\n";
}

void printCombinedReducedPruningRow(
    const std::string& name,
    const rubik::pruning_tables::PruningTable& reduced,
    std::uint32_t fullStateCount)
{
    std::cout
        << "symmetry_combined_pruning,"
        << name << ','
        << reduced.size() << ','
        << bytesFor(reduced) << ','
        << static_cast<int>(maxPruningDepth(reduced)) << ','
        << fullStateCount << ','
        << fullStateCount
        << "\n";
}

void printSymmetryReport()
{
    int udSlicePreserving = 0;
    for (rubik::detail::SymmetryId symmetry = 0; symmetry < rubik::detail::cube_rotation_symmetry_count; ++symmetry) {
        if (rubik::detail::preservesUdSlice(symmetry)) {
            ++udSlicePreserving;
        }
    }

    std::cout << "symmetry,rotation_count," << rubik::detail::cube_rotation_symmetry_count << "\n";
    std::cout << "symmetry,ud_slice_preserving_count," << udSlicePreserving << "\n";
    std::cout << "symmetry_coordinate,name,states,table_rows,table_bytes,orbit_count,compression_ratio,reduction_bytes\n";

    printReductionRow(
        "corner_orientation",
        rubik::coordinates::corner_orientation_count,
        rubik::detail::cornerOrientationSymmetries(),
        &rubik::detail::cornerOrientationSymmetryReduction());
    printReductionRow(
        "edge_orientation",
        rubik::coordinates::edge_orientation_count,
        rubik::detail::edgeOrientationSymmetries(),
        &rubik::detail::edgeOrientationSymmetryReduction());
    printReductionRow(
        "slice_edges",
        rubik::coordinates::slice_edge_count,
        rubik::detail::sliceEdgeSymmetries(),
        nullptr);

    const std::uint32_t cornerEdgeOrientationStates =
        rubik::coordinates::corner_orientation_count * rubik::coordinates::edge_orientation_count;
    const auto& cornerEdgeReduction = rubik::detail::cornerEdgeOrientationSymmetryReduction();
    const std::uint32_t edgeOrientationSliceStates =
        rubik::coordinates::edge_orientation_count * rubik::coordinates::slice_edge_count;
    const std::uint32_t cornerOrientationSliceStates =
        rubik::coordinates::corner_orientation_count * rubik::coordinates::slice_edge_count;
    const auto& edgeSliceReduction = rubik::detail::edgeOrientationSliceEdgeSymmetryReduction();
    const auto& cornerSliceReduction = rubik::detail::cornerOrientationSliceEdgeSymmetryReduction();
    std::cout << "symmetry_combined_coordinate,name,states,orbit_count,compression_ratio,reduction_bytes\n";
    printCombinedReductionRow(
        "corner_edge_orientation",
        cornerEdgeOrientationStates,
        cornerEdgeReduction);
    printCombinedReductionRow(
        "corner_orientation_slice_edges",
        cornerOrientationSliceStates,
        cornerSliceReduction);
    printCombinedReductionRow(
        "edge_orientation_slice_edges",
        edgeOrientationSliceStates,
        edgeSliceReduction);

    std::cout << "symmetry_pruning,name,reduced_entries,reduced_bytes,reduced_max_depth,baseline_entries,baseline_bytes,baseline_max_depth\n";
    printReducedPruningRow(
        "corner_orientation",
        rubik::detail::reducedCornerOrientationPruning(),
        rubik::pruning_tables::cornerOrientation());
    printReducedPruningRow(
        "edge_orientation",
        rubik::detail::reducedEdgeOrientationPruning(),
        rubik::pruning_tables::edgeOrientation());

    std::cout << "symmetry_combined_pruning,name,reduced_entries,reduced_bytes,reduced_max_depth,full_entries,full_bytes_if_materialized\n";
    printCombinedReducedPruningRow(
        "corner_edge_orientation",
        rubik::detail::reducedCornerEdgeOrientationPruning(),
        cornerEdgeOrientationStates);
    printCombinedReducedPruningRow(
        "corner_orientation_slice_edges",
        rubik::detail::reducedCornerOrientationSliceEdgePruning(),
        cornerOrientationSliceStates);
    printCombinedReducedPruningRow(
        "edge_orientation_slice_edges",
        rubik::detail::reducedEdgeOrientationSliceEdgePruning(),
        edgeOrientationSliceStates);
}

void printCacheReport()
{
    const std::filesystem::path cacheDirectory = rubik::pruning_tables::cacheDirectory();
    std::uint64_t fileCount = 0;
    std::uint64_t totalBytes = 0;

    std::cout << "cache_report,directory," << cacheDirectory.string() << "\n";
    std::cout << "cache_file,name,bytes\n";

    std::error_code error;
    if (!std::filesystem::exists(cacheDirectory, error)) {
        std::cout << "cache_summary,files,0\n";
        std::cout << "cache_summary,total_bytes,0\n";
        return;
    }

    std::vector<std::filesystem::directory_entry> entries;
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(cacheDirectory, error)) {
        if (error) {
            break;
        }
        if (!entry.is_regular_file(error) || entry.path().extension() != ".rpt") {
            continue;
        }
        entries.push_back(entry);
    }

    std::sort(entries.begin(), entries.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.path().filename().string() < rhs.path().filename().string();
    });

    for (const std::filesystem::directory_entry& entry : entries) {
        const std::uint64_t bytes = entry.file_size(error);
        if (error) {
            error.clear();
            continue;
        }

        ++fileCount;
        totalBytes += bytes;
        std::cout
            << "cache_file,"
            << entry.path().filename().string() << ','
            << bytes
            << "\n";
    }

    std::cout << "cache_summary,files," << fileCount << "\n";
    std::cout << "cache_summary,total_bytes," << totalBytes << "\n";
}

std::uint64_t printMemoryTableRow(const std::string& profile, const rubik::detail::TableProfileEntry& entry)
{
    const std::uint64_t bytes = entry.entries;
    std::cout
        << "memory_table,"
        << profile << ','
        << entry.name << ','
        << entry.entries << ','
        << bytes
        << "\n";
    return bytes;
}

void printMemoryProfileRows(
    const std::string& profile,
    std::span<const rubik::detail::TableProfileEntry> entries,
    bool includeCornerState = false,
    bool includeCornerUpEdge = false,
    bool includeCornerDownEdge = false)
{
    std::uint64_t totalBytes = 0;
    std::uint64_t tableCount = 0;

    for (const rubik::detail::TableProfileEntry& entry : entries) {
        totalBytes += printMemoryTableRow(profile, entry);
        ++tableCount;
    }
    if (includeCornerState) {
        const auto entriesCount = static_cast<std::uint64_t>(rubik::coordinates::corner_orientation_count) *
            static_cast<std::uint64_t>(rubik::coordinates::corner_permutation_count);
        std::cout
            << "memory_table,"
            << profile
            << ",corner_orientation_permutation,"
            << entriesCount << ','
            << entriesCount
            << "\n";
        totalBytes += entriesCount;
        ++tableCount;
    }
    if (includeCornerUpEdge) {
        const auto entriesCount = static_cast<std::uint64_t>(rubik::coordinates::corner_permutation_count) *
            static_cast<std::uint64_t>(rubik::coordinates::edge_group_permutation_count);
        std::cout
            << "memory_table,"
            << profile
            << ",corner_permutation_up_edge_permutation,"
            << entriesCount << ','
            << entriesCount
            << "\n";
        totalBytes += entriesCount;
        ++tableCount;
    }
    if (includeCornerDownEdge) {
        const auto entriesCount = static_cast<std::uint64_t>(rubik::coordinates::corner_permutation_count) *
            static_cast<std::uint64_t>(rubik::coordinates::edge_group_permutation_count);
        std::cout
            << "memory_table,"
            << profile
            << ",corner_permutation_down_edge_permutation,"
            << entriesCount << ','
            << entriesCount
            << "\n";
        totalBytes += entriesCount;
        ++tableCount;
    }

    std::cout << "memory_summary," << profile << ",tables," << tableCount << "\n";
    std::cout << "memory_summary," << profile << ",total_bytes," << totalBytes << "\n";
}

void printMemoryReport()
{
    using namespace rubik::pruning_tables;

    std::cout << "memory_report,cache_dir," << cacheDirectory() << "\n";
    std::cout << "memory_table,profile,name,entries,bytes\n";
    const bool includeCornerState = cornerStateBoundsEnabled();
    printMemoryProfileRows("embedded_optimal", rubik::detail::optimalTableProfile(rubik::SolveProfile::Embedded), includeCornerState);
    printMemoryProfileRows("default_optimal", rubik::detail::optimalTableProfile(rubik::SolveProfile::Default), includeCornerState);
    printMemoryProfileRows("performance_optimal", rubik::detail::optimalTableProfile(rubik::SolveProfile::Performance), includeCornerState);
    printMemoryProfileRows(
        "large_local_optimal",
        rubik::detail::optimalTableProfile(rubik::SolveProfile::LargeLocal),
        includeCornerState,
        true,
        true);

    std::vector<rubik::detail::TableProfileEntry> fastTwoPhase;
    const auto phase1 = rubik::detail::phase1BaseTableProfile();
    const auto phase2 = rubik::detail::phase2TableProfile();
    fastTwoPhase.insert(fastTwoPhase.end(), phase1.begin(), phase1.end());
    fastTwoPhase.insert(fastTwoPhase.end(), phase2.begin(), phase2.end());
    printMemoryProfileRows("fast_two_phase", fastTwoPhase);
}

std::chrono::milliseconds warmUpTables(const rubik::SolveOptions& options)
{
    const auto startedAt = std::chrono::steady_clock::now();

    for (const rubik::detail::TableProfileEntry& entry : rubik::detail::optimalTableProfile(options.profile)) {
        (void)entry.table();
    }
    if (cornerStateBoundsEnabled()) {
        (void)rubik::pruning_tables::cornerOrientationPermutation();
    }
    if (cornerUpEdgeBoundsEnabled(options)) {
        (void)rubik::pruning_tables::cornerPermutationUpEdgePermutation();
    }
    if (cornerDownEdgeBoundsEnabled(options)) {
        (void)rubik::pruning_tables::cornerPermutationDownEdgePermutation();
    }
    if (options.mode == rubik::SolveMode::Optimal &&
        environmentFlagEnabled("RUBIK_EXPERIMENTAL_PHASE2_OPTIMAL_ORDERING")) {
        for (const rubik::detail::TableProfileEntry& entry : rubik::detail::phase2TableProfile()) {
            (void)entry.table();
        }
    }
    if (options.mode == rubik::SolveMode::Fast) {
        for (const rubik::detail::TableProfileEntry& entry : rubik::detail::phase2TableProfile()) {
            (void)entry.table();
        }
    }

    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startedAt);
}

std::size_t warmUpTablePayloadBytes(const rubik::SolveOptions& options)
{
    std::size_t bytes = rubik::detail::estimatedSolverTablePayloadBytes(options.mode, options.profile);
    if (cornerStateBoundsEnabled()) {
        bytes += static_cast<std::size_t>(rubik::coordinates::corner_orientation_count) *
            static_cast<std::size_t>(rubik::coordinates::corner_permutation_count);
    }
    if (cornerUpEdgeBoundsEnabled(options)) {
        bytes += static_cast<std::size_t>(rubik::coordinates::corner_permutation_count) *
            static_cast<std::size_t>(rubik::coordinates::edge_group_permutation_count);
    }
    if (cornerDownEdgeBoundsEnabled(options)) {
        bytes += static_cast<std::size_t>(rubik::coordinates::corner_permutation_count) *
            static_cast<std::size_t>(rubik::coordinates::edge_group_permutation_count);
    }
    if (options.mode == rubik::SolveMode::Optimal &&
        environmentFlagEnabled("RUBIK_EXPERIMENTAL_PHASE2_OPTIMAL_ORDERING")) {
        bytes += rubik::detail::tableProfilePayloadBytes(rubik::detail::phase2TableProfile());
    }
    return bytes;
}

void printLowerBoundBenchmark(
    const rubik::SolveOptions& options,
    const std::vector<Case>& cases,
    int iterations)
{
    rubik::Solver solver;
    std::int64_t totalElapsedUs = 0;
    std::uint64_t totalEvaluations = 0;
    std::int64_t checksum = 0;

    printBenchmarkPolicyRows(options, true);
    const std::chrono::milliseconds warmupElapsed = warmUpTables(options);
    std::cout << "benchmark,warmup_table_payload_bytes," << warmUpTablePayloadBytes(options) << "\n";
    std::cout << "benchmark,warmup_elapsed_ms," << warmupElapsed.count() << "\n";
    std::cout << "lower_bound_benchmark,iterations_per_case," << iterations << "\n";
    std::cout << "lower_bound_case,name,case_depth,lower_bound,elapsed_us,evaluations_per_ms,checksum\n";

    for (const Case& item : cases) {
        rubik::Cube cube = rubik::Cube::solved();
        cube.apply(rubik::parseMoves(item.scramble));

        const int lowerBound = solver.lowerBound(cube, options.metric, options.profile);
        const auto startedAt = std::chrono::steady_clock::now();
        std::int64_t localChecksum = 0;
        for (int i = 0; i < iterations; ++i) {
            localChecksum += solver.lowerBound(cube, options.metric, options.profile);
        }
        const auto elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - startedAt).count();
        const double evaluationsPerMs = elapsedUs > 0
            ? static_cast<double>(iterations) / (static_cast<double>(elapsedUs) / 1000.0)
            : static_cast<double>(iterations);

        totalElapsedUs += elapsedUs;
        totalEvaluations += static_cast<std::uint64_t>(iterations);
        checksum += localChecksum;

        std::cout
            << "lower_bound_case,"
            << item.name << ','
            << item.depth << ','
            << lowerBound << ','
            << elapsedUs << ','
            << std::fixed << std::setprecision(2) << evaluationsPerMs << ','
            << localChecksum
            << "\n";
    }

    const double totalEvaluationsPerMs = totalElapsedUs > 0
        ? static_cast<double>(totalEvaluations) / (static_cast<double>(totalElapsedUs) / 1000.0)
        : static_cast<double>(totalEvaluations);

    std::cout << "lower_bound_summary,total_cases," << cases.size() << "\n";
    std::cout << "lower_bound_summary,total_evaluations," << totalEvaluations << "\n";
    std::cout << "lower_bound_summary,total_elapsed_us," << totalElapsedUs << "\n";
    std::cout << "lower_bound_summary,evaluations_per_ms," << std::fixed << std::setprecision(2) << totalEvaluationsPerMs << "\n";
    std::cout << "lower_bound_summary,checksum," << checksum << "\n";
}

} // namespace

int main(int argc, char** argv)
{
    rubik::SolveOptions options{
        .mode = rubik::SolveMode::Optimal,
        .metric = rubik::Metric::HTM,
        .maxDepth = 20,
        .timeout = std::chrono::seconds(30),
        .maxMemoryBytes = 1024ull * 1024 * 1024,
        .threads = 1,
    };
    int maxCaseDepth = 12;
    CaseSet caseSet = CaseSet::Deterministic;
    int randomCount = 100;
    int randomDepth = 20;
    std::uint64_t randomSeed = 0x5eed;
    int randomStartIndex = 1;
    int slowestCount = 5;
    int lowerBoundIterations = 10000;
    bool diagnoseFast = false;
    bool diagnoseOptimal = false;
    bool reportSymmetry = false;
    bool reportCache = false;
    bool reportMemory = false;
    bool reportPolicy = false;
    bool benchmarkLowerBound = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--timeout-ms" && i + 1 < argc) {
            options.timeout = std::chrono::milliseconds(std::strtoll(argv[++i], nullptr, 10));
        } else if (arg == "--max-depth" && i + 1 < argc) {
            options.maxDepth = static_cast<int>(std::strtol(argv[++i], nullptr, 10));
        } else if (arg == "--max-memory-mb" && i + 1 < argc) {
            options.maxMemoryBytes = static_cast<std::size_t>(std::strtoull(argv[++i], nullptr, 10)) *
                1024ull * 1024ull;
        } else if (arg == "--threads" && i + 1 < argc) {
            options.threads = static_cast<unsigned int>(std::strtoul(argv[++i], nullptr, 10));
            if (options.threads == 0) {
                options.threads = 1;
            }
        } else if (arg == "--max-case-depth" && i + 1 < argc) {
            maxCaseDepth = static_cast<int>(std::strtol(argv[++i], nullptr, 10));
        } else if (arg == "--profile" && i + 1 < argc) {
            const std::string value = argv[++i];
            const auto parsed = parseProfile(value);
            if (!parsed) {
                std::cerr << "Invalid profile: " << value << "\n";
                printUsage(argv[0]);
                return 2;
            }
            options.profile = *parsed;
        } else if (arg == "--mode" && i + 1 < argc) {
            const std::string value = argv[++i];
            const auto parsed = parseMode(value);
            if (!parsed) {
                std::cerr << "Invalid mode: " << value << "\n";
                printUsage(argv[0]);
                return 2;
            }
            options.mode = *parsed;
        } else if (arg == "--case-set" && i + 1 < argc) {
            const std::string value = argv[++i];
            const auto parsed = parseCaseSet(value);
            if (!parsed) {
                std::cerr << "Invalid case set: " << value << "\n";
                printUsage(argv[0]);
                return 2;
            }
            caseSet = *parsed;
        } else if (arg == "--random-count" && i + 1 < argc) {
            randomCount = static_cast<int>(std::strtol(argv[++i], nullptr, 10));
        } else if (arg == "--random-depth" && i + 1 < argc) {
            randomDepth = static_cast<int>(std::strtol(argv[++i], nullptr, 10));
        } else if (arg == "--random-seed" && i + 1 < argc) {
            randomSeed = static_cast<std::uint64_t>(std::strtoull(argv[++i], nullptr, 10));
        } else if (arg == "--random-start-index" && i + 1 < argc) {
            randomStartIndex = static_cast<int>(std::strtol(argv[++i], nullptr, 10));
        } else if (arg == "--slowest-count" && i + 1 < argc) {
            slowestCount = static_cast<int>(std::strtol(argv[++i], nullptr, 10));
        } else if (arg == "--lower-bound-iterations" && i + 1 < argc) {
            lowerBoundIterations = static_cast<int>(std::strtol(argv[++i], nullptr, 10));
        } else if (arg == "--diagnose-fast") {
            diagnoseFast = true;
        } else if (arg == "--diagnose-optimal") {
            diagnoseOptimal = true;
        } else if (arg == "--report-symmetry") {
            reportSymmetry = true;
        } else if (arg == "--report-cache") {
            reportCache = true;
        } else if (arg == "--report-memory") {
            reportMemory = true;
        } else if (arg == "--report-policy") {
            reportPolicy = true;
        } else if (arg == "--benchmark-lower-bound") {
            benchmarkLowerBound = true;
        } else {
            printUsage(argv[0]);
            return 2;
        }
    }

    if (randomCount < 0 || randomDepth < 0 || randomStartIndex < 1 || slowestCount < 0 || lowerBoundIterations < 1) {
        printUsage(argv[0]);
        return 2;
    }

    if (reportSymmetry) {
        printSymmetryReport();
        return 0;
    }
    if (reportCache) {
        printCacheReport();
        return 0;
    }
    if (reportMemory) {
        printMemoryReport();
        return 0;
    }
    if (reportPolicy) {
        printBenchmarkPolicyRows(options, benchmarkLowerBound);
        return 0;
    }

    const std::vector<Case> cases = selectedCases(
        caseSet,
        maxCaseDepth,
        randomCount,
        randomDepth,
        randomSeed,
        randomStartIndex);

    if (benchmarkLowerBound) {
        printLowerBoundBenchmark(options, cases, lowerBoundIterations);
        return 0;
    }

    options.collectDiagnostics = diagnoseOptimal;

    rubik::Solver solver;
    std::uint64_t totalNodes = 0;
    std::int64_t totalElapsedMs = 0;
    int solved = 0;
    std::int64_t maxElapsedMs = 0;
    std::vector<BenchmarkRow> rows;

    std::cout << "cache_dir," << rubik::pruning_tables::cacheDirectory() << "\n";
    printBenchmarkPolicyRows(options, false);
    const std::chrono::milliseconds warmupElapsed = warmUpTables(options);
    std::cout << "benchmark,warmup_table_payload_bytes," << warmUpTablePayloadBytes(options) << "\n";
    std::cout << "benchmark,warmup_elapsed_ms," << warmupElapsed.count() << "\n";
    std::cout << "case,case_depth,scramble,status,optimal,moves,initial_lower_bound,elapsed_ms,nodes_expanded,nodes_per_ms,max_depth,timeout_ms,nodes_by_depth,solution\n";
    std::cout.flush();

    for (const Case& item : cases) {
        rubik::Cube cube = rubik::Cube::solved();
        cube.apply(rubik::parseMoves(item.scramble));

        if (diagnoseFast) {
            printFastDiagnostics(item, cube, options);
            std::cout.flush();
        }

        const int initialLowerBound = solver.lowerBound(cube, options.metric, options.profile);
        const rubik::SolveResult result = solver.solve(cube, options);
        if (diagnoseOptimal) {
            printOptimalDiagnostics(item, result);
            std::cout.flush();
        }
        const auto nodesPerMs = result.elapsed.count() > 0
            ? static_cast<double>(result.nodesExpanded) / static_cast<double>(result.elapsed.count())
            : static_cast<double>(result.nodesExpanded);
        BenchmarkRow row{
            .item = item,
            .status = result.status,
            .isOptimal = result.isOptimal,
            .moveCount = result.moveCount,
            .initialLowerBound = initialLowerBound,
            .elapsedMs = result.elapsed.count(),
            .nodesExpanded = result.nodesExpanded,
            .nodesPerMs = nodesPerMs,
            .nodesByDepth = formatNodesByDepth(result.nodesByDepth),
            .solution = rubik::formatMoves(result.moves),
        };

        totalNodes += result.nodesExpanded;
        totalElapsedMs += result.elapsed.count();
        maxElapsedMs = std::max(maxElapsedMs, result.elapsed.count());
        if (isSolvedStatus(result.status)) {
            ++solved;
        }

        printCaseRow(row, options);
        rows.push_back(std::move(row));
        std::cout.flush();
    }

    const int totalCases = static_cast<int>(cases.size());
    const int failed = totalCases - solved;
    const double averageElapsedMs = totalCases > 0
        ? static_cast<double>(totalElapsedMs) / static_cast<double>(totalCases)
        : 0.0;
    const double averageNodes = totalCases > 0
        ? static_cast<double>(totalNodes) / static_cast<double>(totalCases)
        : 0.0;
    std::vector<std::int64_t> elapsedValues;
    elapsedValues.reserve(rows.size());
    for (const BenchmarkRow& row : rows) {
        elapsedValues.push_back(row.elapsedMs);
    }
    const std::int64_t p50ElapsedMs = nearestRankPercentile(elapsedValues, 50);
    const std::int64_t p90ElapsedMs = nearestRankPercentile(elapsedValues, 90);
    const std::int64_t p95ElapsedMs = nearestRankPercentile(elapsedValues, 95);
    const std::int64_t p99ElapsedMs = nearestRankPercentile(elapsedValues, 99);

    std::cout << "summary,total_cases," << totalCases << "\n";
    std::cout << "summary,solved," << solved << "\n";
    std::cout << "summary,failed," << failed << "\n";
    std::cout << "summary,total_elapsed_ms," << totalElapsedMs << "\n";
    std::cout << "summary,total_nodes_expanded," << totalNodes << "\n";
    std::cout << "summary,average_elapsed_ms," << std::fixed << std::setprecision(2) << averageElapsedMs << "\n";
    std::cout << "summary,average_nodes_expanded," << std::fixed << std::setprecision(2) << averageNodes << "\n";
    std::cout << "summary,p50_elapsed_ms," << p50ElapsedMs << "\n";
    std::cout << "summary,p90_elapsed_ms," << p90ElapsedMs << "\n";
    std::cout << "summary,p95_elapsed_ms," << p95ElapsedMs << "\n";
    std::cout << "summary,p99_elapsed_ms," << p99ElapsedMs << "\n";
    std::cout << "summary,max_elapsed_ms," << maxElapsedMs << "\n";

    for (const BenchmarkRow& row : rows) {
        if (!isSolvedStatus(row.status)) {
            printFailureRow(row);
        }
    }

    std::vector<BenchmarkRow> slowest = rows;
    std::sort(slowest.begin(), slowest.end(), [](const BenchmarkRow& lhs, const BenchmarkRow& rhs) {
        if (lhs.elapsedMs != rhs.elapsedMs) {
            return lhs.elapsedMs > rhs.elapsedMs;
        }
        return lhs.nodesExpanded > rhs.nodesExpanded;
    });

    const int reportedSlowest = std::min<int>(slowestCount, static_cast<int>(slowest.size()));
    for (int i = 0; i < reportedSlowest; ++i) {
        printSlowestRow(slowest[static_cast<std::size_t>(i)], i + 1);
    }

    return solved == totalCases ? 0 : 1;
}
