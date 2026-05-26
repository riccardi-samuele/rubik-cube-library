#pragma once

#include <cstdint>

#include "rubik/cubie_cube.hpp"

namespace rubik {

namespace coordinates {

constexpr std::uint32_t corner_orientation_count = 2187; // 3^7
constexpr std::uint32_t edge_orientation_count = 2048;   // 2^11
constexpr std::uint32_t corner_permutation_count = 40320; // 8!
constexpr std::uint64_t edge_permutation_count = 479001600; // 12!
constexpr std::uint32_t slice_edge_count = 495; // C(12, 4)
constexpr std::uint32_t slice_edge_permutation_count = 24; // 4!
constexpr std::uint32_t edge_group_permutation_count = 11880; // C(12, 4) * 4!

std::uint32_t cornerOrientation(const CubieCube& cube);
std::uint32_t edgeOrientation(const CubieCube& cube);
std::uint32_t cornerPermutation(const CubieCube& cube);
std::uint64_t edgePermutation(const CubieCube& cube);
std::uint32_t sliceEdges(const CubieCube& cube);
std::uint32_t sliceEdgePermutation(const CubieCube& cube);
std::uint32_t upEdgePermutation(const CubieCube& cube);
std::uint32_t downEdgePermutation(const CubieCube& cube);

} // namespace coordinates

} // namespace rubik
