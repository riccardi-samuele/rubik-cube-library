#include "rubik/move.hpp"

#include <sstream>
#include <stdexcept>

namespace rubik {

Face faceOf(Move move)
{
    return static_cast<Face>(static_cast<int>(move) / 3);
}

int quarterTurns(Move move)
{
    switch (static_cast<int>(move) % 3) {
    case 0:
        return 1;
    case 1:
        return 2;
    default:
        return 3;
    }
}

Move inverse(Move move)
{
    const int base = (static_cast<int>(move) / 3) * 3;
    switch (static_cast<int>(move) % 3) {
    case 0:
        return static_cast<Move>(base + 2);
    case 1:
        return move;
    default:
        return static_cast<Move>(base);
    }
}

std::string toString(Move move)
{
    static constexpr char faces[] = {'U', 'R', 'F', 'D', 'L', 'B'};
    std::string text(1, faces[static_cast<int>(faceOf(move))]);
    if (static_cast<int>(move) % 3 == 1) {
        text += '2';
    } else if (static_cast<int>(move) % 3 == 2) {
        text += '\'';
    }
    return text;
}

std::string toString(const std::vector<Move>& moves)
{
    std::string text;
    for (std::size_t i = 0; i < moves.size(); ++i) {
        if (i != 0) {
            text += ' ';
        }
        text += toString(moves[i]);
    }
    return text;
}

std::string formatMoves(const std::vector<Move>& moves)
{
    return toString(moves);
}

std::optional<Move> parseMove(std::string_view token)
{
    if (token.empty() || token.size() > 2) {
        return std::nullopt;
    }

    int face = -1;
    switch (token[0]) {
    case 'U':
        face = 0;
        break;
    case 'R':
        face = 1;
        break;
    case 'F':
        face = 2;
        break;
    case 'D':
        face = 3;
        break;
    case 'L':
        face = 4;
        break;
    case 'B':
        face = 5;
        break;
    default:
        return std::nullopt;
    }

    int suffix = 0;
    if (token.size() == 2) {
        if (token[1] == '2') {
            suffix = 1;
        } else if (token[1] == '\'') {
            suffix = 2;
        } else {
            return std::nullopt;
        }
    }

    return static_cast<Move>(face * 3 + suffix);
}

std::vector<Move> parseMoves(std::string_view text)
{
    std::istringstream input(std::string{text});
    std::string token;
    std::vector<Move> moves;
    while (input >> token) {
        auto move = parseMove(token);
        if (!move) {
            throw std::invalid_argument("invalid move token: " + token);
        }
        moves.push_back(*move);
    }
    return moves;
}

const std::vector<Move>& allMoves()
{
    static const std::vector<Move> moves = {
        Move::U, Move::U2, Move::Up,
        Move::R, Move::R2, Move::Rp,
        Move::F, Move::F2, Move::Fp,
        Move::D, Move::D2, Move::Dp,
        Move::L, Move::L2, Move::Lp,
        Move::B, Move::B2, Move::Bp,
    };
    return moves;
}

} // namespace rubik
