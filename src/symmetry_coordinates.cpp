#include "rubik/detail/symmetry_coordinates.hpp"

#include "rubik/coordinates.hpp"

#include <array>
#include <limits>
#include <vector>

namespace rubik::detail {
namespace {

template <typename Permutation>
int parity(const Permutation& permutation)
{
    int inversions = 0;
    for (std::size_t i = 0; i < permutation.size(); ++i) {
        for (std::size_t j = i + 1; j < permutation.size(); ++j) {
            if (permutation[i] > permutation[j]) {
                ++inversions;
            }
        }
    }
    return inversions % 2;
}

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

                        if (parity(cube.edgePermutation) != parity(cube.cornerPermutation)) {
                            std::swap(cube.cornerPermutation[0], cube.cornerPermutation[1]);
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

template <typename Factory, typename Encoder>
CoordinateSymmetryTable buildSymmetryTable(std::uint32_t stateCount, Factory factory, Encoder encoder)
{
    CoordinateSymmetryTable table(stateCount);
    for (std::uint32_t state = 0; state < stateCount; ++state) {
        const CubieCube representative = factory(state);
        for (SymmetryId symmetry = 0; symmetry < cube_rotation_symmetry_count; ++symmetry) {
            table[state][symmetry] = encoder(applySymmetry(representative, symmetry));
        }
    }
    return table;
}

CoordinateSymmetryReduction buildReduction(const CoordinateSymmetryTable& symmetryTable)
{
    CoordinateSymmetryReduction reduction;
    reduction.canonicalState.resize(symmetryTable.size());
    reduction.canonicalSymmetry.resize(symmetryTable.size());
    reduction.orbitIndex.resize(symmetryTable.size(), std::numeric_limits<std::uint32_t>::max());

    for (std::uint32_t state = 0; state < symmetryTable.size(); ++state) {
        std::uint32_t canonicalState = symmetryTable[state][0];
        SymmetryId canonicalSymmetry = 0;
        for (SymmetryId symmetry = 1; symmetry < cube_rotation_symmetry_count; ++symmetry) {
            const std::uint32_t candidate = symmetryTable[state][symmetry];
            if (candidate < canonicalState) {
                canonicalState = candidate;
                canonicalSymmetry = symmetry;
            }
        }

        reduction.canonicalState[state] = canonicalState;
        reduction.canonicalSymmetry[state] = canonicalSymmetry;
    }

    std::vector<std::uint32_t> canonicalToOrbit(symmetryTable.size(), std::numeric_limits<std::uint32_t>::max());
    for (std::uint32_t state = 0; state < symmetryTable.size(); ++state) {
        const std::uint32_t canonicalState = reduction.canonicalState[state];
        if (canonicalToOrbit[canonicalState] == std::numeric_limits<std::uint32_t>::max()) {
            canonicalToOrbit[canonicalState] = reduction.orbitCount;
            reduction.orbitRepresentative.push_back(canonicalState);
            ++reduction.orbitCount;
        }
        reduction.orbitIndex[state] = canonicalToOrbit[canonicalState];
    }

    return reduction;
}

CoordinateSymmetryReduction buildCornerEdgeOrientationReduction()
{
    const auto& cornerSymmetries = cornerOrientationSymmetries();
    const auto& edgeSymmetries = edgeOrientationSymmetries();
    const std::uint32_t edgeCount = coordinates::edge_orientation_count;
    const std::uint32_t stateCount = coordinates::corner_orientation_count * edgeCount;

    CoordinateSymmetryReduction reduction;
    reduction.canonicalState.resize(stateCount);
    reduction.canonicalSymmetry.resize(stateCount);
    reduction.orbitIndex.resize(stateCount, std::numeric_limits<std::uint32_t>::max());

    for (std::uint32_t corner = 0; corner < coordinates::corner_orientation_count; ++corner) {
        for (std::uint32_t edge = 0; edge < edgeCount; ++edge) {
            const std::uint32_t state = corner * edgeCount + edge;
            std::uint32_t canonicalState = state;
            SymmetryId canonicalSymmetry = 0;

            for (SymmetryId symmetry = 1; symmetry < cube_rotation_symmetry_count; ++symmetry) {
                const std::uint32_t candidate =
                    cornerSymmetries[corner][symmetry] * edgeCount +
                    edgeSymmetries[edge][symmetry];
                if (candidate < canonicalState) {
                    canonicalState = candidate;
                    canonicalSymmetry = symmetry;
                }
            }

            reduction.canonicalState[state] = canonicalState;
            reduction.canonicalSymmetry[state] = canonicalSymmetry;
        }
    }

    std::vector<std::uint32_t> canonicalToOrbit(stateCount, std::numeric_limits<std::uint32_t>::max());
    for (std::uint32_t state = 0; state < stateCount; ++state) {
        const std::uint32_t canonicalState = reduction.canonicalState[state];
        if (canonicalToOrbit[canonicalState] == std::numeric_limits<std::uint32_t>::max()) {
            canonicalToOrbit[canonicalState] = reduction.orbitCount;
            reduction.orbitRepresentative.push_back(canonicalState);
            ++reduction.orbitCount;
        }
        reduction.orbitIndex[state] = canonicalToOrbit[canonicalState];
    }

    return reduction;
}

std::vector<SymmetryId> udSlicePreservingSymmetries()
{
    std::vector<SymmetryId> symmetries;
    for (SymmetryId symmetry = 0; symmetry < cube_rotation_symmetry_count; ++symmetry) {
        if (preservesUdSlice(symmetry)) {
            symmetries.push_back(symmetry);
        }
    }
    return symmetries;
}

CoordinateSymmetryReduction buildEdgeOrientationSliceEdgeReduction()
{
    const auto& edgeSymmetries = edgeOrientationSymmetries();
    const auto& sliceSymmetries = sliceEdgeSymmetries();
    const auto symmetries = udSlicePreservingSymmetries();
    const std::uint32_t sliceCount = coordinates::slice_edge_count;
    const std::uint32_t stateCount = coordinates::edge_orientation_count * sliceCount;

    CoordinateSymmetryReduction reduction;
    reduction.canonicalState.resize(stateCount);
    reduction.canonicalSymmetry.resize(stateCount);
    reduction.orbitIndex.resize(stateCount, std::numeric_limits<std::uint32_t>::max());

    for (std::uint32_t edge = 0; edge < coordinates::edge_orientation_count; ++edge) {
        for (std::uint32_t slice = 0; slice < sliceCount; ++slice) {
            const std::uint32_t state = edge * sliceCount + slice;
            std::uint32_t canonicalState = state;
            SymmetryId canonicalSymmetry = 0;

            for (SymmetryId symmetry : symmetries) {
                const std::uint32_t candidate =
                    edgeSymmetries[edge][symmetry] * sliceCount +
                    sliceSymmetries[slice][symmetry];
                if (candidate < canonicalState) {
                    canonicalState = candidate;
                    canonicalSymmetry = symmetry;
                }
            }

            reduction.canonicalState[state] = canonicalState;
            reduction.canonicalSymmetry[state] = canonicalSymmetry;
        }
    }

    std::vector<std::uint32_t> canonicalToOrbit(stateCount, std::numeric_limits<std::uint32_t>::max());
    for (std::uint32_t state = 0; state < stateCount; ++state) {
        const std::uint32_t canonicalState = reduction.canonicalState[state];
        if (canonicalToOrbit[canonicalState] == std::numeric_limits<std::uint32_t>::max()) {
            canonicalToOrbit[canonicalState] = reduction.orbitCount;
            reduction.orbitRepresentative.push_back(canonicalState);
            ++reduction.orbitCount;
        }
        reduction.orbitIndex[state] = canonicalToOrbit[canonicalState];
    }

    return reduction;
}

CoordinateSymmetryReduction buildCornerOrientationSliceEdgeReduction()
{
    const auto& cornerSymmetries = cornerOrientationSymmetries();
    const auto& sliceSymmetries = sliceEdgeSymmetries();
    const auto symmetries = udSlicePreservingSymmetries();
    const std::uint32_t sliceCount = coordinates::slice_edge_count;
    const std::uint32_t stateCount = coordinates::corner_orientation_count * sliceCount;

    CoordinateSymmetryReduction reduction;
    reduction.canonicalState.resize(stateCount);
    reduction.canonicalSymmetry.resize(stateCount);
    reduction.orbitIndex.resize(stateCount, std::numeric_limits<std::uint32_t>::max());

    for (std::uint32_t corner = 0; corner < coordinates::corner_orientation_count; ++corner) {
        for (std::uint32_t slice = 0; slice < sliceCount; ++slice) {
            const std::uint32_t state = corner * sliceCount + slice;
            std::uint32_t canonicalState = state;
            SymmetryId canonicalSymmetry = 0;

            for (SymmetryId symmetry : symmetries) {
                const std::uint32_t candidate =
                    cornerSymmetries[corner][symmetry] * sliceCount +
                    sliceSymmetries[slice][symmetry];
                if (candidate < canonicalState) {
                    canonicalState = candidate;
                    canonicalSymmetry = symmetry;
                }
            }

            reduction.canonicalState[state] = canonicalState;
            reduction.canonicalSymmetry[state] = canonicalSymmetry;
        }
    }

    std::vector<std::uint32_t> canonicalToOrbit(stateCount, std::numeric_limits<std::uint32_t>::max());
    for (std::uint32_t state = 0; state < stateCount; ++state) {
        const std::uint32_t canonicalState = reduction.canonicalState[state];
        if (canonicalToOrbit[canonicalState] == std::numeric_limits<std::uint32_t>::max()) {
            canonicalToOrbit[canonicalState] = reduction.orbitCount;
            reduction.orbitRepresentative.push_back(canonicalState);
            ++reduction.orbitCount;
        }
        reduction.orbitIndex[state] = canonicalToOrbit[canonicalState];
    }

    return reduction;
}

} // namespace

const CoordinateSymmetryTable& cornerOrientationSymmetries()
{
    static const CoordinateSymmetryTable table = buildSymmetryTable(
        coordinates::corner_orientation_count,
        cubeFromCornerOrientation,
        coordinates::cornerOrientation);
    return table;
}

const CoordinateSymmetryTable& edgeOrientationSymmetries()
{
    static const CoordinateSymmetryTable table = buildSymmetryTable(
        coordinates::edge_orientation_count,
        cubeFromEdgeOrientation,
        coordinates::edgeOrientation);
    return table;
}

const CoordinateSymmetryTable& sliceEdgeSymmetries()
{
    static const CoordinateSymmetryTable table = buildSymmetryTable(
        coordinates::slice_edge_count,
        cubeFromSliceEdges,
        coordinates::sliceEdges);
    return table;
}

const CoordinateSymmetryReduction& cornerOrientationSymmetryReduction()
{
    static const CoordinateSymmetryReduction reduction = buildReduction(cornerOrientationSymmetries());
    return reduction;
}

const CoordinateSymmetryReduction& edgeOrientationSymmetryReduction()
{
    static const CoordinateSymmetryReduction reduction = buildReduction(edgeOrientationSymmetries());
    return reduction;
}

const CoordinateSymmetryReduction& cornerEdgeOrientationSymmetryReduction()
{
    static const CoordinateSymmetryReduction reduction = buildCornerEdgeOrientationReduction();
    return reduction;
}

const CoordinateSymmetryReduction& cornerOrientationSliceEdgeSymmetryReduction()
{
    static const CoordinateSymmetryReduction reduction = buildCornerOrientationSliceEdgeReduction();
    return reduction;
}

const CoordinateSymmetryReduction& edgeOrientationSliceEdgeSymmetryReduction()
{
    static const CoordinateSymmetryReduction reduction = buildEdgeOrientationSliceEdgeReduction();
    return reduction;
}

} // namespace rubik::detail
