#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "rubik/move.hpp"

namespace rubik {

namespace move_tables {

using CoordinateMoveRow = std::array<std::uint32_t, move_count>;
using CoordinateMoveTable = std::vector<CoordinateMoveRow>;

const CoordinateMoveTable& cornerOrientation();
const CoordinateMoveTable& edgeOrientation();
const CoordinateMoveTable& sliceEdges();
const CoordinateMoveTable& sliceEdgePermutation();
const CoordinateMoveTable& cornerPermutation();
const CoordinateMoveTable& upEdgePermutation();
const CoordinateMoveTable& downEdgePermutation();

} // namespace move_tables

} // namespace rubik
