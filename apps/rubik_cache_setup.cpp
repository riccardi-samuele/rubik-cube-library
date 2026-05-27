#include "rubik/cache.hpp"
#include "rubik/version.hpp"

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <string>

namespace {

enum class OutputFormat {
    Text,
    Csv
};

std::optional<rubik::SolveProfile> parseProfile(const std::string& value)
{
    if (value == "auto") {
        return rubik::SolveProfile::Auto;
    }
    if (value == "embedded") {
        return rubik::SolveProfile::Embedded;
    }
    if (value == "default") {
        return rubik::SolveProfile::Default;
    }
    if (value == "performance") {
        return rubik::SolveProfile::Performance;
    }
    if (value == "large-local" || value == "large_local") {
        return rubik::SolveProfile::LargeLocal;
    }
    return std::nullopt;
}

const char* profileName(rubik::SolveProfile profile)
{
    switch (profile) {
    case rubik::SolveProfile::Embedded:
        return "embedded";
    case rubik::SolveProfile::Default:
        return "default";
    case rubik::SolveProfile::Performance:
        return "performance";
    case rubik::SolveProfile::LargeLocal:
        return "large-local";
    case rubik::SolveProfile::Auto:
        return "auto";
    }
    return "unknown";
}

std::optional<unsigned long long> parseUnsigned(const std::string& value)
{
    if (value.empty() || value[0] == '-') {
        return std::nullopt;
    }
    char* end = nullptr;
    errno = 0;
    const unsigned long long parsed = std::strtoull(value.c_str(), &end, 10);
    if (errno != 0 || end == value.c_str() || *end != '\0') {
        return std::nullopt;
    }
    return parsed;
}

void printUsage()
{
    std::cout
        << "rubik-cache-setup " << rubik::version_string << "\n"
        << "Usage: rubik-cache-setup [--profile auto|embedded|default|performance|large-local]\n"
        << "                         [--max-memory-mb N] [--threads N]\n"
        << "                         [--cache-dir PATH] [--dry-run]\n"
        << "                         [--format text|csv]\n";
}

} // namespace

int main(int argc, char** argv)
{
    rubik::CacheSetupOptions options;
    OutputFormat outputFormat = OutputFormat::Text;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        }
        if (arg == "--version" || arg == "-V") {
            std::cout << "rubik-cache-setup " << rubik::version_string << "\n";
            return 0;
        }
        if (arg == "--dry-run") {
            options.dryRun = true;
            continue;
        }
        if (arg == "--profile" && i + 1 < argc) {
            const auto profile = parseProfile(argv[++i]);
            if (!profile) {
                std::cerr << "invalid profile\n";
                return 2;
            }
            options.profile = *profile;
            continue;
        }
        if (arg == "--max-memory-mb" && i + 1 < argc) {
            const auto parsed = parseUnsigned(argv[++i]);
            if (!parsed) {
                std::cerr << "invalid max memory\n";
                return 2;
            }
            options.maxMemoryBytes = static_cast<std::size_t>(*parsed) * 1024ull * 1024ull;
            continue;
        }
        if (arg == "--threads" && i + 1 < argc) {
            const auto parsed = parseUnsigned(argv[++i]);
            if (!parsed || *parsed > std::numeric_limits<unsigned int>::max()) {
                std::cerr << "invalid thread count\n";
                return 2;
            }
            options.threads = static_cast<unsigned int>(*parsed);
            continue;
        }
        if (arg == "--cache-dir" && i + 1 < argc) {
            options.cacheDirectory = argv[++i];
            continue;
        }
        if (arg == "--format" && i + 1 < argc) {
            const std::string value = argv[++i];
            if (value == "text") {
                outputFormat = OutputFormat::Text;
                continue;
            }
            if (value == "csv") {
                outputFormat = OutputFormat::Csv;
                continue;
            }
            std::cerr << "invalid format\n";
            return 2;
        }
        std::cerr << "unknown or incomplete argument: " << arg << "\n";
        return 2;
    }

    const rubik::CacheSetupResult result = rubik::prepareCache(options);
    if (outputFormat == OutputFormat::Csv) {
        std::cout << "cache_setup,status," << (result.ready ? "Ready" : "Failed") << "\n";
        std::cout << "cache_setup,requested_profile," << profileName(result.plan.requestedProfile) << "\n";
        std::cout << "cache_setup,effective_profile," << profileName(result.plan.effectiveProfile) << "\n";
        std::cout << "cache_setup,threads," << result.plan.effectiveThreads << "\n";
        std::cout << "cache_setup,memory_bytes," << result.plan.effectiveMaxMemoryBytes << "\n";
        std::cout << "cache_setup,payload_bytes," << result.plan.estimatedTablePayloadBytes << "\n";
        std::cout << "cache_setup,cache_warm," << (result.cacheWarm ? "true" : "false") << "\n";
        std::cout << "cache_setup,bytes_prepared," << result.bytesPrepared << "\n";
        std::cout << "cache_setup,bytes_missing," << result.bytesMissing << "\n";
        std::cout << "cache_setup,elapsed_ms," << result.elapsed.count() << "\n";
        std::cout << "cache_setup,message," << result.message << "\n";
        return result.ready ? 0 : 1;
    }
    std::cout << "status: " << (result.ready ? "Ready" : "Failed") << "\n";
    std::cout << "requested-profile: " << profileName(result.plan.requestedProfile) << "\n";
    std::cout << "effective-profile: " << profileName(result.plan.effectiveProfile) << "\n";
    std::cout << "threads: " << result.plan.effectiveThreads << "\n";
    std::cout << "memory-bytes: " << result.plan.effectiveMaxMemoryBytes << "\n";
    std::cout << "payload-bytes: " << result.plan.estimatedTablePayloadBytes << "\n";
    std::cout << "cache-warm: " << (result.cacheWarm ? "true" : "false") << "\n";
    std::cout << "bytes-prepared: " << result.bytesPrepared << "\n";
    std::cout << "bytes-missing: " << result.bytesMissing << "\n";
    std::cout << "message: " << result.message << "\n";
    return result.ready ? 0 : 1;
}
