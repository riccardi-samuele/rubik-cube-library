#include "rubik/coordinates.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace rubik::coordinates {
namespace {

constexpr std::array<std::uint32_t, 13> factorials = {{
    1,
    1,
    2,
    6,
    24,
    120,
    720,
    5040,
    40320,
    362880,
    3628800,
    39916800,
    479001600,
}};

constexpr std::uint32_t binomial(int n, int k)
{
    if (k < 0 || k > n) {
        return 0;
    }
    if (k == 0 || k == n) {
        return 1;
    }

    std::uint32_t result = 1;
    for (int i = 1; i <= k; ++i) {
        result = (result * static_cast<std::uint32_t>(n - k + i)) / static_cast<std::uint32_t>(i);
    }
    return result;
}

template <std::size_t N>
std::uint64_t permutationRank(const std::array<std::uint8_t, N>& permutation)
{
    std::uint64_t rank = 0;
    for (std::size_t i = 0; i < N; ++i) {
        std::uint64_t smallerUnused = 0;
        for (std::size_t j = i + 1; j < N; ++j) {
            if (permutation[j] < permutation[i]) {
                ++smallerUnused;
            }
        }
        rank += smallerUnused * factorials[N - 1 - i];
    }
    return rank;
}

std::uint32_t combinationRank(const std::array<int, 4>& positions)
{
    std::uint32_t rank = 0;
    int previous = -1;
    for (int i = 0; i < 4; ++i) {
        for (int candidate = previous + 1; candidate < positions[i]; ++candidate) {
            rank += binomial(CubieCube::edge_count - candidate - 1, 4 - i - 1);
        }
        previous = positions[i];
    }
    return rank;
}

bool isSliceEdge(std::uint8_t edge)
{
    return edge >= static_cast<std::uint8_t>(Edge::FR) &&
        edge <= static_cast<std::uint8_t>(Edge::BR);
}

bool isEdgeInGroup(std::uint8_t edge, std::uint8_t firstEdge)
{
    return edge >= firstEdge && edge < firstEdge + 4;
}

std::uint32_t edgeGroupPermutation(const CubieCube& cube, std::uint8_t firstEdge)
{
    std::array<int, 4> positions{};
    std::array<std::uint8_t, 4> permutation{};
    int count = 0;
    for (int position = 0; position < CubieCube::edge_count; ++position) {
        const std::uint8_t edge = cube.edgePermutation[position];
        if (isEdgeInGroup(edge, firstEdge)) {
            positions[count] = position;
            permutation[count] = static_cast<std::uint8_t>(edge - firstEdge);
            ++count;
        }
    }

    const std::array<int, 4> goalPositions = {{
        static_cast<int>(firstEdge),
        static_cast<int>(firstEdge) + 1,
        static_cast<int>(firstEdge) + 2,
        static_cast<int>(firstEdge) + 3,
    }};
    const std::uint32_t normalizedCombination =
        (combinationRank(positions) + slice_edge_count - combinationRank(goalPositions)) % slice_edge_count;

    return normalizedCombination * 24 + static_cast<std::uint32_t>(permutationRank(permutation));
}

} // namespace

std::uint32_t cornerOrientation(const CubieCube& cube)
{
    std::uint32_t index = 0;
    for (int i = 0; i < CubieCube::corner_count - 1; ++i) {
        index = index * 3 + cube.cornerOrientation[i];
    }
    return index;
}

std::uint32_t edgeOrientation(const CubieCube& cube)
{
    std::uint32_t index = 0;
    for (int i = 0; i < CubieCube::edge_count - 1; ++i) {
        index = index * 2 + cube.edgeOrientation[i];
    }
    return index;
}

std::uint32_t cornerPermutation(const CubieCube& cube)
{
    return static_cast<std::uint32_t>(permutationRank(cube.cornerPermutation));
}

std::uint64_t edgePermutation(const CubieCube& cube)
{
    return permutationRank(cube.edgePermutation);
}

std::uint32_t sliceEdges(const CubieCube& cube)
{
    std::array<int, 4> positions{};
    int count = 0;
    for (int position = 0; position < CubieCube::edge_count; ++position) {
        if (isSliceEdge(cube.edgePermutation[position])) {
            positions[count] = position;
            ++count;
        }
    }

    std::uint32_t lexRank = 0;
    int previous = -1;
    for (int i = 0; i < 4; ++i) {
        for (int candidate = previous + 1; candidate < positions[i]; ++candidate) {
            lexRank += binomial(CubieCube::edge_count - candidate - 1, 4 - i - 1);
        }
        previous = positions[i];
    }

    return slice_edge_count - 1 - lexRank;
}

std::uint32_t sliceEdgePermutation(const CubieCube& cube)
{
    std::array<std::uint8_t, 4> permutation{};
    for (int i = 0; i < 4; ++i) {
        const std::uint8_t edge = cube.edgePermutation[static_cast<int>(Edge::FR) + i];
        permutation[i] = static_cast<std::uint8_t>(edge - static_cast<std::uint8_t>(Edge::FR));
    }
    return static_cast<std::uint32_t>(permutationRank(permutation));
}

std::uint32_t upEdgePermutation(const CubieCube& cube)
{
    return edgeGroupPermutation(cube, static_cast<std::uint8_t>(Edge::UR));
}

std::uint32_t downEdgePermutation(const CubieCube& cube)
{
    return edgeGroupPermutation(cube, static_cast<std::uint8_t>(Edge::DR));
}

} // namespace rubik::coordinates
