#pragma once

#include "rubik/solver.hpp"

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <string>

namespace rubik {

struct CacheSetupOptions {
    SolveProfile profile = SolveProfile::Auto;
    CachePolicy cachePolicy = CachePolicy::AllowBuild;
    std::size_t maxMemoryBytes = 0;
    unsigned int threads = 0;
    std::filesystem::path cacheDirectory;
    bool dryRun = false;
};

struct CacheSetupResult {
    bool ready = false;
    SolvePlan plan;
    std::size_t bytesPrepared = 0;
    std::chrono::milliseconds elapsed{0};
    std::string message;
};

CacheSetupResult prepareCache(const CacheSetupOptions& options);

} // namespace rubik
