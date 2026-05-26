#pragma once

#include <array>
#include <cstdint>

#include "rubik/cube.hpp"
#include "rubik/cubie_cube.hpp"

namespace rubik::detail {

constexpr int cube_rotation_symmetry_count = 24;

using SymmetryId = std::uint8_t;

struct Symmetry {
    std::array<std::array<int, 3>, 3> matrix{};
};

const std::array<Symmetry, cube_rotation_symmetry_count>& cubeRotationSymmetries();

SymmetryId identitySymmetry();
SymmetryId inverseSymmetry(SymmetryId symmetry);
SymmetryId composeSymmetries(SymmetryId first, SymmetryId second);
bool preservesUdSlice(SymmetryId symmetry);

Cube applySymmetry(const Cube& cube, SymmetryId symmetry);
CubieCube applySymmetry(const CubieCube& cube, SymmetryId symmetry);

} // namespace rubik::detail
