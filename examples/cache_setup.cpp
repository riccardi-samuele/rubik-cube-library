#include "rubik/cache.hpp"
#include "rubik/solver.hpp"

#include <iostream>

int main()
{
    rubik::CacheSetupOptions options;
    options.profile = rubik::SolveProfile::Auto;
    options.cachePolicy = rubik::CachePolicy::AllowBuild;
    options.dryRun = true;

    const rubik::CacheSetupResult result = rubik::prepareCache(options);

    std::cout << "ready: " << (result.ready ? "true" : "false") << "\n";
    std::cout << "cache_warm: " << (result.cacheWarm ? "true" : "false") << "\n";
    std::cout << "dry_run: true\n";
    std::cout << "bytes_required: " << result.plan.estimatedTablePayloadBytes << "\n";
    std::cout << "bytes_missing: " << result.bytesMissing << "\n";

    return result.ready ? 0 : 1;
}
