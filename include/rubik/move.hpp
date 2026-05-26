#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace rubik {

enum class Face : std::uint8_t {
    U,
    R,
    F,
    D,
    L,
    B
};

enum class Move : std::uint8_t {
    U,
    U2,
    Up,
    R,
    R2,
    Rp,
    F,
    F2,
    Fp,
    D,
    D2,
    Dp,
    L,
    L2,
    Lp,
    B,
    B2,
    Bp
};

constexpr int move_count = 18;

Face faceOf(Move move);
int quarterTurns(Move move);
Move inverse(Move move);

std::string toString(Move move);
std::string toString(const std::vector<Move>& moves);
std::string formatMoves(const std::vector<Move>& moves);
std::optional<Move> parseMove(std::string_view token);
std::vector<Move> parseMoves(std::string_view text);

const std::vector<Move>& allMoves();

} // namespace rubik
