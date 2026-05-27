#include "rubik/detail/cache_status.hpp"

#include "rubik/coordinates.hpp"
#include "rubik/detail/table_profiles.hpp"
#include "rubik/pruning_tables.hpp"

namespace rubik::detail {

std::size_t missingCacheBytes(const std::filesystem::path& directory, SolveProfile profile)
{
    std::size_t missing = 0;
    for (const TableProfileEntry& entry : optimalTableProfile(profile)) {
        if (!pruning_tables::cacheEntryReady(directory, entry.name, entry.entries)) {
            missing += entry.entries;
        }
    }

    const std::size_t cornerStateEntries =
        static_cast<std::size_t>(coordinates::corner_orientation_count) *
        static_cast<std::size_t>(coordinates::corner_permutation_count);
    if (!pruning_tables::cacheEntryReady(
            directory,
            "corner_orientation_permutation",
            cornerStateEntries)) {
        missing += cornerStateEntries;
    }

    if (profile == SolveProfile::LargeLocal) {
        const std::size_t cornerEdgeGroupEntries =
            static_cast<std::size_t>(coordinates::corner_permutation_count) *
            static_cast<std::size_t>(coordinates::edge_group_permutation_count);
        if (!pruning_tables::cacheEntryReady(
                directory,
                "corner_permutation_up_edge_permutation",
                cornerEdgeGroupEntries)) {
            missing += cornerEdgeGroupEntries;
        }
        if (!pruning_tables::cacheEntryReady(
                directory,
                "corner_permutation_down_edge_permutation",
                cornerEdgeGroupEntries)) {
            missing += cornerEdgeGroupEntries;
        }
    }

    return missing;
}

std::size_t missingCacheBytes(SolveProfile profile)
{
    return missingCacheBytes(pruning_tables::cacheDirectory(), profile);
}

bool profileCacheWarm(SolveProfile profile)
{
    return missingCacheBytes(profile) == 0;
}

} // namespace rubik::detail
