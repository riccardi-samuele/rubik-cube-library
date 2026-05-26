#include "rubik/cube.hpp"
#include "rubik/cubie_cube.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <string>

namespace rubik {
namespace {

struct Vec3 {
    int x = 0;
    int y = 0;
    int z = 0;
};

struct StickerGeometry {
    Vec3 position;
    Vec3 normal;
};

constexpr std::array<char, 6> solved_colors = {'U', 'R', 'F', 'D', 'L', 'B'};

int faceOffset(Face face)
{
    return static_cast<int>(face) * 9;
}

char faceColor(Face face)
{
    return solved_colors[static_cast<int>(face)];
}

Vec3 rotateClockwise(Vec3 vector, Face face)
{
    const auto [x, y, z] = vector;
    switch (face) {
    case Face::U:
        return {z, y, -x};
    case Face::D:
        return {-z, y, x};
    case Face::R:
        return {x, -z, y};
    case Face::L:
        return {x, z, -y};
    case Face::F:
        return {y, -x, z};
    case Face::B:
        return {-y, x, z};
    }
    return vector;
}

bool isInTurnLayer(const StickerGeometry& sticker, Face face)
{
    switch (face) {
    case Face::U:
        return sticker.position.y == 1;
    case Face::D:
        return sticker.position.y == -1;
    case Face::R:
        return sticker.position.x == 1;
    case Face::L:
        return sticker.position.x == -1;
    case Face::F:
        return sticker.position.z == 1;
    case Face::B:
        return sticker.position.z == -1;
    }
    return false;
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

    return faceOffset(face) + row * 3 + col;
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

const std::array<int, Cube::sticker_count>& permutationFor(Face face)
{
    static const auto permutations = [] {
        std::array<std::array<int, Cube::sticker_count>, 6> result{};
        for (int faceIndex = 0; faceIndex < 6; ++faceIndex) {
            const Face currentFace = static_cast<Face>(faceIndex);
            for (int source = 0; source < static_cast<int>(Cube::sticker_count); ++source) {
                StickerGeometry moved = geometry()[source];
                if (isInTurnLayer(moved, currentFace)) {
                    moved.position = rotateClockwise(moved.position, currentFace);
                    moved.normal = rotateClockwise(moved.normal, currentFace);
                }
                result[faceIndex][indexFromGeometry(moved)] = source;
            }
        }
        return result;
    }();
    return permutations[static_cast<int>(face)];
}

} // namespace

InvalidCube::InvalidCube(const std::string& message)
    : std::runtime_error(message)
{
}

Cube::Cube()
    : Cube(solved())
{
}

Cube::Cube(const Stickers& stickers)
    : stickers_(stickers)
{
}

Cube Cube::solved()
{
    Stickers stickers{};
    for (int face = 0; face < 6; ++face) {
        std::fill_n(stickers.begin() + face * 9, 9, solved_colors[face]);
    }
    return Cube(stickers);
}

Cube Cube::fromUncheckedStickers(const Stickers& stickers)
{
    return Cube(stickers);
}

Cube Cube::fromString(std::string_view stickers)
{
    CubeParseResult result = Cube::fromStickers(stickers);
    if (!result) {
        throw InvalidCube(result.error.message);
    }
    return result.cube;
}

CubeParseResult Cube::fromStickers(std::string_view stickers)
{
    return rubik::fromStickers(stickers);
}

CubeParseResult Cube::fromStickers(const Stickers& stickers)
{
    return rubik::fromStickers(stickers);
}

CubeError Cube::validateStickers(std::string_view stickers)
{
    return rubik::validateStickers(stickers);
}

CubeError Cube::validateStickers(const Stickers& stickers)
{
    return rubik::validateStickers(stickers);
}

const Cube::Stickers& Cube::stickers() const
{
    return stickers_;
}

std::string Cube::toString() const
{
    return {stickers_.begin(), stickers_.end()};
}

bool Cube::isSolved() const
{
    for (int face = 0; face < 6; ++face) {
        for (int offset = 0; offset < 9; ++offset) {
            if (stickers_[face * 9 + offset] != faceColor(static_cast<Face>(face))) {
                return false;
            }
        }
    }
    return true;
}

bool Cube::isValid() const
{
    return validateStickers(stickers_).code == CubeErrorCode::None;
}

void Cube::apply(Move move)
{
    const auto& permutation = permutationFor(faceOf(move));
    for (int turn = 0; turn < quarterTurns(move); ++turn) {
        Stickers next{};
        for (std::size_t i = 0; i < sticker_count; ++i) {
            next[i] = stickers_[permutation[i]];
        }
        stickers_ = next;
    }
}

void Cube::apply(const std::vector<Move>& moves)
{
    for (Move move : moves) {
        apply(move);
    }
}

Cube Cube::moved(Move move) const
{
    Cube cube = *this;
    cube.apply(move);
    return cube;
}

bool CubeParseResult::ok() const
{
    return error.code == CubeErrorCode::None;
}

CubeParseResult::operator bool() const
{
    return ok();
}

CubeParseResult fromStickers(std::string_view stickers)
{
    CubeError error = validateStickers(stickers);
    if (error.code != CubeErrorCode::None) {
        return {.cube = Cube::solved(), .error = error};
    }

    Cube::Stickers values{};
    std::copy(stickers.begin(), stickers.end(), values.begin());
    return {.cube = Cube::fromUncheckedStickers(values), .error = {}};
}

CubeParseResult fromStickers(const Cube::Stickers& stickers)
{
    CubeError error = validateStickers(stickers);
    if (error.code != CubeErrorCode::None) {
        return {.cube = Cube::solved(), .error = error};
    }
    return {.cube = Cube::fromUncheckedStickers(stickers), .error = {}};
}

CubeError validateStickers(std::string_view stickers)
{
    if (stickers.size() != Cube::sticker_count) {
        return {
            .code = CubeErrorCode::InvalidStickerCount,
            .message = "a cube must contain exactly 54 stickers",
        };
    }

    Cube::Stickers values{};
    std::copy(stickers.begin(), stickers.end(), values.begin());
    return validateStickers(values);
}

CubeError validateStickers(const Cube::Stickers& stickers)
{
    std::map<char, int> counts;
    for (char sticker : stickers) {
        if (std::find(solved_colors.begin(), solved_colors.end(), sticker) == solved_colors.end()) {
            return {
                .code = CubeErrorCode::InvalidColor,
                .message = "stickers must use only U,R,F,D,L,B",
            };
        }
        ++counts[sticker];
    }

    for (char color : solved_colors) {
        if (counts[color] != 9) {
            return {
                .code = CubeErrorCode::InvalidColorCount,
                .message = "a cube must contain exactly nine stickers for each color U,R,F,D,L,B",
            };
        }
    }

    for (int face = 0; face < 6; ++face) {
        if (stickers[face * 9 + 4] != solved_colors[face]) {
            return {
                .code = CubeErrorCode::InvalidCenterConfiguration,
                .message = "center stickers must be ordered as U,R,F,D,L,B",
            };
        }
    }

    CubieParseResult cubie = CubieCube::fromStickers(stickers);
    if (!cubie) {
        return cubie.error;
    }

    return {};
}

} // namespace rubik
