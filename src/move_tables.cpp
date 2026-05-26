#include "rubik/move_tables.hpp"

#include "rubik/coordinates.hpp"
#include "rubik/cubie_cube.hpp"

#include <array>

namespace rubik::move_tables {
namespace {

constexpr std::array<std::uint32_t, 9> factorials = {{
    1,
    1,
    2,
    6,
    24,
    120,
    720,
    5040,
    40320,
}};

std::array<std::uint8_t, 4> permutation4FromRank(std::uint32_t index);

CubieCube cubeFromCornerOrientation(std::uint32_t index)
{
    CubieCube cube = CubieCube::solved();
    int sum = 0;
    for (int i = CubieCube::corner_count - 2; i >= 0; --i) {
        cube.cornerOrientation[i] = static_cast<std::uint8_t>(index % 3);
        sum += cube.cornerOrientation[i];
        index /= 3;
    }
    cube.cornerOrientation[CubieCube::corner_count - 1] = static_cast<std::uint8_t>((3 - (sum % 3)) % 3);
    return cube;
}

CubieCube cubeFromEdgeOrientation(std::uint32_t index)
{
    CubieCube cube = CubieCube::solved();
    int sum = 0;
    for (int i = CubieCube::edge_count - 2; i >= 0; --i) {
        cube.edgeOrientation[i] = static_cast<std::uint8_t>(index % 2);
        sum += cube.edgeOrientation[i];
        index /= 2;
    }
    cube.edgeOrientation[CubieCube::edge_count - 1] = static_cast<std::uint8_t>(sum % 2);
    return cube;
}

const std::vector<CubieCube>& sliceEdgeRepresentatives()
{
    static const std::vector<CubieCube> representatives = [] {
        std::vector<CubieCube> result(coordinates::slice_edge_count);

        for (int a = 0; a < CubieCube::edge_count - 3; ++a) {
            for (int b = a + 1; b < CubieCube::edge_count - 2; ++b) {
                for (int c = b + 1; c < CubieCube::edge_count - 1; ++c) {
                    for (int d = c + 1; d < CubieCube::edge_count; ++d) {
                        CubieCube cube = CubieCube::solved();
                        const std::array<int, 4> slicePositions = {{a, b, c, d}};
                        int slice = 0;
                        int regular = 0;

                        for (int position = 0; position < CubieCube::edge_count; ++position) {
                            if (slice < 4 && position == slicePositions[slice]) {
                                cube.edgePermutation[position] = static_cast<std::uint8_t>(
                                    static_cast<int>(Edge::FR) + slice);
                                ++slice;
                            } else {
                                cube.edgePermutation[position] = static_cast<std::uint8_t>(regular);
                                ++regular;
                            }
                        }

                        result[coordinates::sliceEdges(cube)] = cube;
                    }
                }
            }
        }

        return result;
    }();

    return representatives;
}

CubieCube cubeFromSliceEdges(std::uint32_t index)
{
    return sliceEdgeRepresentatives()[index];
}

CubieCube cubeFromCornerPermutation(std::uint32_t index)
{
    CubieCube cube = CubieCube::solved();
    std::array<std::uint8_t, CubieCube::corner_count> values = {{
        0, 1, 2, 3, 4, 5, 6, 7,
    }};

    for (int position = 0; position < CubieCube::corner_count; ++position) {
        const std::uint32_t divisor = factorials[CubieCube::corner_count - 1 - position];
        const std::uint32_t selected = index / divisor;
        index %= divisor;

        cube.cornerPermutation[position] = values[selected];
        for (std::uint32_t i = selected; i + 1 < values.size(); ++i) {
            values[i] = values[i + 1];
        }
    }

    return cube;
}

CubieCube cubeFromSliceEdgePermutation(std::uint32_t index)
{
    CubieCube cube = CubieCube::solved();
    const auto permutation = permutation4FromRank(index);
    for (int i = 0; i < 4; ++i) {
        cube.edgePermutation[static_cast<int>(Edge::FR) + i] =
            static_cast<std::uint8_t>(static_cast<int>(Edge::FR) + permutation[i]);
    }
    return cube;
}

std::array<std::uint8_t, 4> permutation4FromRank(std::uint32_t index)
{
    std::array<std::uint8_t, 4> permutation{};
    std::array<std::uint8_t, 4> values = {{0, 1, 2, 3}};

    for (int position = 0; position < 4; ++position) {
        const std::uint32_t divisor = factorials[3 - position];
        const std::uint32_t selected = index / divisor;
        index %= divisor;

        permutation[position] = values[selected];
        for (std::uint32_t i = selected; i + 1 < values.size(); ++i) {
            values[i] = values[i + 1];
        }
    }

    return permutation;
}

template <typename Encoder>
std::vector<CubieCube> buildEdgeGroupRepresentatives(std::uint8_t firstEdge, Encoder encoder)
{
    std::vector<CubieCube> representatives(coordinates::edge_group_permutation_count);
    for (int a = 0; a < CubieCube::edge_count - 3; ++a) {
        for (int b = a + 1; b < CubieCube::edge_count - 2; ++b) {
            for (int c = b + 1; c < CubieCube::edge_count - 1; ++c) {
                for (int d = c + 1; d < CubieCube::edge_count; ++d) {
                    const std::array<int, 4> groupPositions = {{a, b, c, d}};
                    for (std::uint32_t permutationRank = 0; permutationRank < 24; ++permutationRank) {
                        CubieCube cube = CubieCube::solved();
                        std::array<bool, CubieCube::edge_count> occupied{};
                        const auto permutation = permutation4FromRank(permutationRank);

                        for (int i = 0; i < 4; ++i) {
                            const int position = groupPositions[i];
                            cube.edgePermutation[position] = static_cast<std::uint8_t>(firstEdge + permutation[i]);
                            occupied[position] = true;
                        }

                        std::uint8_t nextOtherEdge = 0;
                        for (int position = 0; position < CubieCube::edge_count; ++position) {
                            if (occupied[position]) {
                                continue;
                            }
                            while (nextOtherEdge >= firstEdge && nextOtherEdge < firstEdge + 4) {
                                ++nextOtherEdge;
                            }
                            cube.edgePermutation[position] = nextOtherEdge;
                            ++nextOtherEdge;
                        }

                        representatives[encoder(cube)] = cube;
                    }
                }
            }
        }
    }

    return representatives;
}

const std::vector<CubieCube>& upEdgeRepresentatives()
{
    static const std::vector<CubieCube> representatives = buildEdgeGroupRepresentatives(
        static_cast<std::uint8_t>(Edge::UR),
        coordinates::upEdgePermutation);
    return representatives;
}

const std::vector<CubieCube>& downEdgeRepresentatives()
{
    static const std::vector<CubieCube> representatives = buildEdgeGroupRepresentatives(
        static_cast<std::uint8_t>(Edge::DR),
        coordinates::downEdgePermutation);
    return representatives;
}

CubieCube cubeFromUpEdgePermutation(std::uint32_t index)
{
    return upEdgeRepresentatives()[index];
}

CubieCube cubeFromDownEdgePermutation(std::uint32_t index)
{
    return downEdgeRepresentatives()[index];
}

template <typename Factory, typename Encoder>
CoordinateMoveTable buildTable(std::uint32_t stateCount, Factory factory, Encoder encoder)
{
    CoordinateMoveTable table(stateCount);
    for (std::uint32_t state = 0; state < stateCount; ++state) {
        for (Move move : allMoves()) {
            CubieCube cube = factory(state);
            cube.apply(move);
            table[state][static_cast<int>(move)] = encoder(cube);
        }
    }
    return table;
}

bool isPhase2Move(Move move)
{
    switch (move) {
    case Move::U:
    case Move::U2:
    case Move::Up:
    case Move::D:
    case Move::D2:
    case Move::Dp:
    case Move::R2:
    case Move::F2:
    case Move::L2:
    case Move::B2:
        return true;
    case Move::R:
    case Move::Rp:
    case Move::F:
    case Move::Fp:
    case Move::L:
    case Move::Lp:
    case Move::B:
    case Move::Bp:
        return false;
    }
    return false;
}

CoordinateMoveTable buildSliceEdgePermutationTable()
{
    CoordinateMoveTable table(coordinates::slice_edge_permutation_count);
    for (std::uint32_t state = 0; state < coordinates::slice_edge_permutation_count; ++state) {
        for (Move move : allMoves()) {
            if (!isPhase2Move(move)) {
                table[state][static_cast<int>(move)] = state;
                continue;
            }

            CubieCube cube = cubeFromSliceEdgePermutation(state);
            cube.apply(move);
            table[state][static_cast<int>(move)] = coordinates::sliceEdgePermutation(cube);
        }
    }
    return table;
}

} // namespace

const CoordinateMoveTable& cornerOrientation()
{
    static const CoordinateMoveTable table = buildTable(
        coordinates::corner_orientation_count,
        cubeFromCornerOrientation,
        coordinates::cornerOrientation);
    return table;
}

const CoordinateMoveTable& edgeOrientation()
{
    static const CoordinateMoveTable table = buildTable(
        coordinates::edge_orientation_count,
        cubeFromEdgeOrientation,
        coordinates::edgeOrientation);
    return table;
}

const CoordinateMoveTable& sliceEdges()
{
    static const CoordinateMoveTable table = buildTable(
        coordinates::slice_edge_count,
        cubeFromSliceEdges,
        coordinates::sliceEdges);
    return table;
}

const CoordinateMoveTable& sliceEdgePermutation()
{
    static const CoordinateMoveTable table = buildSliceEdgePermutationTable();
    return table;
}

const CoordinateMoveTable& cornerPermutation()
{
    static const CoordinateMoveTable table = buildTable(
        coordinates::corner_permutation_count,
        cubeFromCornerPermutation,
        coordinates::cornerPermutation);
    return table;
}

const CoordinateMoveTable& upEdgePermutation()
{
    static const CoordinateMoveTable table = buildTable(
        coordinates::edge_group_permutation_count,
        cubeFromUpEdgePermutation,
        coordinates::upEdgePermutation);
    return table;
}

const CoordinateMoveTable& downEdgePermutation()
{
    static const CoordinateMoveTable table = buildTable(
        coordinates::edge_group_permutation_count,
        cubeFromDownEdgePermutation,
        coordinates::downEdgePermutation);
    return table;
}

} // namespace rubik::move_tables
