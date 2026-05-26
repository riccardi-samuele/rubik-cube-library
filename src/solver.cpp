#include "rubik/solver.hpp"

#include "rubik/coordinates.hpp"
#include "rubik/cubie_cube.hpp"
#include "rubik/detail/optimal_plan.hpp"
#include "rubik/detail/symmetry_coordinates.hpp"
#include "rubik/detail/symmetry_pruning.hpp"
#include "rubik/detail/table_profiles.hpp"
#include "rubik/move_tables.hpp"
#include "rubik/phase1.hpp"
#include "rubik/phase2.hpp"
#include "rubik/pruning_tables.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <unordered_map>

namespace rubik {
namespace {

class Deadline {
public:
    explicit Deadline(std::chrono::milliseconds timeout)
        : enabled_(timeout.count() > 0),
          end_(std::chrono::steady_clock::now() + timeout)
    {
    }

    bool expired() const
    {
        return enabled_ && std::chrono::steady_clock::now() >= end_;
    }

private:
    bool enabled_;
    std::chrono::steady_clock::time_point end_;
};

struct Phase1Coordinates {
    std::uint32_t cornerOrientation = 0;
    std::uint32_t edgeOrientation = 0;
    std::uint32_t sliceEdges = 0;
};

struct SearchNode {
    CubieCube cube;
    std::uint32_t cornerOrientation = 0;
    std::uint32_t edgeOrientation = 0;
    std::uint32_t sliceEdges = 0;
    std::uint32_t cornerPermutation = 0;
    std::uint32_t upEdgePermutation = 0;
    std::uint32_t downEdgePermutation = 0;
    std::array<Phase1Coordinates, 2> extraPhase1Directions{};
};

struct CubieCubeKey {
    std::uint64_t low = 0;
    std::uint64_t high = 0;

    friend bool operator==(const CubieCubeKey& lhs, const CubieCubeKey& rhs) = default;
};

struct CubieCubeKeyHash {
    std::size_t operator()(const CubieCubeKey& key) const
    {
        std::uint64_t value = key.low;
        value ^= key.high + 0x9e3779b97f4a7c15ULL + (value << 6) + (value >> 2);
        value ^= value >> 33;
        value *= 0xff51afd7ed558ccdULL;
        value ^= value >> 33;
        value *= 0xc4ceb9fe1a85ec53ULL;
        value ^= value >> 33;
        return static_cast<std::size_t>(value);
    }
};

CubieCubeKey makeCubieCubeKey(const CubieCube& cube)
{
    CubieCubeKey key{};
    int bit = 0;
    const auto append = [&](std::uint64_t value, int bits) {
        if (bit < 64) {
            key.low |= value << bit;
            if (bit + bits > 64) {
                key.high |= value >> (64 - bit);
            }
        } else {
            key.high |= value << (bit - 64);
        }
        bit += bits;
    };

    for (std::uint8_t value : cube.cornerPermutation) {
        append(value, 3);
    }
    for (std::uint8_t value : cube.cornerOrientation) {
        append(value, 2);
    }
    for (std::uint8_t value : cube.edgePermutation) {
        append(value, 4);
    }
    for (std::uint8_t value : cube.edgeOrientation) {
        append(value, 1);
    }

    return key;
}

struct GoalTableEntry {
    CubieCube cube;
    CubieCubeKey key;
    std::uint32_t parent = std::numeric_limits<std::uint32_t>::max();
    Move moveToParent = Move::U;
    std::uint8_t depth = 0;
};

struct GoalTable {
    int radius = 0;
    std::vector<GoalTableEntry> entries;
    std::unordered_map<CubieCubeKey, std::uint32_t, CubieCubeKeyHash> indexByKey;
};

std::uint64_t goalTableEstimatedStates(int radius)
{
    if (radius <= 0) {
        return 1;
    }

    std::uint64_t states = 1;
    std::uint64_t width = 18;
    for (int depth = 1; depth <= radius; ++depth) {
        states += width;
        width *= 12;
    }
    return states;
}

int experimentalGoalTableDepth()
{
    const char* value = std::getenv("RUBIK_EXPERIMENTAL_OPTIMAL_GOAL_TABLE_DEPTH");
    if (value == nullptr || value[0] == '\0' || value[0] == '0') {
        return 0;
    }

    const int depth = std::atoi(value);
    return std::clamp(depth, 0, 7);
}

std::size_t experimentalGoalTablePayloadBytes(int radius)
{
    return static_cast<std::size_t>(goalTableEstimatedStates(radius)) *
        (sizeof(GoalTableEntry) + sizeof(std::pair<const CubieCubeKey, std::uint32_t>));
}

GoalTable buildGoalTable(int targetRadius)
{
    GoalTable table;
    table.radius = targetRadius;
    table.entries.reserve(static_cast<std::size_t>(goalTableEstimatedStates(targetRadius)));
    table.indexByKey.reserve(static_cast<std::size_t>(goalTableEstimatedStates(targetRadius)));

    const CubieCube solved = CubieCube::solved();
    const CubieCubeKey solvedKey = makeCubieCubeKey(solved);
    table.entries.push_back({
        .cube = solved,
        .key = solvedKey,
        .parent = std::numeric_limits<std::uint32_t>::max(),
        .moveToParent = Move::U,
        .depth = 0,
    });
    table.indexByKey.emplace(solvedKey, 0);

    std::size_t layerBegin = 0;
    std::size_t layerEnd = 1;
    for (int depth = 0; depth < targetRadius; ++depth) {
        const std::size_t nextLayerBegin = table.entries.size();
        for (std::size_t i = layerBegin; i < layerEnd; ++i) {
            const GoalTableEntry parent = table.entries[i];
            for (Move move : allMoves()) {
                CubieCube next = parent.cube.moved(move);
                CubieCubeKey key = makeCubieCubeKey(next);
                if (table.indexByKey.find(key) != table.indexByKey.end()) {
                    continue;
                }

                const auto index = static_cast<std::uint32_t>(table.entries.size());
                table.indexByKey.emplace(key, index);
                table.entries.push_back({
                    .cube = std::move(next),
                    .key = key,
                    .parent = static_cast<std::uint32_t>(i),
                    .moveToParent = inverse(move),
                    .depth = static_cast<std::uint8_t>(depth + 1),
                });
            }
        }
        layerBegin = nextLayerBegin;
        layerEnd = table.entries.size();
    }

    return table;
}

const GoalTable& goalTable(int radius)
{
    static std::array<GoalTable, 8> tables{};
    static std::array<bool, 8> built{};
    static std::mutex mutex;

    const auto index = static_cast<std::size_t>(radius);
    std::scoped_lock lock(mutex);
    if (!built[index]) {
        tables[index] = buildGoalTable(radius);
        built[index] = true;
    }

    return tables[index];
}

std::vector<Move> reconstructGoalTableSuffix(const GoalTable& table, std::uint32_t index)
{
    std::vector<Move> suffix;
    while (index != std::numeric_limits<std::uint32_t>::max()) {
        const GoalTableEntry& entry = table.entries[index];
        if (entry.parent == std::numeric_limits<std::uint32_t>::max()) {
            break;
        }
        suffix.push_back(entry.moveToParent);
        index = entry.parent;
    }
    return suffix;
}

struct Phase1BoundTables {
    const pruning_tables::PruningTable& cornerOrientation;
    const pruning_tables::PruningTable& edgeOrientation;
    const pruning_tables::PruningTable& sliceEdges;
    const pruning_tables::PruningTable& cornerOrientationSlice;
    const pruning_tables::PruningTable& edgeOrientationSlice;
};

const Phase1BoundTables& phase1BoundTables()
{
    static const Phase1BoundTables tables{
        .cornerOrientation = pruning_tables::cornerOrientation(),
        .edgeOrientation = pruning_tables::edgeOrientation(),
        .sliceEdges = pruning_tables::sliceEdges(),
        .cornerOrientationSlice = pruning_tables::cornerOrientationSlice(),
        .edgeOrientationSlice = pruning_tables::edgeOrientationSlice(),
    };
    return tables;
}

std::array<detail::SymmetryId, 3> phase1DirectionSymmetries()
{
    std::array<detail::SymmetryId, 3> result = {{
        detail::identitySymmetry(),
        detail::identitySymmetry(),
        detail::identitySymmetry(),
    }};

    const auto& symmetries = detail::cubeRotationSymmetries();
    for (detail::SymmetryId symmetry = 0; symmetry < symmetries.size(); ++symmetry) {
        if (symmetries[symmetry].matrix[1][0] != 0 && result[1] == detail::identitySymmetry()) {
            result[1] = symmetry;
        }
        if (symmetries[symmetry].matrix[1][2] != 0 && result[2] == detail::identitySymmetry()) {
            result[2] = symmetry;
        }
    }

    return result;
}

const std::array<std::array<Move, move_count>, 2>& extraPhase1DirectionMoveTable()
{
    static const auto table = [] {
        std::array<std::array<Move, move_count>, 2> result{};
        const auto symmetries = phase1DirectionSymmetries();

        for (std::size_t extraDirection = 0; extraDirection < result.size(); ++extraDirection) {
            const std::size_t direction = extraDirection + 1;
            for (Move move : allMoves()) {
                Cube transformed = Cube::solved();
                transformed.apply(move);
                transformed = detail::applySymmetry(transformed, symmetries[direction]);

                bool found = false;
                for (Move candidate : allMoves()) {
                    Cube candidateCube = Cube::solved();
                    candidateCube.apply(candidate);
                    if (candidateCube == transformed) {
                        result[extraDirection][static_cast<int>(move)] = candidate;
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    throw std::logic_error("cube symmetry did not map a face turn to a face turn");
                }
            }
        }

        return result;
    }();
    return table;
}

Phase1Coordinates phase1Coordinates(const CubieCube& cube)
{
    return {
        .cornerOrientation = coordinates::cornerOrientation(cube),
        .edgeOrientation = coordinates::edgeOrientation(cube),
        .sliceEdges = coordinates::sliceEdges(cube),
    };
}

std::array<Phase1Coordinates, 2> makeExtraPhase1DirectionCoordinates(const CubieCube& cube)
{
    std::array<Phase1Coordinates, 2> result{};
    const auto symmetries = phase1DirectionSymmetries();
    for (std::size_t extraDirection = 0; extraDirection < result.size(); ++extraDirection) {
        const std::size_t direction = extraDirection + 1;
        result[extraDirection] = phase1Coordinates(detail::applySymmetry(cube, symmetries[direction]));
    }
    return result;
}

Phase1Coordinates movedPhase1Coordinates(const Phase1Coordinates& coordinates, Move move)
{
    const int moveIndex = static_cast<int>(move);
    return {
        .cornerOrientation = move_tables::cornerOrientation()[coordinates.cornerOrientation][moveIndex],
        .edgeOrientation = move_tables::edgeOrientation()[coordinates.edgeOrientation][moveIndex],
        .sliceEdges = move_tables::sliceEdges()[coordinates.sliceEdges][moveIndex],
    };
}

SearchNode makeSearchNode(const CubieCube& cube, bool includePhase1Directions)
{
    return {
        .cube = cube,
        .cornerOrientation = coordinates::cornerOrientation(cube),
        .edgeOrientation = coordinates::edgeOrientation(cube),
        .sliceEdges = coordinates::sliceEdges(cube),
        .cornerPermutation = coordinates::cornerPermutation(cube),
        .upEdgePermutation = coordinates::upEdgePermutation(cube),
        .downEdgePermutation = coordinates::downEdgePermutation(cube),
        .extraPhase1Directions = includePhase1Directions
            ? makeExtraPhase1DirectionCoordinates(cube)
            : std::array<Phase1Coordinates, 2>{},
    };
}

SearchNode moved(const SearchNode& node, Move move, bool includePhase1Directions)
{
    const int moveIndex = static_cast<int>(move);
    std::array<Phase1Coordinates, 2> extraPhase1Directions{};
    if (includePhase1Directions) {
        const auto& directionMoves = extraPhase1DirectionMoveTable();
        for (std::size_t direction = 0; direction < extraPhase1Directions.size(); ++direction) {
            extraPhase1Directions[direction] = movedPhase1Coordinates(
                node.extraPhase1Directions[direction],
                directionMoves[direction][moveIndex]);
        }
    }

    return {
        .cube = node.cube.moved(move),
        .cornerOrientation = move_tables::cornerOrientation()[node.cornerOrientation][moveIndex],
        .edgeOrientation = move_tables::edgeOrientation()[node.edgeOrientation][moveIndex],
        .sliceEdges = move_tables::sliceEdges()[node.sliceEdges][moveIndex],
        .cornerPermutation = move_tables::cornerPermutation()[node.cornerPermutation][moveIndex],
        .upEdgePermutation = move_tables::upEdgePermutation()[node.upEdgePermutation][moveIndex],
        .downEdgePermutation = move_tables::downEdgePermutation()[node.downEdgePermutation][moveIndex],
        .extraPhase1Directions = extraPhase1Directions,
    };
}

bool experimentalSymmetryBoundsEnabled()
{
    const char* value = std::getenv("RUBIK_EXPERIMENTAL_SYMMETRY_BOUNDS");
    return value != nullptr && value[0] != '\0' && value[0] != '0';
}

bool environmentFlagEnabled(const char* name)
{
    const char* value = std::getenv(name);
    return value != nullptr && value[0] != '\0' && value[0] != '0';
}

bool threePhase1BoundsEnabled(SolveMode mode, SolveProfile profile)
{
    if (environmentFlagEnabled("RUBIK_DISABLE_THREE_PHASE1_BOUNDS")) {
        return false;
    }
    if (environmentFlagEnabled("RUBIK_EXPERIMENTAL_THREE_PHASE1_BOUNDS")) {
        return true;
    }

    (void)profile;
    return mode == SolveMode::Optimal;
}

bool threePhase1LowerBoundEnabled(SolveProfile profile)
{
    if (environmentFlagEnabled("RUBIK_DISABLE_THREE_PHASE1_BOUNDS")) {
        return false;
    }
    if (environmentFlagEnabled("RUBIK_EXPERIMENTAL_THREE_PHASE1_BOUNDS")) {
        return true;
    }

    (void)profile;
    return true;
}

bool strongOptimalMoveOrderingEnabled()
{
    return environmentFlagEnabled("RUBIK_EXPERIMENTAL_STRONG_OPTIMAL_ORDERING");
}

bool phase2OptimalMoveOrderingEnabled()
{
    return environmentFlagEnabled("RUBIK_EXPERIMENTAL_PHASE2_OPTIMAL_ORDERING");
}

bool experimentalCornerStateBoundsEnabled()
{
    if (environmentFlagEnabled("RUBIK_DISABLE_CORNER_STATE_BOUNDS")) {
        return false;
    }
    if (environmentFlagEnabled("RUBIK_EXPERIMENTAL_CORNER_STATE_BOUNDS")) {
        return true;
    }
    return true;
}

bool experimentalCornerUpEdgeBoundsEnabled()
{
    return environmentFlagEnabled("RUBIK_EXPERIMENTAL_CORNER_UP_EDGE_BOUNDS");
}

bool experimentalCornerDownEdgeBoundsEnabled()
{
    return environmentFlagEnabled("RUBIK_EXPERIMENTAL_CORNER_DOWN_EDGE_BOUNDS");
}

bool largeLocalOptimalProfile(SolveMode mode, SolveProfile profile)
{
    return mode == SolveMode::Optimal && profile == SolveProfile::LargeLocal;
}

bool cornerUpEdgeBoundsEnabled(SolveMode mode, SolveProfile profile)
{
    return largeLocalOptimalProfile(mode, profile) || experimentalCornerUpEdgeBoundsEnabled();
}

bool cornerDownEdgeBoundsEnabled(SolveMode mode, SolveProfile profile)
{
    return largeLocalOptimalProfile(mode, profile) || experimentalCornerDownEdgeBoundsEnabled();
}

std::size_t experimentalCornerStatePayloadBytes()
{
    return static_cast<std::size_t>(coordinates::corner_orientation_count) *
        static_cast<std::size_t>(coordinates::corner_permutation_count);
}

std::size_t experimentalCornerEdgeGroupPayloadBytes()
{
    return static_cast<std::size_t>(coordinates::corner_permutation_count) *
        static_cast<std::size_t>(coordinates::edge_group_permutation_count);
}

std::size_t phase2OrderingPayloadBytes()
{
    return detail::tableProfilePayloadBytes(detail::phase2TableProfile());
}

int phase1CoordinateLowerBound(const Phase1Coordinates& coordinates, const Phase1BoundTables& tables)
{
    const auto cornerOrientationSliceIndex =
        coordinates.cornerOrientation * coordinates::slice_edge_count + coordinates.sliceEdges;
    const auto edgeOrientationSliceIndex =
        coordinates.edgeOrientation * coordinates::slice_edge_count + coordinates.sliceEdges;

    int bound = tables.cornerOrientation[coordinates.cornerOrientation];
    bound = std::max(bound, static_cast<int>(tables.edgeOrientation[coordinates.edgeOrientation]));
    bound = std::max(bound, static_cast<int>(tables.sliceEdges[coordinates.sliceEdges]));
    bound = std::max(bound, static_cast<int>(tables.cornerOrientationSlice[cornerOrientationSliceIndex]));
    bound = std::max(bound, static_cast<int>(tables.edgeOrientationSlice[edgeOrientationSliceIndex]));
    return bound;
}

int nodeThreePhase1LowerBound(const SearchNode& node)
{
    const Phase1BoundTables& tables = phase1BoundTables();
    return std::max(
        phase1CoordinateLowerBound(node.extraPhase1Directions[0], tables),
        phase1CoordinateLowerBound(node.extraPhase1Directions[1], tables));
}

int nodeExperimentalSymmetryLowerBound(const SearchNode& node)
{
    const auto cornerEdgeState =
        node.cornerOrientation * coordinates::edge_orientation_count +
        node.edgeOrientation;
    const auto cornerSliceState =
        node.cornerOrientation * coordinates::slice_edge_count +
        node.sliceEdges;
    const auto edgeSliceState =
        node.edgeOrientation * coordinates::slice_edge_count +
        node.sliceEdges;

    const auto& cornerEdgeReduction = detail::cornerEdgeOrientationSymmetryReduction();
    const auto& cornerSliceReduction = detail::cornerOrientationSliceEdgeSymmetryReduction();
    const auto& edgeSliceReduction = detail::edgeOrientationSliceEdgeSymmetryReduction();

    return std::max({
        static_cast<int>(detail::reducedCornerEdgeOrientationPruning()[
            cornerEdgeReduction.orbitIndex[cornerEdgeState]]),
        static_cast<int>(detail::reducedCornerOrientationSliceEdgePruning()[
            cornerSliceReduction.orbitIndex[cornerSliceState]]),
        static_cast<int>(detail::reducedEdgeOrientationSliceEdgePruning()[
            edgeSliceReduction.orbitIndex[edgeSliceState]]),
    });
}

int nodeBaseLowerBound(const SearchNode& node)
{
    const auto cornerOrientationSliceIndex = node.cornerOrientation * coordinates::slice_edge_count + node.sliceEdges;
    const auto edgeOrientationSliceIndex = node.edgeOrientation * coordinates::slice_edge_count + node.sliceEdges;
    const auto cornerPermutationSliceIndex = node.cornerPermutation * coordinates::slice_edge_count + node.sliceEdges;

    return std::max({
        static_cast<int>(pruning_tables::cornerOrientation()[node.cornerOrientation]),
        static_cast<int>(pruning_tables::edgeOrientation()[node.edgeOrientation]),
        static_cast<int>(pruning_tables::sliceEdges()[node.sliceEdges]),
        static_cast<int>(pruning_tables::cornerPermutation()[node.cornerPermutation]),
        static_cast<int>(pruning_tables::upEdgePermutation()[node.upEdgePermutation]),
        static_cast<int>(pruning_tables::downEdgePermutation()[node.downEdgePermutation]),
        static_cast<int>(pruning_tables::cornerOrientationSlice()[cornerOrientationSliceIndex]),
        static_cast<int>(pruning_tables::edgeOrientationSlice()[edgeOrientationSliceIndex]),
        static_cast<int>(pruning_tables::cornerPermutationSlice()[cornerPermutationSliceIndex]),
    });
}

int nodeLowerBoundWithoutThreePhase1(
    const SearchNode& node,
    SolveProfile profile,
    bool includeExperimentalSymmetryBounds,
    bool includeExperimentalCornerStateBounds,
    bool includeExperimentalCornerUpEdgeBounds,
    bool includeExperimentalCornerDownEdgeBounds)
{
    const auto cornerOrientationPermutationIndex =
        node.cornerOrientation * coordinates::corner_permutation_count + node.cornerPermutation;
    const auto cornerPermutationEdgeOrientationIndex =
        node.cornerPermutation * coordinates::edge_orientation_count + node.edgeOrientation;
    const auto cornerOrientationUpEdgeIndex =
        node.cornerOrientation * coordinates::edge_group_permutation_count + node.upEdgePermutation;
    const auto cornerOrientationDownEdgeIndex =
        node.cornerOrientation * coordinates::edge_group_permutation_count + node.downEdgePermutation;
    const auto cornerPermutationUpEdgeIndex =
        node.cornerPermutation * coordinates::edge_group_permutation_count + node.upEdgePermutation;
    const auto cornerPermutationDownEdgeIndex =
        node.cornerPermutation * coordinates::edge_group_permutation_count + node.downEdgePermutation;
    const auto edgeOrientationUpEdgeIndex =
        node.edgeOrientation * coordinates::edge_group_permutation_count + node.upEdgePermutation;
    const auto edgeOrientationDownEdgeIndex =
        node.edgeOrientation * coordinates::edge_group_permutation_count + node.downEdgePermutation;
    const auto upDownEdgeIndex = node.upEdgePermutation * coordinates::edge_group_permutation_count + node.downEdgePermutation;

    int bound = nodeBaseLowerBound(node);

    if (profile == SolveProfile::Performance || profile == SolveProfile::LargeLocal) {
        bound = std::max(
            bound,
            static_cast<int>(pruning_tables::upDownEdgePermutation()[upDownEdgeIndex]));
    }
    if (includeExperimentalCornerStateBounds) {
        bound = std::max(
            bound,
            static_cast<int>(pruning_tables::cornerOrientationPermutation()[cornerOrientationPermutationIndex]));
    }
    if (includeExperimentalCornerUpEdgeBounds) {
        bound = std::max(
            bound,
            static_cast<int>(pruning_tables::cornerPermutationUpEdgePermutation()[cornerPermutationUpEdgeIndex]));
    }
    if (includeExperimentalCornerDownEdgeBounds) {
        bound = std::max(
            bound,
            static_cast<int>(pruning_tables::cornerPermutationDownEdgePermutation()[cornerPermutationDownEdgeIndex]));
    }
    if (profile != SolveProfile::Embedded) {
        bound = std::max(
            bound,
            static_cast<int>(pruning_tables::cornerPermutationEdgeOrientation()[cornerPermutationEdgeOrientationIndex]));
        bound = std::max(
            bound,
            static_cast<int>(pruning_tables::cornerOrientationUpEdgePermutation()[cornerOrientationUpEdgeIndex]));
        bound = std::max(
            bound,
            static_cast<int>(pruning_tables::cornerOrientationDownEdgePermutation()[cornerOrientationDownEdgeIndex]));
        bound = std::max(
            bound,
            static_cast<int>(pruning_tables::edgeOrientationUpEdgePermutation()[edgeOrientationUpEdgeIndex]));
        bound = std::max(
            bound,
            static_cast<int>(pruning_tables::edgeOrientationDownEdgePermutation()[edgeOrientationDownEdgeIndex]));
    }
    if (includeExperimentalSymmetryBounds) {
        bound = std::max(bound, nodeExperimentalSymmetryLowerBound(node));
    }

    return bound;
}

int nodeLowerBound(
    const SearchNode& node,
    SolveProfile profile,
    bool includeExperimentalSymmetryBounds,
    bool includeExperimentalCornerStateBounds,
    bool includeExperimentalCornerUpEdgeBounds,
    bool includeExperimentalCornerDownEdgeBounds,
    bool includeThreePhase1Bounds)
{
    int bound = nodeLowerBoundWithoutThreePhase1(
        node,
        profile,
        includeExperimentalSymmetryBounds,
        includeExperimentalCornerStateBounds,
        includeExperimentalCornerUpEdgeBounds,
        includeExperimentalCornerDownEdgeBounds);
    if (includeThreePhase1Bounds) {
        bound = std::max(bound, nodeThreePhase1LowerBound(node));
    }

    return bound;
}

int nodePhase2OrderingLowerBound(const SearchNode& node)
{
    if (node.cornerOrientation != 0 ||
        node.edgeOrientation != 0 ||
        node.sliceEdges != 0) {
        return -1;
    }

    const std::uint32_t sliceEdgePermutation = coordinates::sliceEdgePermutation(node.cube);
    const auto cornerSliceIndex =
        node.cornerPermutation * coordinates::slice_edge_permutation_count + sliceEdgePermutation;
    const auto upEdgeSliceIndex =
        node.upEdgePermutation * coordinates::slice_edge_permutation_count + sliceEdgePermutation;
    const auto downEdgeSliceIndex =
        node.downEdgePermutation * coordinates::slice_edge_permutation_count + sliceEdgePermutation;

    return std::max({
        static_cast<int>(pruning_tables::phase2CornerSlicePermutation()[cornerSliceIndex]),
        static_cast<int>(pruning_tables::phase2UpEdgeSlicePermutation()[upEdgeSliceIndex]),
        static_cast<int>(pruning_tables::phase2DownEdgeSlicePermutation()[downEdgeSliceIndex]),
    });
}

int axisOf(Face face)
{
    switch (face) {
    case Face::U:
    case Face::D:
        return 0;
    case Face::R:
    case Face::L:
        return 1;
    case Face::F:
    case Face::B:
        return 2;
    }
    return -1;
}

int faceOrder(Face face)
{
    switch (face) {
    case Face::U:
        return 0;
    case Face::D:
        return 1;
    case Face::R:
        return 0;
    case Face::L:
        return 1;
    case Face::F:
        return 0;
    case Face::B:
        return 1;
    }
    return 0;
}

bool isRedundant(Move previous, Move current)
{
    const Face previousFace = faceOf(previous);
    const Face currentFace = faceOf(current);
    if (previousFace == currentFace) {
        return true;
    }

    return axisOf(previousFace) == axisOf(currentFace) &&
        faceOrder(previousFace) > faceOrder(currentFace);
}

enum class SearchState {
    Found,
    NotFound,
    Timeout
};

struct CandidateMove {
    Move move = Move::U;
    SearchNode node;
    int lowerBound = 0;
    int orderBound = 0;
    int phase2OrderBound = -1;
    int order = 0;
};

struct FastSearchResult {
    SolveStatus status = SolveStatus::DepthLimitExceeded;
    std::vector<Move> moves;
    std::uint64_t nodesExpanded = 0;
    std::vector<std::uint64_t> nodesByDepth;
};

struct TwoPhaseAttemptOptions {
    std::size_t phase1CandidateLimit = 1;
    std::size_t phase2CandidateLimit = 0;
    std::chrono::milliseconds phase1Timeout{0};
    std::chrono::milliseconds phase2Timeout{0};
};

struct TwoPhaseAttemptResult {
    SolveStatus status = SolveStatus::DepthLimitExceeded;
    std::vector<Move> moves;
    std::uint64_t nodesExpanded = 0;
    std::vector<std::uint64_t> nodesByDepth;
};

struct Phase1Candidate {
    std::vector<Move> moves;
    int phase2LowerBound = 0;
};

int phase2CandidateLowerBound(const Cube& cube)
{
    const CubieParseResult parsed = CubieCube::fromCube(cube);
    if (!parsed) {
        return 0;
    }

    const std::uint32_t cornerPermutation = coordinates::cornerPermutation(parsed.cube);
    const std::uint32_t upEdgePermutation = coordinates::upEdgePermutation(parsed.cube);
    const std::uint32_t downEdgePermutation = coordinates::downEdgePermutation(parsed.cube);
    const std::uint32_t sliceEdgePermutation = coordinates::sliceEdgePermutation(parsed.cube);

    const auto cornerSliceIndex =
        cornerPermutation * coordinates::slice_edge_permutation_count + sliceEdgePermutation;
    const auto upEdgeSliceIndex =
        upEdgePermutation * coordinates::slice_edge_permutation_count + sliceEdgePermutation;
    const auto downEdgeSliceIndex =
        downEdgePermutation * coordinates::slice_edge_permutation_count + sliceEdgePermutation;

    return std::max({
        static_cast<int>(pruning_tables::cornerPermutation()[cornerPermutation]),
        static_cast<int>(pruning_tables::upEdgePermutation()[upEdgePermutation]),
        static_cast<int>(pruning_tables::downEdgePermutation()[downEdgePermutation]),
        static_cast<int>(pruning_tables::phase2CornerSlicePermutation()[cornerSliceIndex]),
        static_cast<int>(pruning_tables::phase2UpEdgeSlicePermutation()[upEdgeSliceIndex]),
        static_cast<int>(pruning_tables::phase2DownEdgeSlicePermutation()[downEdgeSliceIndex]),
    });
}

std::size_t beamWidth(SolveProfile profile)
{
    switch (profile) {
    case SolveProfile::Embedded:
        return 1024;
    case SolveProfile::Performance:
    case SolveProfile::LargeLocal:
    case SolveProfile::Auto:
        return 16384;
    case SolveProfile::Default:
        return 4096;
    }
    return 4096;
}

struct BeamEntry {
    SearchNode node;
    std::vector<Move> path;
    int score = 0;
    int lowerBound = 0;
};

FastSearchResult fastBeamSearch(
    const SearchNode& root,
    const Deadline& deadline,
    const SolveOptions& options,
    bool includeExperimentalSymmetryBounds,
    bool includeExperimentalCornerStateBounds,
    bool includeExperimentalCornerUpEdgeBounds,
    bool includeExperimentalCornerDownEdgeBounds,
    bool includeThreePhase1Bounds)
{
    std::vector<BeamEntry> beam;
    const int rootLowerBound = nodeLowerBound(
        root,
        options.profile,
        includeExperimentalSymmetryBounds,
        includeExperimentalCornerStateBounds,
        includeExperimentalCornerUpEdgeBounds,
        includeExperimentalCornerDownEdgeBounds,
        includeThreePhase1Bounds);
    beam.push_back({
        .node = root,
        .path = {},
        .score = rootLowerBound,
        .lowerBound = rootLowerBound,
    });

    const std::size_t width = beamWidth(options.profile);
    FastSearchResult result;

    for (int depth = 0; depth < options.maxDepth; ++depth) {
        if (deadline.expired()) {
            result.status = SolveStatus::Timeout;
            return result;
        }

        std::vector<BeamEntry> nextBeam;
        nextBeam.reserve(std::min<std::size_t>(width * 6, width * move_count));
        const std::uint64_t nodesBeforeDepth = result.nodesExpanded;

        for (const BeamEntry& entry : beam) {
            for (Move move : allMoves()) {
                if (!entry.path.empty() && isRedundant(entry.path.back(), move)) {
                    continue;
                }

                SearchNode next = moved(entry.node, move, includeThreePhase1Bounds);
                ++result.nodesExpanded;

                std::vector<Move> path = entry.path;
                path.push_back(move);

                if (next.cube.isSolved()) {
                    result.status = SolveStatus::Found;
                    result.moves = std::move(path);
                    result.nodesByDepth.push_back(result.nodesExpanded - nodesBeforeDepth);
                    return result;
                }

                const int lowerBound = nodeLowerBound(
                    next,
                    options.profile,
                    includeExperimentalSymmetryBounds,
                    includeExperimentalCornerStateBounds,
                    includeExperimentalCornerUpEdgeBounds,
                    includeExperimentalCornerDownEdgeBounds,
                    includeThreePhase1Bounds);
                if (static_cast<int>(path.size()) + lowerBound > options.maxDepth) {
                    continue;
                }

                nextBeam.push_back({
                    .node = std::move(next),
                    .path = std::move(path),
                    .score = static_cast<int>(path.size()) + lowerBound,
                    .lowerBound = lowerBound,
                });
            }
        }

        result.nodesByDepth.push_back(result.nodesExpanded - nodesBeforeDepth);
        if (nextBeam.empty()) {
            result.status = SolveStatus::DepthLimitExceeded;
            return result;
        }

        const auto compare = [](const BeamEntry& lhs, const BeamEntry& rhs) {
            if (lhs.score != rhs.score) {
                return lhs.score < rhs.score;
            }
            if (lhs.lowerBound != rhs.lowerBound) {
                return lhs.lowerBound < rhs.lowerBound;
            }
            return lhs.path < rhs.path;
        };

        if (nextBeam.size() > width) {
            std::nth_element(nextBeam.begin(), nextBeam.begin() + static_cast<std::ptrdiff_t>(width), nextBeam.end(), compare);
            nextBeam.resize(width);
        }
        std::sort(nextBeam.begin(), nextBeam.end(), compare);
        beam = std::move(nextBeam);
    }

    result.status = SolveStatus::DepthLimitExceeded;
    return result;
}

TwoPhaseAttemptResult twoPhaseAttempt(
    const Cube& cube,
    const Deadline& deadline,
    const SolveOptions& options,
    const TwoPhaseAttemptOptions& attemptOptions)
{
    Phase1CandidatesResult phase1 = findPhase1Candidates(cube, {
        .maxDepth = std::min(options.maxDepth, 12),
        .timeout = attemptOptions.phase1Timeout,
        .profile = options.profile,
        .maxCandidates = attemptOptions.phase1CandidateLimit,
    });

    TwoPhaseAttemptResult result{
        .status = phase1.status,
        .moves = {},
        .nodesExpanded = phase1.nodesExpanded,
        .nodesByDepth = phase1.nodesByDepth,
    };

    if (phase1.status != SolveStatus::Found && phase1.status != SolveStatus::Solved) {
        return result;
    }

    std::vector<Phase1Candidate> candidates;
    candidates.reserve(phase1.candidates.size());

    for (const std::vector<Move>& phase1Moves : phase1.candidates) {
        if (static_cast<int>(phase1Moves.size()) > options.maxDepth) {
            continue;
        }

        Cube phase2Cube = cube;
        phase2Cube.apply(phase1Moves);
        candidates.push_back({
            .moves = phase1Moves,
            .phase2LowerBound = phase2CandidateLowerBound(phase2Cube),
        });
    }

    std::sort(candidates.begin(), candidates.end(), [](const Phase1Candidate& lhs, const Phase1Candidate& rhs) {
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

    bool hasSolution = false;
    std::vector<Move> bestSolution;

    const std::size_t phase2CandidateLimit = attemptOptions.phase2CandidateLimit == 0
        ? candidates.size()
        : std::min(attemptOptions.phase2CandidateLimit, candidates.size());
    for (std::size_t i = 0; i < phase2CandidateLimit; ++i) {
        const Phase1Candidate& candidate = candidates[i];
        if (deadline.expired()) {
            break;
        }

        Cube phase2Cube = cube;
        phase2Cube.apply(candidate.moves);

        Phase2Result phase2 = solvePhase2(phase2Cube, {
            .maxDepth = options.maxDepth - static_cast<int>(candidate.moves.size()),
            .timeout = attemptOptions.phase2Timeout,
            .profile = options.profile,
        });

        result.nodesExpanded += phase2.nodesExpanded;
        result.nodesByDepth.insert(
            result.nodesByDepth.end(),
            phase2.nodesByDepth.begin(),
            phase2.nodesByDepth.end());

        if (phase2.status != SolveStatus::Found && phase2.status != SolveStatus::Solved) {
            continue;
        }

        std::vector<Move> solution = candidate.moves;
        solution.insert(solution.end(), phase2.moves.begin(), phase2.moves.end());
        if (!hasSolution || solution.size() < bestSolution.size()) {
            bestSolution = std::move(solution);
            hasSolution = true;
        }
    }

    if (!hasSolution) {
        result.status = deadline.expired() ? SolveStatus::Timeout : SolveStatus::DepthLimitExceeded;
        return result;
    }

    result.status = SolveStatus::Found;
    result.moves = std::move(bestSolution);
    return result;
}

int collectCandidateMoves(
    const SearchNode& node,
    int depth,
    int limit,
    SolveProfile profile,
    bool includeExperimentalSymmetryBounds,
    bool includeExperimentalCornerStateBounds,
    bool includeExperimentalCornerUpEdgeBounds,
    bool includeExperimentalCornerDownEdgeBounds,
    bool includeThreePhase1Bounds,
    bool useStrongMoveOrdering,
    bool usePhase2MoveOrdering,
    const Move* previousMove,
    std::array<CandidateMove, move_count>& candidates,
    SolveBoundDiagnostics* diagnostics)
{
    int candidateCount = 0;

    for (Move move : allMoves()) {
        if (previousMove != nullptr && isRedundant(*previousMove, move)) {
            continue;
        }

        SearchNode next = moved(node, move, includeThreePhase1Bounds);
        const int candidateCheapLowerBound = nodeLowerBoundWithoutThreePhase1(
            next,
            profile,
            includeExperimentalSymmetryBounds,
            includeExperimentalCornerStateBounds,
            includeExperimentalCornerUpEdgeBounds,
            includeExperimentalCornerDownEdgeBounds);
        const int candidateOrderBound = nodeBaseLowerBound(next);
        const int candidatePhase2OrderBound = usePhase2MoveOrdering
            ? nodePhase2OrderingLowerBound(next)
            : -1;

        if (depth + 1 + candidateCheapLowerBound > limit) {
            if (diagnostics != nullptr) {
                ++diagnostics->cheapCandidatePrunes;
            }
            continue;
        }

        int candidateLowerBound = candidateCheapLowerBound;
        if (includeThreePhase1Bounds) {
            if (diagnostics != nullptr) {
                ++diagnostics->threePhaseCandidateChecks;
            }
            candidateLowerBound = std::max(candidateLowerBound, nodeThreePhase1LowerBound(next));
        }
        if (depth + 1 + candidateLowerBound > limit) {
            if (diagnostics != nullptr) {
                ++diagnostics->threePhaseCandidatePrunes;
            }
            continue;
        }

        candidates[candidateCount] = {
            .move = move,
            .node = std::move(next),
            .lowerBound = candidateLowerBound,
            .orderBound = useStrongMoveOrdering ? candidateLowerBound : candidateOrderBound,
            .phase2OrderBound = candidatePhase2OrderBound,
            .order = static_cast<int>(move),
        };
        ++candidateCount;
    }

    return candidateCount;
}

bool candidateMoveLess(const CandidateMove& lhs, const CandidateMove& rhs)
{
    if (lhs.orderBound != rhs.orderBound) {
        return lhs.orderBound < rhs.orderBound;
    }
    const bool lhsHasPhase2Order = lhs.phase2OrderBound >= 0;
    const bool rhsHasPhase2Order = rhs.phase2OrderBound >= 0;
    if (lhsHasPhase2Order != rhsHasPhase2Order) {
        return lhsHasPhase2Order;
    }
    if (lhsHasPhase2Order && lhs.phase2OrderBound != rhs.phase2OrderBound) {
        return lhs.phase2OrderBound < rhs.phase2OrderBound;
    }
    return lhs.order < rhs.order;
}

SearchState dfs(
    const SearchNode& node,
    int depth,
    int limit,
    const Deadline& deadline,
    SolveProfile profile,
    bool includeExperimentalSymmetryBounds,
    bool includeExperimentalCornerStateBounds,
    bool includeExperimentalCornerUpEdgeBounds,
    bool includeExperimentalCornerDownEdgeBounds,
    bool includeThreePhase1Bounds,
    bool useStrongMoveOrdering,
    bool usePhase2MoveOrdering,
    const GoalTable* exactGoalTable,
    const std::atomic_bool* stopRequested,
    std::vector<Move>& path,
    std::vector<Move>& solution,
    std::uint64_t& nodesExpanded,
    SolveBoundDiagnostics* diagnostics)
{
    ++nodesExpanded;
    if (stopRequested != nullptr && stopRequested->load(std::memory_order_relaxed)) {
        return SearchState::NotFound;
    }
    if (deadline.expired()) {
        return SearchState::Timeout;
    }
    if (node.cube.isSolved()) {
        solution = path;
        return SearchState::Found;
    }
    const int remaining = limit - depth;
    if (exactGoalTable != nullptr && remaining <= exactGoalTable->radius) {
        const auto found = exactGoalTable->indexByKey.find(makeCubieCubeKey(node.cube));
        if (found == exactGoalTable->indexByKey.end()) {
            return SearchState::NotFound;
        }

        const GoalTableEntry& entry = exactGoalTable->entries[found->second];
        if (entry.depth > remaining) {
            return SearchState::NotFound;
        }

        solution = path;
        std::vector<Move> suffix = reconstructGoalTableSuffix(*exactGoalTable, found->second);
        solution.insert(solution.end(), suffix.begin(), suffix.end());
        return SearchState::Found;
    }
    if (depth == 0) {
        const int cheapLowerBound = nodeLowerBoundWithoutThreePhase1(
            node,
            profile,
            includeExperimentalSymmetryBounds,
            includeExperimentalCornerStateBounds,
            includeExperimentalCornerUpEdgeBounds,
            includeExperimentalCornerDownEdgeBounds);
        if (depth + cheapLowerBound > limit) {
            if (diagnostics != nullptr) {
                ++diagnostics->cheapNodePrunes;
            }
            return SearchState::NotFound;
        }
        if (includeThreePhase1Bounds) {
            if (diagnostics != nullptr) {
                ++diagnostics->threePhaseNodeChecks;
            }
            if (depth + std::max(cheapLowerBound, nodeThreePhase1LowerBound(node)) > limit) {
                if (diagnostics != nullptr) {
                    ++diagnostics->threePhaseNodePrunes;
                }
                return SearchState::NotFound;
            }
        }
    }

    std::array<CandidateMove, move_count> candidates{};
    int candidateCount = collectCandidateMoves(
        node,
        depth,
        limit,
        profile,
        includeExperimentalSymmetryBounds,
        includeExperimentalCornerStateBounds,
        includeExperimentalCornerUpEdgeBounds,
        includeExperimentalCornerDownEdgeBounds,
        includeThreePhase1Bounds,
        useStrongMoveOrdering,
        usePhase2MoveOrdering,
        path.empty() ? nullptr : &path.back(),
        candidates,
        diagnostics);

    std::sort(candidates.begin(), candidates.begin() + candidateCount, candidateMoveLess);

    for (int i = 0; i < candidateCount; ++i) {
        const Move move = candidates[i].move;
        const SearchNode& next = candidates[i].node;
        path.push_back(move);

        const SearchState result = dfs(
            next,
            depth + 1,
            limit,
            deadline,
            profile,
            includeExperimentalSymmetryBounds,
            includeExperimentalCornerStateBounds,
            includeExperimentalCornerUpEdgeBounds,
            includeExperimentalCornerDownEdgeBounds,
            includeThreePhase1Bounds,
            useStrongMoveOrdering,
            usePhase2MoveOrdering,
            exactGoalTable,
            stopRequested,
            path,
            solution,
            nodesExpanded,
            diagnostics);
        if (result != SearchState::NotFound) {
            return result;
        }

        path.pop_back();
    }

    return SearchState::NotFound;
}

void mergeDiagnostics(SolveBoundDiagnostics& target, const SolveBoundDiagnostics& source)
{
    target.cheapNodePrunes += source.cheapNodePrunes;
    target.threePhaseNodeChecks += source.threePhaseNodeChecks;
    target.threePhaseNodePrunes += source.threePhaseNodePrunes;
    target.cheapCandidatePrunes += source.cheapCandidatePrunes;
    target.threePhaseCandidateChecks += source.threePhaseCandidateChecks;
    target.threePhaseCandidatePrunes += source.threePhaseCandidatePrunes;
}

SearchState parallelRootDfs(
    const SearchNode& root,
    int limit,
    const Deadline& deadline,
    SolveProfile profile,
    bool includeExperimentalSymmetryBounds,
    bool includeExperimentalCornerStateBounds,
    bool includeExperimentalCornerUpEdgeBounds,
    bool includeExperimentalCornerDownEdgeBounds,
    bool includeThreePhase1Bounds,
    bool useStrongMoveOrdering,
    bool usePhase2MoveOrdering,
    const GoalTable* exactGoalTable,
    unsigned int threadCount,
    std::vector<Move>& solution,
    std::uint64_t& nodesExpanded,
    SolveBoundDiagnostics* diagnostics)
{
    ++nodesExpanded;
    if (deadline.expired()) {
        return SearchState::Timeout;
    }
    if (root.cube.isSolved()) {
        solution.clear();
        return SearchState::Found;
    }

    std::array<CandidateMove, move_count> candidates{};
    const int candidateCount = collectCandidateMoves(
        root,
        0,
        limit,
        profile,
        includeExperimentalSymmetryBounds,
        includeExperimentalCornerStateBounds,
        includeExperimentalCornerUpEdgeBounds,
        includeExperimentalCornerDownEdgeBounds,
        includeThreePhase1Bounds,
        useStrongMoveOrdering,
        usePhase2MoveOrdering,
        nullptr,
        candidates,
        diagnostics);
    std::sort(candidates.begin(), candidates.begin() + candidateCount, candidateMoveLess);

    if (candidateCount == 0) {
        return SearchState::NotFound;
    }

    std::atomic_int nextCandidate{0};
    std::atomic_bool stopRequested{false};
    std::atomic_bool timedOut{false};
    std::mutex resultMutex;
    std::vector<Move> foundSolution;
    std::uint64_t totalWorkerNodes = 0;
    SolveBoundDiagnostics workerDiagnostics;

    const unsigned int workers = std::max(1u, std::min(threadCount, static_cast<unsigned int>(candidateCount)));
    std::vector<std::thread> threads;
    threads.reserve(workers);

    for (unsigned int worker = 0; worker < workers; ++worker) {
        threads.emplace_back([&, worker] {
            (void)worker;
            std::uint64_t localNodes = 0;
            SolveBoundDiagnostics localDiagnostics;
            while (!stopRequested.load(std::memory_order_relaxed)) {
                const int index = nextCandidate.fetch_add(1, std::memory_order_relaxed);
                if (index >= candidateCount) {
                    break;
                }

                std::vector<Move> path{candidates[index].move};
                std::vector<Move> localSolution;
                const SearchState state = dfs(
                    candidates[index].node,
                    1,
                    limit,
                    deadline,
                    profile,
                    includeExperimentalSymmetryBounds,
                    includeExperimentalCornerStateBounds,
                    includeExperimentalCornerUpEdgeBounds,
                    includeExperimentalCornerDownEdgeBounds,
                    includeThreePhase1Bounds,
                    useStrongMoveOrdering,
                    usePhase2MoveOrdering,
                    exactGoalTable,
                    &stopRequested,
                    path,
                    localSolution,
                    localNodes,
                    diagnostics == nullptr ? nullptr : &localDiagnostics);

                if (state == SearchState::Found) {
                    {
                        std::scoped_lock lock(resultMutex);
                        if (foundSolution.empty()) {
                            foundSolution = std::move(localSolution);
                        }
                    }
                    stopRequested.store(true, std::memory_order_relaxed);
                    break;
                }
                if (state == SearchState::Timeout) {
                    timedOut.store(true, std::memory_order_relaxed);
                    stopRequested.store(true, std::memory_order_relaxed);
                    break;
                }
            }

            std::scoped_lock lock(resultMutex);
            totalWorkerNodes += localNodes;
            if (diagnostics != nullptr) {
                mergeDiagnostics(workerDiagnostics, localDiagnostics);
            }
        });
    }

    for (std::thread& thread : threads) {
        thread.join();
    }

    nodesExpanded += totalWorkerNodes;
    if (diagnostics != nullptr) {
        mergeDiagnostics(*diagnostics, workerDiagnostics);
    }
    if (!foundSolution.empty()) {
        solution = std::move(foundSolution);
        return SearchState::Found;
    }
    if (timedOut.load(std::memory_order_relaxed)) {
        return SearchState::Timeout;
    }
    return SearchState::NotFound;
}

} // namespace

SolveResult Solver::solve(const Cube& cube, const SolveOptions& options) const
{
    const auto startedAt = std::chrono::steady_clock::now();
    const auto elapsed = [&] {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startedAt);
    };
    const detail::OptimalPlan plan = detail::makeOptimalPlan(options);
    const auto withPlan = [&](SolveResult result) {
        result.plan = plan.publicPlan;
        return result;
    };
    if (!plan.supported) {
        return withPlan({
            .status = plan.status,
            .moves = {},
            .moveCount = -1,
            .isOptimal = false,
            .metric = options.metric,
            .elapsed = elapsed(),
            .nodesExpanded = 0,
            .memoryUsedBytes = sizeof(Solver),
            .nodesByDepth = {},
            .boundDiagnostics = {},
            .plan = {},
        });
    }
    const SolveOptions effectiveOptions = plan.effectiveOptions;

    if ((effectiveOptions.mode != SolveMode::Optimal && effectiveOptions.mode != SolveMode::Fast) ||
        effectiveOptions.metric != Metric::HTM) {
        return withPlan({
            .status = SolveStatus::UnsupportedOptions,
            .moves = {},
            .moveCount = -1,
            .isOptimal = false,
            .metric = effectiveOptions.metric,
            .elapsed = elapsed(),
            .nodesExpanded = 0,
            .memoryUsedBytes = sizeof(Solver),
            .nodesByDepth = {},
            .boundDiagnostics = {},
            .plan = {},
        });
    }

    if (!cube.isValid()) {
        return withPlan({
            .status = SolveStatus::InvalidCube,
            .moves = {},
            .moveCount = -1,
            .isOptimal = false,
            .metric = effectiveOptions.metric,
            .elapsed = elapsed(),
            .nodesExpanded = 0,
            .memoryUsedBytes = sizeof(Solver),
            .nodesByDepth = {},
            .boundDiagnostics = {},
            .plan = {},
        });
    }
    if (cube.isSolved()) {
        return withPlan({
            .status = SolveStatus::Solved,
            .moves = {},
            .moveCount = 0,
            .isOptimal = true,
            .metric = effectiveOptions.metric,
            .elapsed = elapsed(),
            .nodesExpanded = 0,
            .memoryUsedBytes = sizeof(Solver),
            .nodesByDepth = {},
            .boundDiagnostics = {},
            .plan = {},
        });
    }

    CubieParseResult parsed = CubieCube::fromCube(cube);
    if (!parsed) {
        return withPlan({
            .status = SolveStatus::InvalidCube,
            .moves = {},
            .moveCount = -1,
            .isOptimal = false,
            .metric = effectiveOptions.metric,
            .elapsed = elapsed(),
            .nodesExpanded = 0,
            .memoryUsedBytes = sizeof(Solver),
            .nodesByDepth = {},
            .boundDiagnostics = {},
            .plan = {},
        });
    }

    std::size_t estimatedTableBytes =
        detail::estimatedSolverTablePayloadBytes(effectiveOptions.mode, effectiveOptions.profile);
    const bool includeExperimentalCornerStateBounds = experimentalCornerStateBoundsEnabled();
    const bool includeExperimentalCornerUpEdgeBounds =
        cornerUpEdgeBoundsEnabled(effectiveOptions.mode, effectiveOptions.profile);
    const bool includeExperimentalCornerDownEdgeBounds =
        cornerDownEdgeBoundsEnabled(effectiveOptions.mode, effectiveOptions.profile);
    if (includeExperimentalCornerStateBounds) {
        estimatedTableBytes += experimentalCornerStatePayloadBytes();
    }
    if (includeExperimentalCornerUpEdgeBounds) {
        estimatedTableBytes += experimentalCornerEdgeGroupPayloadBytes();
    }
    if (includeExperimentalCornerDownEdgeBounds) {
        estimatedTableBytes += experimentalCornerEdgeGroupPayloadBytes();
    }
    const bool usePhase2MoveOrdering = phase2OptimalMoveOrderingEnabled() &&
        effectiveOptions.mode == SolveMode::Optimal;
    if (usePhase2MoveOrdering) {
        estimatedTableBytes += phase2OrderingPayloadBytes();
    }
    const int goalTableDepth = effectiveOptions.mode == SolveMode::Optimal
        ? experimentalGoalTableDepth()
        : 0;
    if (goalTableDepth > 0) {
        estimatedTableBytes += experimentalGoalTablePayloadBytes(goalTableDepth);
    }
    const std::size_t estimatedMemoryBytes = sizeof(Solver) + sizeof(Cube) + estimatedTableBytes;
    if (effectiveOptions.maxMemoryBytes > 0 && estimatedMemoryBytes > effectiveOptions.maxMemoryBytes) {
        return withPlan({
            .status = SolveStatus::MemoryLimitExceeded,
            .moves = {},
            .moveCount = -1,
            .isOptimal = false,
            .metric = effectiveOptions.metric,
            .elapsed = elapsed(),
            .nodesExpanded = 0,
            .memoryUsedBytes = estimatedMemoryBytes,
            .nodesByDepth = {},
            .boundDiagnostics = {},
            .plan = {},
        });
    }

    const Deadline deadline(effectiveOptions.timeout);
    std::vector<std::uint64_t> nodesByDepth;
    const bool includeExperimentalSymmetryBounds = experimentalSymmetryBoundsEnabled();
    const bool includeThreePhase1Bounds =
        threePhase1BoundsEnabled(effectiveOptions.mode, effectiveOptions.profile);
    const bool useStrongMoveOrdering = strongOptimalMoveOrderingEnabled();
    const SearchNode root = makeSearchNode(parsed.cube, includeThreePhase1Bounds);
    const GoalTable* exactGoalTable = goalTableDepth > 0
        ? &goalTable(goalTableDepth)
        : nullptr;

    if (effectiveOptions.mode == SolveMode::Fast) {
        const auto remainingTimeout = [&]() {
            if (effectiveOptions.timeout.count() <= 0) {
                return std::chrono::milliseconds{0};
            }
            const auto used = elapsed();
            if (used >= effectiveOptions.timeout) {
                return std::chrono::milliseconds{1};
            }
            return effectiveOptions.timeout - used;
        };

        const auto quickPhaseTimeout = [&]() {
            const std::chrono::milliseconds profileBudget = [&]() {
                switch (effectiveOptions.profile) {
                case SolveProfile::Embedded:
                    return std::chrono::milliseconds{500};
                case SolveProfile::Performance:
                case SolveProfile::LargeLocal:
                case SolveProfile::Auto:
                    return std::chrono::milliseconds{5000};
                case SolveProfile::Default:
                    return std::chrono::milliseconds{2000};
                }
                return std::chrono::milliseconds{2000};
            }();

            const auto remaining = remainingTimeout();
            if (remaining.count() <= 0) {
                return profileBudget;
            }
            return std::min(profileBudget, remaining);
        };

        const auto phase1CandidateLimit = [&]() -> std::size_t {
            switch (effectiveOptions.profile) {
            case SolveProfile::Embedded:
                return 4;
            case SolveProfile::Performance:
            case SolveProfile::LargeLocal:
            case SolveProfile::Auto:
                return 64;
            case SolveProfile::Default:
                return 16;
            }
            return 16;
        };

        const auto quickPhase1CandidateLimit = [&]() -> std::size_t {
            switch (effectiveOptions.profile) {
            case SolveProfile::Embedded:
                return 4;
            case SolveProfile::Performance:
            case SolveProfile::LargeLocal:
            case SolveProfile::Auto:
                return 8;
            case SolveProfile::Default:
                return 4;
            }
            return 4;
        };

        const auto phase2CandidateTimeout = [&]() {
            const std::chrono::milliseconds profileBudget = [&]() {
                switch (effectiveOptions.profile) {
                case SolveProfile::Embedded:
                    return std::chrono::milliseconds{100};
                case SolveProfile::Performance:
                case SolveProfile::LargeLocal:
                case SolveProfile::Auto:
                    return std::chrono::milliseconds{750};
                case SolveProfile::Default:
                    return std::chrono::milliseconds{150};
                }
                return std::chrono::milliseconds{150};
            }();

            const auto remaining = remainingTimeout();
            if (remaining.count() <= 0) {
                return profileBudget;
            }
            return std::min(profileBudget, remaining);
        };

        const auto quickPhase1Timeout = [&]() {
            const auto remaining = remainingTimeout();
            const auto budget =
                effectiveOptions.profile == SolveProfile::Performance ||
                    effectiveOptions.profile == SolveProfile::LargeLocal
                ? std::chrono::milliseconds{500}
                : std::chrono::milliseconds{250};
            if (remaining.count() <= 0) {
                return budget;
            }
            return std::min(budget, remaining);
        };

        const auto quickPhase2Timeout = [&]() {
            const auto remaining = remainingTimeout();
            const auto budget =
                effectiveOptions.profile == SolveProfile::Performance ||
                    effectiveOptions.profile == SolveProfile::LargeLocal
                ? std::chrono::milliseconds{150}
                : std::chrono::milliseconds{75};
            if (remaining.count() <= 0) {
                return budget;
            }
            return std::min(budget, remaining);
        };

        std::vector<TwoPhaseAttemptOptions> attempts;
        if (effectiveOptions.profile != SolveProfile::Embedded) {
            attempts.push_back({
                .phase1CandidateLimit = quickPhase1CandidateLimit(),
                .phase1Timeout = quickPhase1Timeout(),
                .phase2Timeout = quickPhase2Timeout(),
            });
        }
        attempts.push_back({
            .phase1CandidateLimit = effectiveOptions.profile == SolveProfile::Embedded
                ? std::size_t{16}
                : phase1CandidateLimit(),
            .phase2CandidateLimit = effectiveOptions.profile == SolveProfile::Embedded
                ? std::size_t{4}
                : std::size_t{0},
            .phase1Timeout = quickPhaseTimeout(),
            .phase2Timeout = phase2CandidateTimeout(),
        });
        if (effectiveOptions.profile == SolveProfile::Embedded) {
            attempts.push_back({
                .phase1CandidateLimit = 16,
                .phase2CandidateLimit = 0,
                .phase1Timeout = std::chrono::milliseconds{500},
                .phase2Timeout = std::chrono::milliseconds{150},
            });
        }

        std::uint64_t twoPhaseNodesExpanded = 0;
        std::vector<std::uint64_t> twoPhaseNodesByDepth;

        for (const TwoPhaseAttemptOptions& attempt : attempts) {
            if (deadline.expired()) {
                break;
            }

            TwoPhaseAttemptResult twoPhase = twoPhaseAttempt(cube, deadline, effectiveOptions, attempt);
            twoPhaseNodesExpanded += twoPhase.nodesExpanded;
            twoPhaseNodesByDepth.insert(
                twoPhaseNodesByDepth.end(),
                twoPhase.nodesByDepth.begin(),
                twoPhase.nodesByDepth.end());

            if (twoPhase.status == SolveStatus::Found) {
                return withPlan({
                    .status = SolveStatus::Found,
                    .moves = twoPhase.moves,
                    .moveCount = static_cast<int>(twoPhase.moves.size()),
                    .isOptimal = false,
                    .metric = effectiveOptions.metric,
                    .elapsed = elapsed(),
                    .nodesExpanded = twoPhaseNodesExpanded,
                    .memoryUsedBytes = estimatedMemoryBytes + sizeof(Move) * twoPhase.moves.capacity(),
                    .nodesByDepth = twoPhaseNodesByDepth,
                    .boundDiagnostics = {},
                    .plan = {},
                });
            }
        }

        FastSearchResult fast = fastBeamSearch(
            root,
            deadline,
            effectiveOptions,
            includeExperimentalSymmetryBounds,
            includeExperimentalCornerStateBounds,
            includeExperimentalCornerUpEdgeBounds,
            includeExperimentalCornerDownEdgeBounds,
            includeThreePhase1Bounds);
        return withPlan({
            .status = fast.status,
            .moves = fast.moves,
            .moveCount = fast.moves.empty() ? -1 : static_cast<int>(fast.moves.size()),
            .isOptimal = false,
            .metric = effectiveOptions.metric,
            .elapsed = elapsed(),
            .nodesExpanded = fast.nodesExpanded,
            .memoryUsedBytes = estimatedMemoryBytes,
            .nodesByDepth = fast.nodesByDepth,
            .boundDiagnostics = {},
            .plan = {},
        });
    }

    std::uint64_t nodesExpanded = 0;
    SolveBoundDiagnostics boundDiagnostics;
    SolveBoundDiagnostics* boundDiagnosticsPtr = effectiveOptions.collectDiagnostics
        ? &boundDiagnostics
        : nullptr;
    for (int limit = nodeLowerBound(
             root,
             effectiveOptions.profile,
             includeExperimentalSymmetryBounds,
             includeExperimentalCornerStateBounds,
             includeExperimentalCornerUpEdgeBounds,
             includeExperimentalCornerDownEdgeBounds,
             includeThreePhase1Bounds);
         limit <= effectiveOptions.maxDepth;
         ++limit) {
        std::vector<Move> path;
        std::vector<Move> solution;
        const std::uint64_t nodesBeforeDepth = nodesExpanded;

        const SearchState result = effectiveOptions.threads > 1
            ? parallelRootDfs(
                  root,
                  limit,
                  deadline,
                  effectiveOptions.profile,
                  includeExperimentalSymmetryBounds,
                  includeExperimentalCornerStateBounds,
                  includeExperimentalCornerUpEdgeBounds,
                  includeExperimentalCornerDownEdgeBounds,
                  includeThreePhase1Bounds,
                  useStrongMoveOrdering,
                  usePhase2MoveOrdering,
                  exactGoalTable,
                  effectiveOptions.threads,
                  solution,
                  nodesExpanded,
                  boundDiagnosticsPtr)
            : dfs(
                  root,
                  0,
                  limit,
                  deadline,
                  effectiveOptions.profile,
                  includeExperimentalSymmetryBounds,
                  includeExperimentalCornerStateBounds,
                  includeExperimentalCornerUpEdgeBounds,
                  includeExperimentalCornerDownEdgeBounds,
                  includeThreePhase1Bounds,
                  useStrongMoveOrdering,
                  usePhase2MoveOrdering,
                  exactGoalTable,
                  nullptr,
                  path,
                  solution,
                  nodesExpanded,
                  boundDiagnosticsPtr);
        nodesByDepth.push_back(nodesExpanded - nodesBeforeDepth);
        if (result == SearchState::Found) {
            return withPlan({
                .status = SolveStatus::Optimal,
                .moves = solution,
                .moveCount = static_cast<int>(solution.size()),
                .isOptimal = true,
                .metric = effectiveOptions.metric,
                .elapsed = elapsed(),
                .nodesExpanded = nodesExpanded,
                .memoryUsedBytes = estimatedMemoryBytes + sizeof(Move) * solution.capacity(),
                .nodesByDepth = nodesByDepth,
                .boundDiagnostics = boundDiagnostics,
                .plan = {},
            });
        }
        if (result == SearchState::Timeout) {
            return withPlan({
                .status = SolveStatus::Timeout,
                .moves = {},
                .moveCount = -1,
                .isOptimal = false,
                .metric = effectiveOptions.metric,
                .elapsed = elapsed(),
                .nodesExpanded = nodesExpanded,
                .memoryUsedBytes = estimatedMemoryBytes,
                .nodesByDepth = nodesByDepth,
                .boundDiagnostics = boundDiagnostics,
                .plan = {},
            });
        }
    }

    return withPlan({
        .status = SolveStatus::DepthLimitExceeded,
        .moves = {},
        .moveCount = -1,
        .isOptimal = false,
        .metric = effectiveOptions.metric,
        .elapsed = elapsed(),
        .nodesExpanded = nodesExpanded,
        .memoryUsedBytes = estimatedMemoryBytes,
        .nodesByDepth = nodesByDepth,
        .boundDiagnostics = boundDiagnostics,
        .plan = {},
    });
}

int Solver::lowerBound(const Cube& cube, Metric metric, SolveProfile profile) const
{
    if (metric != Metric::HTM || !cube.isValid()) {
        return -1;
    }

    const CubieParseResult parsed = CubieCube::fromCube(cube);
    if (!parsed) {
        return -1;
    }

    const bool includeExperimentalSymmetryBounds = experimentalSymmetryBoundsEnabled();
    const bool includeExperimentalCornerStateBounds = experimentalCornerStateBoundsEnabled();
    const bool includeExperimentalCornerUpEdgeBounds = cornerUpEdgeBoundsEnabled(SolveMode::Optimal, profile);
    const bool includeExperimentalCornerDownEdgeBounds = cornerDownEdgeBoundsEnabled(SolveMode::Optimal, profile);
    const bool includeThreePhase1Bounds = threePhase1LowerBoundEnabled(profile);
    return nodeLowerBound(
        makeSearchNode(parsed.cube, includeThreePhase1Bounds),
        profile,
        includeExperimentalSymmetryBounds,
        includeExperimentalCornerStateBounds,
        includeExperimentalCornerUpEdgeBounds,
        includeExperimentalCornerDownEdgeBounds,
        includeThreePhase1Bounds);
}

} // namespace rubik
