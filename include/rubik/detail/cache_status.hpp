#pragma once

#include "rubik/solver.hpp"

#include <cstddef>
#include <filesystem>

namespace rubik::detail {

std::size_t missingCacheBytes(const std::filesystem::path& directory, SolveProfile profile);
std::size_t missingCacheBytes(SolveProfile profile);
bool profileCacheWarm(SolveProfile profile);

} // namespace rubik::detail
