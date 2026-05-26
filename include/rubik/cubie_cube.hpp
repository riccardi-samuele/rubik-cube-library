#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "rubik/cube.hpp"
#include "rubik/errors.hpp"

namespace rubik {

enum class Corner : std::uint8_t {
    URF,
    UFL,
    ULB,
    UBR,
    DFR,
    DLF,
    DBL,
    DRB
};

enum class Edge : std::uint8_t {
    UR,
    UF,
    UL,
    UB,
    DR,
    DF,
    DL,
    DB,
    FR,
    FL,
    BL,
    BR
};

class CubieCube;
struct CubieParseResult;

class CubieCube {
public:
    static constexpr int corner_count = 8;
    static constexpr int edge_count = 12;

    using CornerArray = std::array<std::uint8_t, corner_count>;
    using EdgeArray = std::array<std::uint8_t, edge_count>;

    CornerArray cornerPermutation{};
    CornerArray cornerOrientation{};
    EdgeArray edgePermutation{};
    EdgeArray edgeOrientation{};

    static CubieCube solved();
    static CubieParseResult fromCube(const Cube& cube);
    static CubieParseResult fromStickers(const Cube::Stickers& stickers);

    Cube toCube() const;
    void apply(Move move);
    void apply(const std::vector<Move>& moves);
    CubieCube moved(Move move) const;

    bool isSolved() const;
    CubeError validate() const;

    friend bool operator==(const CubieCube& lhs, const CubieCube& rhs) = default;
};

struct CubieParseResult {
    CubieCube cube = CubieCube::solved();
    CubeError error;

    bool ok() const;
    explicit operator bool() const;
};

} // namespace rubik
