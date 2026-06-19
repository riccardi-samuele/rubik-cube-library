#include "rubik/phase1.hpp"

#include "rubik/coordinates.hpp"
#include "rubik/cubie_cube.hpp"
#include "rubik/detail/move_restrictions.hpp"
#include "rubik/move_tables.hpp"
#include "rubik/pruning_tables.hpp"

#include <algorithm>
#include <array>
#include <cstddef>

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

struct Phase1Node {
    CubieCube cube;
    std::uint32_t cornerOrientation = 0;
    std::uint32_t edgeOrientation = 0;
    std::uint32_t sliceEdges = 0;
};

struct CandidateMove {
    Move move = Move::U;
    int lowerBound = 0;
    int order = 0;
};

enum class SearchState {
    Found,
    NotFound,
    Timeout
};

Phase1Node makePhase1Node(const CubieCube& cube)
{
    return {
        .cube = cube,
        .cornerOrientation = coordinates::cornerOrientation(cube),
        .edgeOrientation = coordinates::edgeOrientation(cube),
        .sliceEdges = coordinates::sliceEdges(cube),
    };
}

Phase1Node moved(const Phase1Node& node, Move move)
{
    const int moveIndex = static_cast<int>(move);
    return {
        .cube = node.cube.moved(move),
        .cornerOrientation = move_tables::cornerOrientation()[node.cornerOrientation][moveIndex],
        .edgeOrientation = move_tables::edgeOrientation()[node.edgeOrientation][moveIndex],
        .sliceEdges = move_tables::sliceEdges()[node.sliceEdges][moveIndex],
    };
}

bool isPhase1Solved(const Phase1Node& node)
{
    return node.cornerOrientation == 0 &&
        node.edgeOrientation == 0 &&
        node.sliceEdges == 0;
}

int phase1LowerBound(const Phase1Node& node, SolveProfile profile)
{
    (void)profile;
    const auto cornerOrientationSliceIndex =
        node.cornerOrientation * coordinates::slice_edge_count + node.sliceEdges;
    const auto edgeOrientationSliceIndex =
        node.edgeOrientation * coordinates::slice_edge_count + node.sliceEdges;

    return std::max({
        static_cast<int>(pruning_tables::cornerOrientation()[node.cornerOrientation]),
        static_cast<int>(pruning_tables::edgeOrientation()[node.edgeOrientation]),
        static_cast<int>(pruning_tables::sliceEdges()[node.sliceEdges]),
        static_cast<int>(pruning_tables::cornerOrientationSlice()[cornerOrientationSliceIndex]),
        static_cast<int>(pruning_tables::edgeOrientationSlice()[edgeOrientationSliceIndex]),
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

bool containsCandidate(
    const std::vector<std::vector<Move>>& candidates,
    const std::vector<Move>& path)
{
    return std::find(candidates.begin(), candidates.end(), path) != candidates.end();
}

SearchState phase1CandidatesDfs(
    const Phase1Node& node,
    int depth,
    int limit,
    const Deadline& deadline,
    SolveProfile profile,
    std::size_t maxCandidates,
    const std::vector<Move>& allowedMoves,
    std::vector<Move>& path,
    std::vector<std::vector<Move>>& candidates,
    std::uint64_t& nodesExpanded)
{
    ++nodesExpanded;
    if (deadline.expired()) {
        return SearchState::Timeout;
    }
    if (isPhase1Solved(node)) {
        if (!containsCandidate(candidates, path)) {
            candidates.push_back(path);
        }
        return candidates.size() >= maxCandidates ? SearchState::Found : SearchState::NotFound;
    }
    if (depth + phase1LowerBound(node, profile) > limit) {
        return SearchState::NotFound;
    }

    std::array<CandidateMove, move_count> candidateMoves{};
    int candidateMoveCount = 0;

    for (Move move : allowedMoves) {
        if (!path.empty() && isRedundant(path.back(), move)) {
            continue;
        }

        Phase1Node next = moved(node, move);
        const int candidateLowerBound = phase1LowerBound(next, profile);
        if (depth + 1 + candidateLowerBound > limit) {
            continue;
        }

        candidateMoves[candidateMoveCount] = {
            .move = move,
            .lowerBound = candidateLowerBound,
            .order = static_cast<int>(move),
        };
        ++candidateMoveCount;
    }

    std::sort(candidateMoves.begin(), candidateMoves.begin() + candidateMoveCount, [](const CandidateMove& lhs, const CandidateMove& rhs) {
        if (lhs.lowerBound != rhs.lowerBound) {
            return lhs.lowerBound < rhs.lowerBound;
        }
        return lhs.order < rhs.order;
    });

    for (int i = 0; i < candidateMoveCount; ++i) {
        const Move move = candidateMoves[i].move;
        Phase1Node next = moved(node, move);
        path.push_back(move);

        const SearchState result = phase1CandidatesDfs(
            next,
            depth + 1,
            limit,
            deadline,
            profile,
            maxCandidates,
            allowedMoves,
            path,
            candidates,
            nodesExpanded);
        path.pop_back();

        if (result != SearchState::NotFound) {
            return result;
        }
    }

    return SearchState::NotFound;
}

} // namespace

bool isPhase1Solved(const Cube& cube)
{
    if (!cube.isValid()) {
        return false;
    }

    const CubieParseResult parsed = CubieCube::fromCube(cube);
    return parsed && isPhase1Solved(makePhase1Node(parsed.cube));
}

Phase1CandidatesResult findPhase1Candidates(const Cube& cube, const Phase1Options& options)
{
    const auto startedAt = std::chrono::steady_clock::now();
    const auto elapsed = [&] {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startedAt);
    };

    if (!cube.isValid()) {
        return {
            .status = SolveStatus::InvalidCube,
            .candidates = {},
            .elapsed = elapsed(),
            .nodesExpanded = 0,
            .nodesByDepth = {},
        };
    }

    const CubieParseResult parsed = CubieCube::fromCube(cube);
    if (!parsed) {
        return {
            .status = SolveStatus::InvalidCube,
            .candidates = {},
            .elapsed = elapsed(),
            .nodesExpanded = 0,
            .nodesByDepth = {},
        };
    }

    const detail::AllowedMovesResult allowedMoves =
        detail::allowedMovesForBlockedFaces(options.blockedFaces);
    if (allowedMoves.status != SolveStatus::Found) {
        return {
            .status = allowedMoves.status,
            .candidates = {},
            .elapsed = elapsed(),
            .nodesExpanded = 0,
            .nodesByDepth = {},
        };
    }

    const Phase1Node root = makePhase1Node(parsed.cube);
    if (isPhase1Solved(root)) {
        return {
            .status = SolveStatus::Solved,
            .candidates = {{}},
            .elapsed = elapsed(),
            .nodesExpanded = 0,
            .nodesByDepth = {},
        };
    }

    const Deadline deadline(options.timeout);
    std::uint64_t nodesExpanded = 0;
    std::vector<std::uint64_t> nodesByDepth;
    std::vector<std::vector<Move>> candidates;
    const std::size_t maxCandidates = std::max<std::size_t>(1, options.maxCandidates);

    for (int limit = phase1LowerBound(root, options.profile); limit <= options.maxDepth; ++limit) {
        std::vector<Move> path;
        const std::uint64_t nodesBeforeDepth = nodesExpanded;

        const SearchState result = phase1CandidatesDfs(
            root,
            0,
            limit,
            deadline,
            options.profile,
            maxCandidates,
            allowedMoves.moves,
            path,
            candidates,
            nodesExpanded);

        nodesByDepth.push_back(nodesExpanded - nodesBeforeDepth);
        if (result == SearchState::Found || candidates.size() >= maxCandidates) {
            return {
                .status = SolveStatus::Found,
                .candidates = candidates,
                .elapsed = elapsed(),
                .nodesExpanded = nodesExpanded,
                .nodesByDepth = nodesByDepth,
            };
        }
        if (result == SearchState::Timeout) {
            return {
                .status = candidates.empty() ? SolveStatus::Timeout : SolveStatus::Found,
                .candidates = candidates,
                .elapsed = elapsed(),
                .nodesExpanded = nodesExpanded,
                .nodesByDepth = nodesByDepth,
            };
        }
    }

    return {
        .status = candidates.empty() ? SolveStatus::DepthLimitExceeded : SolveStatus::Found,
        .candidates = candidates,
        .elapsed = elapsed(),
        .nodesExpanded = nodesExpanded,
        .nodesByDepth = nodesByDepth,
    };
}

Phase1Result solvePhase1(const Cube& cube, const Phase1Options& options)
{
    Phase1Options singleCandidateOptions = options;
    singleCandidateOptions.maxCandidates = 1;
    const Phase1CandidatesResult candidates = findPhase1Candidates(cube, singleCandidateOptions);

    if (candidates.status == SolveStatus::Found || candidates.status == SolveStatus::Solved) {
        const std::vector<Move> moves = candidates.candidates.empty()
            ? std::vector<Move>{}
            : candidates.candidates.front();
        return {
            .status = moves.empty() ? SolveStatus::Solved : SolveStatus::Found,
            .moves = moves,
            .moveCount = static_cast<int>(moves.size()),
            .elapsed = candidates.elapsed,
            .nodesExpanded = candidates.nodesExpanded,
            .nodesByDepth = candidates.nodesByDepth,
        };
    }

    return {
        .status = candidates.status,
        .moves = {},
        .moveCount = -1,
        .elapsed = candidates.elapsed,
        .nodesExpanded = candidates.nodesExpanded,
        .nodesByDepth = candidates.nodesByDepth,
    };
}

} // namespace rubik
