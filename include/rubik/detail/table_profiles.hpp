#pragma once

#include "rubik/pruning_tables.hpp"
#include "rubik/solver.hpp"

#include <cstddef>
#include <span>

namespace rubik::detail {

struct TableProfileEntry {
    const char* name;
    std::size_t entries;
    const pruning_tables::PruningTable& (*table)();
};

std::span<const TableProfileEntry> optimalTableProfile(SolveProfile profile);
std::span<const TableProfileEntry> phase1BaseTableProfile();
std::span<const TableProfileEntry> phase2TableProfile();
std::size_t tableProfilePayloadBytes(std::span<const TableProfileEntry> profile);
std::size_t optimalTablePayloadBytes(SolveProfile profile);
std::size_t fastTwoPhaseTablePayloadBytes();
std::size_t estimatedSolverTablePayloadBytes(SolveMode mode, SolveProfile profile);

} // namespace rubik::detail
