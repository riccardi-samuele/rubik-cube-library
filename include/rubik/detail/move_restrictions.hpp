#pragma once

#include "rubik/move.hpp"
#include "rubik/solver.hpp"

#include <vector>

namespace rubik::detail {

struct AllowedMovesResult {
    SolveStatus status = SolveStatus::Found;
    std::vector<Move> moves;
};

bool oppositeFaces(Face first, Face second);
AllowedMovesResult allowedMovesForBlockedFaces(const std::vector<Face>& blockedFaces);

} // namespace rubik::detail
