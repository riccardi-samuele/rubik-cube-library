#include "rubik/detail/symmetry_pruning.hpp"

#include "rubik/detail/symmetry_coordinates.hpp"
#include "rubik/move_tables.hpp"

#include <cstdint>
#include <queue>
#include <vector>

namespace rubik::detail {
namespace {

constexpr std::uint8_t unvisited = 0xff;

pruning_tables::PruningTable buildReducedPruningTable(
    const move_tables::CoordinateMoveTable& moves,
    const CoordinateSymmetryReduction& reduction)
{
    std::vector<std::vector<std::uint32_t>> statesByOrbit(reduction.orbitCount);
    for (std::uint32_t state = 0; state < reduction.orbitIndex.size(); ++state) {
        statesByOrbit[reduction.orbitIndex[state]].push_back(state);
    }

    pruning_tables::PruningTable table(reduction.orbitCount, unvisited);
    std::queue<std::uint32_t> frontier;

    const std::uint32_t solvedOrbit = reduction.orbitIndex[0];
    table[solvedOrbit] = 0;
    frontier.push(solvedOrbit);

    while (!frontier.empty()) {
        const std::uint32_t orbit = frontier.front();
        frontier.pop();
        const std::uint8_t nextDepth = static_cast<std::uint8_t>(table[orbit] + 1);

        for (std::uint32_t state : statesByOrbit[orbit]) {
            for (int move = 0; move < move_count; ++move) {
                const std::uint32_t nextOrbit = reduction.orbitIndex[moves[state][move]];
                if (table[nextOrbit] == unvisited) {
                    table[nextOrbit] = nextDepth;
                    frontier.push(nextOrbit);
                }
            }
        }
    }

    return table;
}

} // namespace

const pruning_tables::PruningTable& reducedCornerOrientationPruning()
{
    static const pruning_tables::PruningTable table = buildReducedPruningTable(
        move_tables::cornerOrientation(),
        cornerOrientationSymmetryReduction());
    return table;
}

const pruning_tables::PruningTable& reducedEdgeOrientationPruning()
{
    static const pruning_tables::PruningTable table = buildReducedPruningTable(
        move_tables::edgeOrientation(),
        edgeOrientationSymmetryReduction());
    return table;
}

const pruning_tables::PruningTable& reducedCornerEdgeOrientationPruning()
{
    static const pruning_tables::PruningTable table = [] {
        const auto& reduction = cornerEdgeOrientationSymmetryReduction();
        const auto& cornerMoves = move_tables::cornerOrientation();
        const auto& edgeMoves = move_tables::edgeOrientation();
        const std::uint32_t edgeCount = static_cast<std::uint32_t>(edgeMoves.size());

        std::vector<std::vector<std::uint32_t>> statesByOrbit(reduction.orbitCount);
        for (std::uint32_t state = 0; state < reduction.orbitIndex.size(); ++state) {
            statesByOrbit[reduction.orbitIndex[state]].push_back(state);
        }

        pruning_tables::PruningTable reduced(reduction.orbitCount, unvisited);
        std::queue<std::uint32_t> frontier;

        const std::uint32_t solvedOrbit = reduction.orbitIndex[0];
        reduced[solvedOrbit] = 0;
        frontier.push(solvedOrbit);

        while (!frontier.empty()) {
            const std::uint32_t orbit = frontier.front();
            frontier.pop();
            const std::uint8_t nextDepth = static_cast<std::uint8_t>(reduced[orbit] + 1);

            for (std::uint32_t state : statesByOrbit[orbit]) {
                const std::uint32_t corner = state / edgeCount;
                const std::uint32_t edge = state % edgeCount;
                for (int move = 0; move < move_count; ++move) {
                    const std::uint32_t nextState =
                        cornerMoves[corner][move] * edgeCount +
                        edgeMoves[edge][move];
                    const std::uint32_t nextOrbit = reduction.orbitIndex[nextState];
                    if (reduced[nextOrbit] == unvisited) {
                        reduced[nextOrbit] = nextDepth;
                        frontier.push(nextOrbit);
                    }
                }
            }
        }

        return reduced;
    }();
    return table;
}

const pruning_tables::PruningTable& reducedCornerOrientationSliceEdgePruning()
{
    static const pruning_tables::PruningTable table = [] {
        const auto& reduction = cornerOrientationSliceEdgeSymmetryReduction();
        const auto& cornerMoves = move_tables::cornerOrientation();
        const auto& sliceMoves = move_tables::sliceEdges();
        const std::uint32_t sliceCount = static_cast<std::uint32_t>(sliceMoves.size());

        std::vector<std::vector<std::uint32_t>> statesByOrbit(reduction.orbitCount);
        for (std::uint32_t state = 0; state < reduction.orbitIndex.size(); ++state) {
            statesByOrbit[reduction.orbitIndex[state]].push_back(state);
        }

        pruning_tables::PruningTable reduced(reduction.orbitCount, unvisited);
        std::queue<std::uint32_t> frontier;

        const std::uint32_t solvedOrbit = reduction.orbitIndex[0];
        reduced[solvedOrbit] = 0;
        frontier.push(solvedOrbit);

        while (!frontier.empty()) {
            const std::uint32_t orbit = frontier.front();
            frontier.pop();
            const std::uint8_t nextDepth = static_cast<std::uint8_t>(reduced[orbit] + 1);

            for (std::uint32_t state : statesByOrbit[orbit]) {
                const std::uint32_t corner = state / sliceCount;
                const std::uint32_t slice = state % sliceCount;
                for (int move = 0; move < move_count; ++move) {
                    const std::uint32_t nextState =
                        cornerMoves[corner][move] * sliceCount +
                        sliceMoves[slice][move];
                    const std::uint32_t nextOrbit = reduction.orbitIndex[nextState];
                    if (reduced[nextOrbit] == unvisited) {
                        reduced[nextOrbit] = nextDepth;
                        frontier.push(nextOrbit);
                    }
                }
            }
        }

        return reduced;
    }();
    return table;
}

const pruning_tables::PruningTable& reducedEdgeOrientationSliceEdgePruning()
{
    static const pruning_tables::PruningTable table = [] {
        const auto& reduction = edgeOrientationSliceEdgeSymmetryReduction();
        const auto& edgeMoves = move_tables::edgeOrientation();
        const auto& sliceMoves = move_tables::sliceEdges();
        const std::uint32_t sliceCount = static_cast<std::uint32_t>(sliceMoves.size());

        std::vector<std::vector<std::uint32_t>> statesByOrbit(reduction.orbitCount);
        for (std::uint32_t state = 0; state < reduction.orbitIndex.size(); ++state) {
            statesByOrbit[reduction.orbitIndex[state]].push_back(state);
        }

        pruning_tables::PruningTable reduced(reduction.orbitCount, unvisited);
        std::queue<std::uint32_t> frontier;

        const std::uint32_t solvedOrbit = reduction.orbitIndex[0];
        reduced[solvedOrbit] = 0;
        frontier.push(solvedOrbit);

        while (!frontier.empty()) {
            const std::uint32_t orbit = frontier.front();
            frontier.pop();
            const std::uint8_t nextDepth = static_cast<std::uint8_t>(reduced[orbit] + 1);

            for (std::uint32_t state : statesByOrbit[orbit]) {
                const std::uint32_t edge = state / sliceCount;
                const std::uint32_t slice = state % sliceCount;
                for (int move = 0; move < move_count; ++move) {
                    const std::uint32_t nextState =
                        edgeMoves[edge][move] * sliceCount +
                        sliceMoves[slice][move];
                    const std::uint32_t nextOrbit = reduction.orbitIndex[nextState];
                    if (reduced[nextOrbit] == unvisited) {
                        reduced[nextOrbit] = nextDepth;
                        frontier.push(nextOrbit);
                    }
                }
            }
        }

        return reduced;
    }();
    return table;
}

} // namespace rubik::detail
