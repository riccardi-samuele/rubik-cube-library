#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "rubik/detail/symmetry.hpp"

namespace rubik::detail {

using CoordinateSymmetryTable = std::vector<std::array<std::uint32_t, cube_rotation_symmetry_count>>;

struct CoordinateSymmetryReduction {
    std::vector<std::uint32_t> canonicalState;
    std::vector<SymmetryId> canonicalSymmetry;
    std::vector<std::uint32_t> orbitIndex;
    std::vector<std::uint32_t> orbitRepresentative;
    std::uint32_t orbitCount = 0;
};

const CoordinateSymmetryTable& cornerOrientationSymmetries();
const CoordinateSymmetryTable& edgeOrientationSymmetries();
const CoordinateSymmetryTable& sliceEdgeSymmetries();

const CoordinateSymmetryReduction& cornerOrientationSymmetryReduction();
const CoordinateSymmetryReduction& edgeOrientationSymmetryReduction();
const CoordinateSymmetryReduction& cornerEdgeOrientationSymmetryReduction();
const CoordinateSymmetryReduction& cornerOrientationSliceEdgeSymmetryReduction();
const CoordinateSymmetryReduction& edgeOrientationSliceEdgeSymmetryReduction();

} // namespace rubik::detail
