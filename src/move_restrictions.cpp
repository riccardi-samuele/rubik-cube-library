#include "rubik/detail/move_restrictions.hpp"

#include <algorithm>
#include <utility>

namespace rubik::detail {
namespace {

bool containsFaceValue(const std::vector<Face>& faces, Face face)
{
    return std::find(faces.begin(), faces.end(), face) != faces.end();
}

} // namespace

bool oppositeFaces(Face first, Face second)
{
    return (first == Face::U && second == Face::D) ||
        (first == Face::D && second == Face::U) ||
        (first == Face::R && second == Face::L) ||
        (first == Face::L && second == Face::R) ||
        (first == Face::F && second == Face::B) ||
        (first == Face::B && second == Face::F);
}

AllowedMovesResult allowedMovesForBlockedFaces(const std::vector<Face>& blockedFaces)
{
    if (blockedFaces.size() > 2) {
        return {.status = SolveStatus::UnsupportedOptions, .moves = {}};
    }

    if (blockedFaces.size() == 2) {
        if (blockedFaces[0] == blockedFaces[1] || !oppositeFaces(blockedFaces[0], blockedFaces[1])) {
            return {.status = SolveStatus::UnsupportedOptions, .moves = {}};
        }
    }

    std::vector<Move> moves;
    moves.reserve(allMoves().size());
    for (Move move : allMoves()) {
        if (!containsFaceValue(blockedFaces, faceOf(move))) {
            moves.push_back(move);
        }
    }

    return {.status = SolveStatus::Found, .moves = std::move(moves)};
}

} // namespace rubik::detail
