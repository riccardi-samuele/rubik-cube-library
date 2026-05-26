#include "rubik/detail/table_profiles.hpp"

#include "rubik/coordinates.hpp"

#include <array>

namespace rubik::detail {
namespace {

using pruning_tables::cornerOrientation;
using pruning_tables::cornerOrientationDownEdgePermutation;
using pruning_tables::cornerOrientationSlice;
using pruning_tables::cornerOrientationUpEdgePermutation;
using pruning_tables::cornerPermutation;
using pruning_tables::cornerPermutationEdgeOrientation;
using pruning_tables::cornerPermutationSlice;
using pruning_tables::downEdgePermutation;
using pruning_tables::edgeOrientation;
using pruning_tables::edgeOrientationDownEdgePermutation;
using pruning_tables::edgeOrientationSlice;
using pruning_tables::edgeOrientationUpEdgePermutation;
using pruning_tables::phase2CornerSlicePermutation;
using pruning_tables::phase2DownEdgeSlicePermutation;
using pruning_tables::phase2UpEdgeSlicePermutation;
using pruning_tables::sliceEdges;
using pruning_tables::upDownEdgePermutation;
using pruning_tables::upEdgePermutation;

constexpr std::array embeddedOptimalTables{
    TableProfileEntry{"corner_orientation", coordinates::corner_orientation_count, cornerOrientation},
    TableProfileEntry{"edge_orientation", coordinates::edge_orientation_count, edgeOrientation},
    TableProfileEntry{"slice_edges", coordinates::slice_edge_count, sliceEdges},
    TableProfileEntry{"corner_permutation", coordinates::corner_permutation_count, cornerPermutation},
    TableProfileEntry{"up_edge_permutation", coordinates::edge_group_permutation_count, upEdgePermutation},
    TableProfileEntry{"down_edge_permutation", coordinates::edge_group_permutation_count, downEdgePermutation},
    TableProfileEntry{
        "corner_orientation_slice",
        coordinates::corner_orientation_count * coordinates::slice_edge_count,
        cornerOrientationSlice},
    TableProfileEntry{
        "edge_orientation_slice",
        coordinates::edge_orientation_count * coordinates::slice_edge_count,
        edgeOrientationSlice},
    TableProfileEntry{
        "corner_permutation_slice",
        coordinates::corner_permutation_count * coordinates::slice_edge_count,
        cornerPermutationSlice},
};

constexpr std::array defaultOptimalTables{
    TableProfileEntry{"corner_orientation", coordinates::corner_orientation_count, cornerOrientation},
    TableProfileEntry{"edge_orientation", coordinates::edge_orientation_count, edgeOrientation},
    TableProfileEntry{"slice_edges", coordinates::slice_edge_count, sliceEdges},
    TableProfileEntry{"corner_permutation", coordinates::corner_permutation_count, cornerPermutation},
    TableProfileEntry{"up_edge_permutation", coordinates::edge_group_permutation_count, upEdgePermutation},
    TableProfileEntry{"down_edge_permutation", coordinates::edge_group_permutation_count, downEdgePermutation},
    TableProfileEntry{
        "corner_orientation_slice",
        coordinates::corner_orientation_count * coordinates::slice_edge_count,
        cornerOrientationSlice},
    TableProfileEntry{
        "edge_orientation_slice",
        coordinates::edge_orientation_count * coordinates::slice_edge_count,
        edgeOrientationSlice},
    TableProfileEntry{
        "corner_permutation_slice",
        coordinates::corner_permutation_count * coordinates::slice_edge_count,
        cornerPermutationSlice},
    TableProfileEntry{
        "corner_permutation_edge_orientation",
        coordinates::corner_permutation_count * coordinates::edge_orientation_count,
        cornerPermutationEdgeOrientation},
    TableProfileEntry{
        "corner_orientation_up_edge_permutation",
        coordinates::corner_orientation_count * coordinates::edge_group_permutation_count,
        cornerOrientationUpEdgePermutation},
    TableProfileEntry{
        "corner_orientation_down_edge_permutation",
        coordinates::corner_orientation_count * coordinates::edge_group_permutation_count,
        cornerOrientationDownEdgePermutation},
    TableProfileEntry{
        "edge_orientation_up_edge_permutation",
        coordinates::edge_orientation_count * coordinates::edge_group_permutation_count,
        edgeOrientationUpEdgePermutation},
    TableProfileEntry{
        "edge_orientation_down_edge_permutation",
        coordinates::edge_orientation_count * coordinates::edge_group_permutation_count,
        edgeOrientationDownEdgePermutation},
};

constexpr std::array performanceOptimalTables{
    TableProfileEntry{"corner_orientation", coordinates::corner_orientation_count, cornerOrientation},
    TableProfileEntry{"edge_orientation", coordinates::edge_orientation_count, edgeOrientation},
    TableProfileEntry{"slice_edges", coordinates::slice_edge_count, sliceEdges},
    TableProfileEntry{"corner_permutation", coordinates::corner_permutation_count, cornerPermutation},
    TableProfileEntry{"up_edge_permutation", coordinates::edge_group_permutation_count, upEdgePermutation},
    TableProfileEntry{"down_edge_permutation", coordinates::edge_group_permutation_count, downEdgePermutation},
    TableProfileEntry{
        "corner_orientation_slice",
        coordinates::corner_orientation_count * coordinates::slice_edge_count,
        cornerOrientationSlice},
    TableProfileEntry{
        "edge_orientation_slice",
        coordinates::edge_orientation_count * coordinates::slice_edge_count,
        edgeOrientationSlice},
    TableProfileEntry{
        "corner_permutation_slice",
        coordinates::corner_permutation_count * coordinates::slice_edge_count,
        cornerPermutationSlice},
    TableProfileEntry{
        "corner_permutation_edge_orientation",
        coordinates::corner_permutation_count * coordinates::edge_orientation_count,
        cornerPermutationEdgeOrientation},
    TableProfileEntry{
        "corner_orientation_up_edge_permutation",
        coordinates::corner_orientation_count * coordinates::edge_group_permutation_count,
        cornerOrientationUpEdgePermutation},
    TableProfileEntry{
        "corner_orientation_down_edge_permutation",
        coordinates::corner_orientation_count * coordinates::edge_group_permutation_count,
        cornerOrientationDownEdgePermutation},
    TableProfileEntry{
        "edge_orientation_up_edge_permutation",
        coordinates::edge_orientation_count * coordinates::edge_group_permutation_count,
        edgeOrientationUpEdgePermutation},
    TableProfileEntry{
        "edge_orientation_down_edge_permutation",
        coordinates::edge_orientation_count * coordinates::edge_group_permutation_count,
        edgeOrientationDownEdgePermutation},
    TableProfileEntry{
        "up_down_edge_permutation",
        coordinates::edge_group_permutation_count * coordinates::edge_group_permutation_count,
        upDownEdgePermutation},
};

constexpr std::array phase1BaseTables{
    TableProfileEntry{"corner_orientation", coordinates::corner_orientation_count, cornerOrientation},
    TableProfileEntry{"edge_orientation", coordinates::edge_orientation_count, edgeOrientation},
    TableProfileEntry{"slice_edges", coordinates::slice_edge_count, sliceEdges},
    TableProfileEntry{
        "corner_orientation_slice",
        coordinates::corner_orientation_count * coordinates::slice_edge_count,
        cornerOrientationSlice},
    TableProfileEntry{
        "edge_orientation_slice",
        coordinates::edge_orientation_count * coordinates::slice_edge_count,
        edgeOrientationSlice},
};

constexpr std::array phase2Tables{
    TableProfileEntry{
        "phase2_corner_slice_permutation",
        coordinates::corner_permutation_count * coordinates::slice_edge_permutation_count,
        phase2CornerSlicePermutation},
    TableProfileEntry{
        "phase2_up_edge_slice_permutation",
        coordinates::edge_group_permutation_count * coordinates::slice_edge_permutation_count,
        phase2UpEdgeSlicePermutation},
    TableProfileEntry{
        "phase2_down_edge_slice_permutation",
        coordinates::edge_group_permutation_count * coordinates::slice_edge_permutation_count,
        phase2DownEdgeSlicePermutation},
};

} // namespace

std::span<const TableProfileEntry> optimalTableProfile(SolveProfile profile)
{
    switch (profile) {
    case SolveProfile::Embedded:
        return embeddedOptimalTables;
    case SolveProfile::Performance:
        return performanceOptimalTables;
    case SolveProfile::Default:
        return defaultOptimalTables;
    }
    return defaultOptimalTables;
}

std::span<const TableProfileEntry> phase1BaseTableProfile()
{
    return phase1BaseTables;
}

std::span<const TableProfileEntry> phase2TableProfile()
{
    return phase2Tables;
}

std::size_t tableProfilePayloadBytes(std::span<const TableProfileEntry> profile)
{
    std::size_t bytes = 0;
    for (const TableProfileEntry& entry : profile) {
        bytes += entry.entries;
    }
    return bytes;
}

std::size_t optimalTablePayloadBytes(SolveProfile profile)
{
    return tableProfilePayloadBytes(optimalTableProfile(profile));
}

std::size_t fastTwoPhaseTablePayloadBytes()
{
    return tableProfilePayloadBytes(phase1BaseTableProfile()) +
        tableProfilePayloadBytes(phase2TableProfile());
}

std::size_t estimatedSolverTablePayloadBytes(SolveMode mode, SolveProfile profile)
{
    if (mode == SolveMode::Optimal) {
        return optimalTablePayloadBytes(profile);
    }
    if (mode == SolveMode::Fast) {
        return optimalTablePayloadBytes(profile) + tableProfilePayloadBytes(phase2TableProfile());
    }
    return 0;
}

} // namespace rubik::detail
