#include "rubik/detail/symmetry.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <stdexcept>

namespace rubik::detail {
namespace {

struct Vec3 {
    int x = 0;
    int y = 0;
    int z = 0;

    friend bool operator==(const Vec3& lhs, const Vec3& rhs) = default;
};

struct StickerGeometry {
    Vec3 position;
    Vec3 normal;
};

constexpr std::array<char, 6> face_colors = {'U', 'R', 'F', 'D', 'L', 'B'};

int determinantSign(const std::array<int, 3>& permutation)
{
    int inversions = 0;
    for (int i = 0; i < 3; ++i) {
        for (int j = i + 1; j < 3; ++j) {
            if (permutation[i] > permutation[j]) {
                ++inversions;
            }
        }
    }
    return inversions % 2 == 0 ? 1 : -1;
}

Symmetry makeSymmetry(const std::array<int, 3>& axisPermutation, const std::array<int, 3>& signs)
{
    Symmetry symmetry{};
    for (int row = 0; row < 3; ++row) {
        symmetry.matrix[row].fill(0);
    }
    for (int row = 0; row < 3; ++row) {
        symmetry.matrix[row][axisPermutation[row]] = signs[row];
    }
    return symmetry;
}

Vec3 apply(const Symmetry& symmetry, Vec3 vector)
{
    const std::array<int, 3> input = {vector.x, vector.y, vector.z};
    std::array<int, 3> output{};
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            output[row] += symmetry.matrix[row][col] * input[col];
        }
    }
    return {output[0], output[1], output[2]};
}

Symmetry multiply(const Symmetry& first, const Symmetry& second)
{
    Symmetry result{};
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            for (int k = 0; k < 3; ++k) {
                result.matrix[row][col] += first.matrix[row][k] * second.matrix[k][col];
            }
        }
    }
    return result;
}

bool sameSymmetry(const Symmetry& lhs, const Symmetry& rhs)
{
    return lhs.matrix == rhs.matrix;
}

StickerGeometry stickerGeometry(Face face, int row, int col)
{
    const int a = col - 1;
    const int b = 1 - row;
    switch (face) {
    case Face::U:
        return {{a, 1, row - 1}, {0, 1, 0}};
    case Face::R:
        return {{1, b, 1 - col}, {1, 0, 0}};
    case Face::F:
        return {{a, b, 1}, {0, 0, 1}};
    case Face::D:
        return {{a, -1, 1 - row}, {0, -1, 0}};
    case Face::L:
        return {{-1, b, col - 1}, {-1, 0, 0}};
    case Face::B:
        return {{1 - col, b, -1}, {0, 0, -1}};
    }
    return {};
}

Face faceFromNormal(Vec3 normal)
{
    if (normal.y == 1) {
        return Face::U;
    }
    if (normal.x == 1) {
        return Face::R;
    }
    if (normal.z == 1) {
        return Face::F;
    }
    if (normal.y == -1) {
        return Face::D;
    }
    if (normal.x == -1) {
        return Face::L;
    }
    return Face::B;
}

int indexFromGeometry(const StickerGeometry& sticker)
{
    const Face face = faceFromNormal(sticker.normal);
    int row = 0;
    int col = 0;

    switch (face) {
    case Face::U:
        row = sticker.position.z + 1;
        col = sticker.position.x + 1;
        break;
    case Face::R:
        row = 1 - sticker.position.y;
        col = 1 - sticker.position.z;
        break;
    case Face::F:
        row = 1 - sticker.position.y;
        col = sticker.position.x + 1;
        break;
    case Face::D:
        row = 1 - sticker.position.z;
        col = sticker.position.x + 1;
        break;
    case Face::L:
        row = 1 - sticker.position.y;
        col = sticker.position.z + 1;
        break;
    case Face::B:
        row = 1 - sticker.position.y;
        col = 1 - sticker.position.x;
        break;
    }

    return static_cast<int>(face) * 9 + row * 3 + col;
}

std::array<StickerGeometry, Cube::sticker_count> makeGeometry()
{
    std::array<StickerGeometry, Cube::sticker_count> geometry{};
    for (int face = 0; face < 6; ++face) {
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                geometry[face * 9 + row * 3 + col] = stickerGeometry(static_cast<Face>(face), row, col);
            }
        }
    }
    return geometry;
}

const std::array<StickerGeometry, Cube::sticker_count>& geometry()
{
    static const auto value = makeGeometry();
    return value;
}

std::uint8_t colorIndex(char color)
{
    const auto it = std::find(face_colors.begin(), face_colors.end(), color);
    if (it == face_colors.end()) {
        throw std::invalid_argument("cube symmetry requires canonical U,R,F,D,L,B sticker colors");
    }
    return static_cast<std::uint8_t>(it - face_colors.begin());
}

std::array<char, 6> colorMapFor(const Symmetry& symmetry)
{
    std::array<char, 6> colorMap{};
    for (int face = 0; face < 6; ++face) {
        const StickerGeometry center = stickerGeometry(static_cast<Face>(face), 1, 1);
        const Face targetFace = faceFromNormal(apply(symmetry, center.normal));
        colorMap[face] = face_colors[static_cast<int>(targetFace)];
    }
    return colorMap;
}

Symmetry identityMatrix()
{
    Symmetry identity{};
    identity.matrix = {{
        {{1, 0, 0}},
        {{0, 1, 0}},
        {{0, 0, 1}},
    }};
    return identity;
}

const std::array<Symmetry, cube_rotation_symmetry_count>& buildSymmetries()
{
    static const auto symmetries = [] {
        std::array<Symmetry, cube_rotation_symmetry_count> result{};
        int count = 0;

        std::array<int, 3> axes = {0, 1, 2};
        do {
            const int permutationSign = determinantSign(axes);
            for (int sx : {-1, 1}) {
                for (int sy : {-1, 1}) {
                    for (int sz : {-1, 1}) {
                        if (permutationSign * sx * sy * sz != 1) {
                            continue;
                        }
                        result[count] = makeSymmetry(axes, {sx, sy, sz});
                        ++count;
                    }
                }
            }
        } while (std::next_permutation(axes.begin(), axes.end()));

        assert(count == cube_rotation_symmetry_count);
        const auto identityIt = std::find_if(result.begin(), result.end(), [](const Symmetry& symmetry) {
            return sameSymmetry(symmetry, identityMatrix());
        });
        assert(identityIt != result.end());
        std::swap(result[0], *identityIt);

        return result;
    }();
    return symmetries;
}

SymmetryId findSymmetry(const Symmetry& target)
{
    const auto& symmetries = cubeRotationSymmetries();
    for (SymmetryId i = 0; i < symmetries.size(); ++i) {
        if (sameSymmetry(symmetries[i], target)) {
            return i;
        }
    }
    throw std::logic_error("cube rotation symmetry set is not closed");
}

} // namespace

const std::array<Symmetry, cube_rotation_symmetry_count>& cubeRotationSymmetries()
{
    return buildSymmetries();
}

SymmetryId identitySymmetry()
{
    return 0;
}

SymmetryId inverseSymmetry(SymmetryId symmetry)
{
    const auto& matrix = cubeRotationSymmetries()[symmetry].matrix;
    Symmetry inverse{};
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            inverse.matrix[row][col] = matrix[col][row];
        }
    }
    return findSymmetry(inverse);
}

SymmetryId composeSymmetries(SymmetryId first, SymmetryId second)
{
    return findSymmetry(multiply(cubeRotationSymmetries()[first], cubeRotationSymmetries()[second]));
}

bool preservesUdSlice(SymmetryId symmetry)
{
    return cubeRotationSymmetries()[symmetry].matrix[1][1] != 0;
}

Cube applySymmetry(const Cube& cube, SymmetryId symmetry)
{
    const Symmetry& transform = cubeRotationSymmetries()[symmetry];
    const auto colorMap = colorMapFor(transform);
    Cube::Stickers stickers{};

    for (int source = 0; source < static_cast<int>(Cube::sticker_count); ++source) {
        const StickerGeometry moved = {
            apply(transform, geometry()[source].position),
            apply(transform, geometry()[source].normal),
        };
        const int target = indexFromGeometry(moved);
        stickers[target] = colorMap[colorIndex(cube.stickers()[source])];
    }

    return Cube::fromUncheckedStickers(stickers);
}

CubieCube applySymmetry(const CubieCube& cube, SymmetryId symmetry)
{
    const CubieParseResult parsed = CubieCube::fromCube(applySymmetry(cube.toCube(), symmetry));
    assert(parsed);
    return parsed.cube;
}

} // namespace rubik::detail
