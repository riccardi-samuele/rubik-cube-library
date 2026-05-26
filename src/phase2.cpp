#include "rubik/phase2.hpp"

#include "rubik/coordinates.hpp"
#include "rubik/cubie_cube.hpp"
#include "rubik/move_tables.hpp"
#include "rubik/phase1.hpp"
#include "rubik/pruning_tables.hpp"

#include <algorithm>
#include <array>
#include <queue>

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

struct Phase2Node {
    CubieCube cube;
    std::uint32_t cornerPermutation = 0;
    std::uint32_t upEdgePermutation = 0;
    std::uint32_t downEdgePermutation = 0;
    std::uint32_t sliceEdgePermutation = 0;
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

constexpr std::array<Move, 10> phase2Moves = {{
    Move::U,
    Move::U2,
    Move::Up,
    Move::D,
    Move::D2,
    Move::Dp,
    Move::R2,
    Move::F2,
    Move::L2,
    Move::B2,
}};

Phase2Node makePhase2Node(const CubieCube& cube)
{
    return {
        .cube = cube,
        .cornerPermutation = coordinates::cornerPermutation(cube),
        .upEdgePermutation = coordinates::upEdgePermutation(cube),
        .downEdgePermutation = coordinates::downEdgePermutation(cube),
        .sliceEdgePermutation = coordinates::sliceEdgePermutation(cube),
    };
}

Phase2Node moved(const Phase2Node& node, Move move)
{
    const int moveIndex = static_cast<int>(move);
    return {
        .cube = node.cube.moved(move),
        .cornerPermutation = move_tables::cornerPermutation()[node.cornerPermutation][moveIndex],
        .upEdgePermutation = move_tables::upEdgePermutation()[node.upEdgePermutation][moveIndex],
        .downEdgePermutation = move_tables::downEdgePermutation()[node.downEdgePermutation][moveIndex],
        .sliceEdgePermutation = move_tables::sliceEdgePermutation()[node.sliceEdgePermutation][moveIndex],
    };
}

bool isSolved(const Phase2Node& node)
{
    return node.cornerPermutation == 0 &&
        node.upEdgePermutation == 0 &&
        node.downEdgePermutation == 0 &&
        node.sliceEdgePermutation == 0;
}

const std::array<std::uint8_t, coordinates::slice_edge_permutation_count>& slicePermutationDistances()
{
    static const std::array<std::uint8_t, coordinates::slice_edge_permutation_count> distances = [] {
        std::array<std::uint8_t, coordinates::slice_edge_permutation_count> result{};
        result.fill(0xff);

        std::queue<std::uint32_t> queue;
        result[0] = 0;
        queue.push(0);

        while (!queue.empty()) {
            const std::uint32_t state = queue.front();
            queue.pop();

            for (Move move : phase2Moves) {
                const std::uint32_t next = move_tables::sliceEdgePermutation()[state][static_cast<int>(move)];
                if (result[next] != 0xff) {
                    continue;
                }
                result[next] = static_cast<std::uint8_t>(result[state] + 1);
                queue.push(next);
            }
        }

        return result;
    }();

    return distances;
}

int phase2LowerBound(const Phase2Node& node, SolveProfile profile)
{
    (void)profile;
    const auto cornerSliceIndex =
        node.cornerPermutation * coordinates::slice_edge_permutation_count + node.sliceEdgePermutation;
    const auto upEdgeSliceIndex =
        node.upEdgePermutation * coordinates::slice_edge_permutation_count + node.sliceEdgePermutation;
    const auto downEdgeSliceIndex =
        node.downEdgePermutation * coordinates::slice_edge_permutation_count + node.sliceEdgePermutation;

    return std::max({
        static_cast<int>(pruning_tables::cornerPermutation()[node.cornerPermutation]),
        static_cast<int>(pruning_tables::upEdgePermutation()[node.upEdgePermutation]),
        static_cast<int>(pruning_tables::downEdgePermutation()[node.downEdgePermutation]),
        static_cast<int>(slicePermutationDistances()[node.sliceEdgePermutation]),
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

SearchState phase2Dfs(
    const Phase2Node& node,
    int depth,
    int limit,
    const Deadline& deadline,
    SolveProfile profile,
    std::vector<Move>& path,
    std::vector<Move>& solution,
    std::uint64_t& nodesExpanded)
{
    ++nodesExpanded;
    if (deadline.expired()) {
        return SearchState::Timeout;
    }
    if (isSolved(node)) {
        solution = path;
        return SearchState::Found;
    }
    if (depth + phase2LowerBound(node, profile) > limit) {
        return SearchState::NotFound;
    }

    std::array<CandidateMove, phase2Moves.size()> candidates{};
    int candidateCount = 0;

    for (Move move : phase2Moves) {
        if (!path.empty() && isRedundant(path.back(), move)) {
            continue;
        }

        Phase2Node next = moved(node, move);
        const int candidateLowerBound = phase2LowerBound(next, profile);
        if (depth + 1 + candidateLowerBound > limit) {
            continue;
        }

        candidates[candidateCount] = {
            .move = move,
            .lowerBound = candidateLowerBound,
            .order = static_cast<int>(move),
        };
        ++candidateCount;
    }

    std::sort(candidates.begin(), candidates.begin() + candidateCount, [](const CandidateMove& lhs, const CandidateMove& rhs) {
        if (lhs.lowerBound != rhs.lowerBound) {
            return lhs.lowerBound < rhs.lowerBound;
        }
        return lhs.order < rhs.order;
    });

    for (int i = 0; i < candidateCount; ++i) {
        const Move move = candidates[i].move;
        Phase2Node next = moved(node, move);
        path.push_back(move);

        const SearchState result = phase2Dfs(next, depth + 1, limit, deadline, profile, path, solution, nodesExpanded);
        if (result != SearchState::NotFound) {
            return result;
        }

        path.pop_back();
    }

    return SearchState::NotFound;
}

} // namespace

Phase2Result solvePhase2(const Cube& cube, const Phase2Options& options)
{
    const auto startedAt = std::chrono::steady_clock::now();
    const auto elapsed = [&] {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startedAt);
    };

    if (!cube.isValid()) {
        return {
            .status = SolveStatus::InvalidCube,
            .moves = {},
            .moveCount = -1,
            .elapsed = elapsed(),
            .nodesExpanded = 0,
            .nodesByDepth = {},
        };
    }
    if (!isPhase1Solved(cube)) {
        return {
            .status = SolveStatus::UnsupportedOptions,
            .moves = {},
            .moveCount = -1,
            .elapsed = elapsed(),
            .nodesExpanded = 0,
            .nodesByDepth = {},
        };
    }

    const CubieParseResult parsed = CubieCube::fromCube(cube);
    if (!parsed) {
        return {
            .status = SolveStatus::InvalidCube,
            .moves = {},
            .moveCount = -1,
            .elapsed = elapsed(),
            .nodesExpanded = 0,
            .nodesByDepth = {},
        };
    }

    const Phase2Node root = makePhase2Node(parsed.cube);
    if (isSolved(root)) {
        return {
            .status = SolveStatus::Solved,
            .moves = {},
            .moveCount = 0,
            .elapsed = elapsed(),
            .nodesExpanded = 0,
            .nodesByDepth = {},
        };
    }

    const Deadline deadline(options.timeout);
    std::uint64_t nodesExpanded = 0;
    std::vector<std::uint64_t> nodesByDepth;

    for (int limit = phase2LowerBound(root, options.profile); limit <= options.maxDepth; ++limit) {
        std::vector<Move> path;
        std::vector<Move> solution;
        const std::uint64_t nodesBeforeDepth = nodesExpanded;

        const SearchState result = phase2Dfs(
            root,
            0,
            limit,
            deadline,
            options.profile,
            path,
            solution,
            nodesExpanded);

        nodesByDepth.push_back(nodesExpanded - nodesBeforeDepth);
        if (result == SearchState::Found) {
            return {
                .status = SolveStatus::Found,
                .moves = solution,
                .moveCount = static_cast<int>(solution.size()),
                .elapsed = elapsed(),
                .nodesExpanded = nodesExpanded,
                .nodesByDepth = nodesByDepth,
            };
        }
        if (result == SearchState::Timeout) {
            return {
                .status = SolveStatus::Timeout,
                .moves = {},
                .moveCount = -1,
                .elapsed = elapsed(),
                .nodesExpanded = nodesExpanded,
                .nodesByDepth = nodesByDepth,
            };
        }
    }

    return {
        .status = SolveStatus::DepthLimitExceeded,
        .moves = {},
        .moveCount = -1,
        .elapsed = elapsed(),
        .nodesExpanded = nodesExpanded,
        .nodesByDepth = nodesByDepth,
    };
}

} // namespace rubik
