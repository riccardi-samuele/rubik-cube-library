#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace rubik {

namespace pruning_tables {

using PruningTable = std::vector<std::uint8_t>;

std::string cacheDirectory();

const PruningTable& cornerOrientation();
const PruningTable& edgeOrientation();
const PruningTable& sliceEdges();
const PruningTable& cornerPermutation();
const PruningTable& upEdgePermutation();
const PruningTable& downEdgePermutation();
const PruningTable& cornerOrientationSlice();
const PruningTable& edgeOrientationSlice();
const PruningTable& cornerOrientationPermutation();
const PruningTable& cornerPermutationSlice();
const PruningTable& cornerOrientationUpEdgePermutation();
const PruningTable& cornerOrientationDownEdgePermutation();
const PruningTable& cornerPermutationUpEdgePermutation();
const PruningTable& cornerPermutationDownEdgePermutation();
const PruningTable& edgeOrientationUpEdgePermutation();
const PruningTable& edgeOrientationDownEdgePermutation();
const PruningTable& upDownEdgePermutation();
const PruningTable& cornerPermutationEdgeOrientation();
const PruningTable& phase2CornerSlicePermutation();
const PruningTable& phase2UpEdgeSlicePermutation();
const PruningTable& phase2DownEdgeSlicePermutation();

} // namespace pruning_tables

} // namespace rubik
