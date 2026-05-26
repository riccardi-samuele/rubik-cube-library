#include "rubik/cubie_cube.hpp"

#include <algorithm>
#include <array>

namespace rubik {
namespace {

constexpr std::array<std::array<int, 3>, CubieCube::corner_count> corner_facelets = {{
    {{8, 9, 20}},
    {{6, 18, 38}},
    {{0, 36, 47}},
    {{2, 45, 11}},
    {{29, 26, 15}},
    {{27, 44, 24}},
    {{33, 53, 42}},
    {{35, 17, 51}},
}};

constexpr std::array<std::array<char, 3>, CubieCube::corner_count> corner_colors = {{
    {{'U', 'R', 'F'}},
    {{'U', 'F', 'L'}},
    {{'U', 'L', 'B'}},
    {{'U', 'B', 'R'}},
    {{'D', 'F', 'R'}},
    {{'D', 'L', 'F'}},
    {{'D', 'B', 'L'}},
    {{'D', 'R', 'B'}},
}};

constexpr std::array<std::array<int, 2>, CubieCube::edge_count> edge_facelets = {{
    {{5, 10}},
    {{7, 19}},
    {{3, 37}},
    {{1, 46}},
    {{32, 16}},
    {{28, 25}},
    {{30, 43}},
    {{34, 52}},
    {{23, 12}},
    {{21, 41}},
    {{50, 39}},
    {{48, 14}},
}};

constexpr std::array<std::array<char, 2>, CubieCube::edge_count> edge_colors = {{
    {{'U', 'R'}},
    {{'U', 'F'}},
    {{'U', 'L'}},
    {{'U', 'B'}},
    {{'D', 'R'}},
    {{'D', 'F'}},
    {{'D', 'L'}},
    {{'D', 'B'}},
    {{'F', 'R'}},
    {{'F', 'L'}},
    {{'B', 'L'}},
    {{'B', 'R'}},
}};

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

template <typename Permutation>
bool isPermutation(const Permutation& permutation)
{
    std::array<bool, 12> seen{};
    for (std::uint8_t value : permutation) {
        if (value >= permutation.size() || seen[value]) {
            return false;
        }
        seen[value] = true;
    }
    return true;
}

bool sameCornerCubie(const std::array<char, 3>& a, const std::array<char, 3>& b)
{
    for (char color : a) {
        if (std::find(b.begin(), b.end(), color) == b.end()) {
            return false;
        }
    }
    return true;
}

CubieCube stickerMoveTransform(Move move)
{
    Cube cube = Cube::solved();
    cube.apply(move);
    return CubieCube::fromCube(cube).cube;
}

const CubieCube& moveTransform(Move move)
{
    static const std::array<CubieCube, move_count> transforms = [] {
        std::array<CubieCube, move_count> result{};
        for (Move move : allMoves()) {
            result[static_cast<int>(move)] = stickerMoveTransform(move);
        }
        return result;
    }();

    return transforms[static_cast<int>(move)];
}

} // namespace

CubieCube CubieCube::solved()
{
    CubieCube cube;
    for (int i = 0; i < corner_count; ++i) {
        cube.cornerPermutation[i] = static_cast<std::uint8_t>(i);
        cube.cornerOrientation[i] = 0;
    }
    for (int i = 0; i < edge_count; ++i) {
        cube.edgePermutation[i] = static_cast<std::uint8_t>(i);
        cube.edgeOrientation[i] = 0;
    }
    return cube;
}

CubieParseResult CubieCube::fromCube(const Cube& cube)
{
    return fromStickers(cube.stickers());
}

CubieParseResult CubieCube::fromStickers(const Cube::Stickers& stickers)
{
    CubieCube cube;

    for (int position = 0; position < corner_count; ++position) {
        std::array<char, 3> colors = {{
            stickers[corner_facelets[position][0]],
            stickers[corner_facelets[position][1]],
            stickers[corner_facelets[position][2]],
        }};

        int orientation = 0;
        while (orientation < 3 && colors[orientation] != 'U' && colors[orientation] != 'D') {
            ++orientation;
        }
        if (orientation == 3) {
            return {
                .cube = CubieCube::solved(),
                .error = {
                    .code = CubeErrorCode::InvalidCornerOrientation,
                    .message = "each corner must contain exactly one U or D sticker",
                },
            };
        }

        bool found = false;
        for (int cubie = 0; cubie < corner_count; ++cubie) {
            if (sameCornerCubie(colors, corner_colors[cubie])) {
                cube.cornerPermutation[position] = static_cast<std::uint8_t>(cubie);
                cube.cornerOrientation[position] = static_cast<std::uint8_t>(orientation % 3);
                found = true;
                break;
            }
        }
        if (!found) {
            return {
                .cube = CubieCube::solved(),
                .error = {
                    .code = CubeErrorCode::InvalidCornerPermutation,
                    .message = "corner stickers do not form the eight physical corner cubies",
                },
            };
        }
    }

    for (int position = 0; position < edge_count; ++position) {
        const char first = stickers[edge_facelets[position][0]];
        const char second = stickers[edge_facelets[position][1]];

        bool found = false;
        for (int cubie = 0; cubie < edge_count; ++cubie) {
            if (first == edge_colors[cubie][0] && second == edge_colors[cubie][1]) {
                cube.edgePermutation[position] = static_cast<std::uint8_t>(cubie);
                cube.edgeOrientation[position] = 0;
                found = true;
                break;
            }
            if (first == edge_colors[cubie][1] && second == edge_colors[cubie][0]) {
                cube.edgePermutation[position] = static_cast<std::uint8_t>(cubie);
                cube.edgeOrientation[position] = 1;
                found = true;
                break;
            }
        }
        if (!found) {
            return {
                .cube = CubieCube::solved(),
                .error = {
                    .code = CubeErrorCode::InvalidEdgePermutation,
                    .message = "edge stickers do not form the twelve physical edge cubies",
                },
            };
        }
    }

    CubeError error = cube.validate();
    if (error.code != CubeErrorCode::None) {
        return {.cube = CubieCube::solved(), .error = error};
    }
    return {.cube = cube, .error = {}};
}

Cube CubieCube::toCube() const
{
    Cube::Stickers stickers = Cube::solved().stickers();

    for (int position = 0; position < corner_count; ++position) {
        const auto cubie = cornerPermutation[position];
        const auto orientation = cornerOrientation[position];
        const auto& facelets = corner_facelets[position];
        const auto& colors = corner_colors[cubie];

        stickers[facelets[orientation]] = colors[0];
        stickers[facelets[(orientation + 1) % 3]] = colors[1];
        stickers[facelets[(orientation + 2) % 3]] = colors[2];
    }

    for (int position = 0; position < edge_count; ++position) {
        const auto cubie = edgePermutation[position];
        const auto orientation = edgeOrientation[position];
        const auto& facelets = edge_facelets[position];
        const auto& colors = edge_colors[cubie];

        stickers[facelets[orientation]] = colors[0];
        stickers[facelets[(orientation + 1) % 2]] = colors[1];
    }

    return Cube::fromUncheckedStickers(stickers);
}

void CubieCube::apply(Move move)
{
    const CubieCube& transform = moveTransform(move);
    CubieCube next;

    for (int position = 0; position < corner_count; ++position) {
        const auto source = transform.cornerPermutation[position];
        next.cornerPermutation[position] = cornerPermutation[source];
        next.cornerOrientation[position] = static_cast<std::uint8_t>(
            (cornerOrientation[source] + transform.cornerOrientation[position]) % 3);
    }

    for (int position = 0; position < edge_count; ++position) {
        const auto source = transform.edgePermutation[position];
        next.edgePermutation[position] = edgePermutation[source];
        next.edgeOrientation[position] = static_cast<std::uint8_t>(
            (edgeOrientation[source] + transform.edgeOrientation[position]) % 2);
    }

    *this = next;
}

void CubieCube::apply(const std::vector<Move>& moves)
{
    for (Move move : moves) {
        apply(move);
    }
}

CubieCube CubieCube::moved(Move move) const
{
    CubieCube cube = *this;
    cube.apply(move);
    return cube;
}

bool CubieCube::isSolved() const
{
    return validate().code == CubeErrorCode::None && *this == solved();
}

CubeError CubieCube::validate() const
{
    if (!isPermutation(cornerPermutation)) {
        return {
            .code = CubeErrorCode::InvalidCornerPermutation,
            .message = "corner permutation must contain each corner exactly once",
        };
    }
    if (!isPermutation(edgePermutation)) {
        return {
            .code = CubeErrorCode::InvalidEdgePermutation,
            .message = "edge permutation must contain each edge exactly once",
        };
    }

    int cornerTwist = 0;
    for (std::uint8_t orientation : cornerOrientation) {
        if (orientation > 2) {
            return {
                .code = CubeErrorCode::InvalidCornerOrientation,
                .message = "corner orientation values must be 0, 1, or 2",
            };
        }
        cornerTwist += orientation;
    }
    if (cornerTwist % 3 != 0) {
        return {
            .code = CubeErrorCode::InvalidCornerOrientation,
            .message = "corner orientation sum must be divisible by 3",
        };
    }

    int edgeFlip = 0;
    for (std::uint8_t orientation : edgeOrientation) {
        if (orientation > 1) {
            return {
                .code = CubeErrorCode::InvalidEdgeOrientation,
                .message = "edge orientation values must be 0 or 1",
            };
        }
        edgeFlip += orientation;
    }
    if (edgeFlip % 2 != 0) {
        return {
            .code = CubeErrorCode::InvalidEdgeOrientation,
            .message = "edge orientation sum must be even",
        };
    }

    if (parity(cornerPermutation) != parity(edgePermutation)) {
        return {
            .code = CubeErrorCode::InvalidParity,
            .message = "corner and edge permutation parity must match",
        };
    }

    return {};
}

bool CubieParseResult::ok() const
{
    return error.code == CubeErrorCode::None;
}

CubieParseResult::operator bool() const
{
    return ok();
}

} // namespace rubik
