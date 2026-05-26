#pragma once

#include "rubik/pruning_tables.hpp"

namespace rubik::detail {

const pruning_tables::PruningTable& reducedCornerOrientationPruning();
const pruning_tables::PruningTable& reducedEdgeOrientationPruning();
const pruning_tables::PruningTable& reducedCornerEdgeOrientationPruning();
const pruning_tables::PruningTable& reducedCornerOrientationSliceEdgePruning();
const pruning_tables::PruningTable& reducedEdgeOrientationSliceEdgePruning();

} // namespace rubik::detail
