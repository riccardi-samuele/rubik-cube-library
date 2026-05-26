#include "rubik/pruning_tables.hpp"

#include "rubik/coordinates.hpp"
#include "rubik/move_tables.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <queue>
#include <string>

namespace rubik::pruning_tables {
namespace {

constexpr std::uint8_t unvisited = 0xff;
constexpr std::uint32_t cache_magic = 0x52425054; // RBPT
constexpr std::uint32_t cache_version = 1;

struct CacheHeader {
    std::uint32_t magic = cache_magic;
    std::uint32_t version = cache_version;
    std::uint64_t size = 0;
};

std::filesystem::path cacheDirectoryPath()
{
    if (const char* configured = std::getenv("RUBIK_TABLE_CACHE_DIR")) {
        if (*configured != '\0') {
            return configured;
        }
    }
    return std::filesystem::temp_directory_path() / "rubik_cube_library";
}

std::filesystem::path cachePath(const std::string& name)
{
    return cacheDirectoryPath() / (name + ".rpt");
}

bool loadFromCache(const std::string& name, std::size_t expectedSize, PruningTable& table)
{
    std::ifstream input(cachePath(name), std::ios::binary);
    if (!input) {
        return false;
    }

    CacheHeader header;
    input.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!input ||
        header.magic != cache_magic ||
        header.version != cache_version ||
        header.size != expectedSize) {
        return false;
    }

    PruningTable loaded(expectedSize);
    input.read(reinterpret_cast<char*>(loaded.data()), static_cast<std::streamsize>(loaded.size()));
    if (!input) {
        return false;
    }

    table = std::move(loaded);
    return true;
}

void saveToCache(const std::string& name, const PruningTable& table)
{
    std::error_code error;
    std::filesystem::create_directories(cacheDirectoryPath(), error);
    if (error) {
        return;
    }

    const std::filesystem::path path = cachePath(name);
    const std::filesystem::path temporary = path.string() + ".tmp";

    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    if (!output) {
        return;
    }

    const CacheHeader header{
        .magic = cache_magic,
        .version = cache_version,
        .size = static_cast<std::uint64_t>(table.size()),
    };

    output.write(reinterpret_cast<const char*>(&header), sizeof(header));
    output.write(reinterpret_cast<const char*>(table.data()), static_cast<std::streamsize>(table.size()));
    output.close();

    if (!output) {
        std::filesystem::remove(temporary, error);
        return;
    }

    std::filesystem::rename(temporary, path, error);
    if (error) {
        std::filesystem::remove(path, error);
        error.clear();
        std::filesystem::rename(temporary, path, error);
    }
}

PruningTable buildPruningTable(const move_tables::CoordinateMoveTable& moves)
{
    PruningTable table(moves.size(), unvisited);
    std::queue<std::uint32_t> frontier;

    table[0] = 0;
    frontier.push(0);

    while (!frontier.empty()) {
        const std::uint32_t state = frontier.front();
        frontier.pop();
        const std::uint8_t nextDepth = static_cast<std::uint8_t>(table[state] + 1);

        for (std::uint32_t next : moves[state]) {
            if (table[next] == unvisited) {
                table[next] = nextDepth;
                frontier.push(next);
            }
        }
    }

    return table;
}

PruningTable buildCombinedPruningTable(
    const move_tables::CoordinateMoveTable& firstMoves,
    const move_tables::CoordinateMoveTable& secondMoves)
{
    const std::uint32_t secondSize = static_cast<std::uint32_t>(secondMoves.size());
    PruningTable table(firstMoves.size() * secondMoves.size(), unvisited);
    std::queue<std::uint32_t> frontier;

    table[0] = 0;
    frontier.push(0);

    while (!frontier.empty()) {
        const std::uint32_t state = frontier.front();
        frontier.pop();

        const std::uint32_t first = state / secondSize;
        const std::uint32_t second = state % secondSize;
        const std::uint8_t nextDepth = static_cast<std::uint8_t>(table[state] + 1);

        for (int move = 0; move < move_count; ++move) {
            const std::uint32_t next =
                firstMoves[first][move] * secondSize + secondMoves[second][move];
            if (table[next] == unvisited) {
                table[next] = nextDepth;
                frontier.push(next);
            }
        }
    }

    return table;
}

bool isPhase2MoveIndex(int move)
{
    switch (static_cast<Move>(move)) {
    case Move::U:
    case Move::U2:
    case Move::Up:
    case Move::D:
    case Move::D2:
    case Move::Dp:
    case Move::R2:
    case Move::F2:
    case Move::L2:
    case Move::B2:
        return true;
    case Move::R:
    case Move::Rp:
    case Move::F:
    case Move::Fp:
    case Move::L:
    case Move::Lp:
    case Move::B:
    case Move::Bp:
        return false;
    }
    return false;
}

PruningTable buildPhase2CombinedPruningTable(
    const move_tables::CoordinateMoveTable& firstMoves,
    const move_tables::CoordinateMoveTable& secondMoves)
{
    const std::uint32_t secondSize = static_cast<std::uint32_t>(secondMoves.size());
    PruningTable table(firstMoves.size() * secondMoves.size(), unvisited);
    std::queue<std::uint32_t> frontier;

    table[0] = 0;
    frontier.push(0);

    while (!frontier.empty()) {
        const std::uint32_t state = frontier.front();
        frontier.pop();

        const std::uint32_t first = state / secondSize;
        const std::uint32_t second = state % secondSize;
        const std::uint8_t nextDepth = static_cast<std::uint8_t>(table[state] + 1);

        for (int move = 0; move < move_count; ++move) {
            if (!isPhase2MoveIndex(move)) {
                continue;
            }

            const std::uint32_t next =
                firstMoves[first][move] * secondSize + secondMoves[second][move];
            if (table[next] == unvisited) {
                table[next] = nextDepth;
                frontier.push(next);
            }
        }
    }

    return table;
}

template <typename Builder>
PruningTable cachedPruningTable(const std::string& name, std::size_t expectedSize, Builder builder)
{
    PruningTable table;
    if (loadFromCache(name, expectedSize, table)) {
        return table;
    }

    table = builder();
    saveToCache(name, table);
    return table;
}

} // namespace

std::string cacheDirectory()
{
    return cacheDirectoryPath().string();
}

const PruningTable& cornerOrientation()
{
    static const PruningTable table = cachedPruningTable(
        "corner_orientation",
        coordinates::corner_orientation_count,
        [] { return buildPruningTable(move_tables::cornerOrientation()); });
    return table;
}

const PruningTable& edgeOrientation()
{
    static const PruningTable table = cachedPruningTable(
        "edge_orientation",
        coordinates::edge_orientation_count,
        [] { return buildPruningTable(move_tables::edgeOrientation()); });
    return table;
}

const PruningTable& sliceEdges()
{
    static const PruningTable table = cachedPruningTable(
        "slice_edges",
        coordinates::slice_edge_count,
        [] { return buildPruningTable(move_tables::sliceEdges()); });
    return table;
}

const PruningTable& cornerPermutation()
{
    static const PruningTable table = cachedPruningTable(
        "corner_permutation",
        coordinates::corner_permutation_count,
        [] { return buildPruningTable(move_tables::cornerPermutation()); });
    return table;
}

const PruningTable& upEdgePermutation()
{
    static const PruningTable table = cachedPruningTable(
        "up_edge_permutation",
        coordinates::edge_group_permutation_count,
        [] { return buildPruningTable(move_tables::upEdgePermutation()); });
    return table;
}

const PruningTable& downEdgePermutation()
{
    static const PruningTable table = cachedPruningTable(
        "down_edge_permutation",
        coordinates::edge_group_permutation_count,
        [] { return buildPruningTable(move_tables::downEdgePermutation()); });
    return table;
}

const PruningTable& cornerOrientationSlice()
{
    static const PruningTable table = cachedPruningTable(
        "corner_orientation_slice",
        coordinates::corner_orientation_count * coordinates::slice_edge_count,
        [] {
            return buildCombinedPruningTable(
                move_tables::cornerOrientation(),
                move_tables::sliceEdges());
        });
    return table;
}

const PruningTable& edgeOrientationSlice()
{
    static const PruningTable table = cachedPruningTable(
        "edge_orientation_slice",
        coordinates::edge_orientation_count * coordinates::slice_edge_count,
        [] {
            return buildCombinedPruningTable(
                move_tables::edgeOrientation(),
                move_tables::sliceEdges());
        });
    return table;
}

const PruningTable& cornerOrientationPermutation()
{
    static const PruningTable table = cachedPruningTable(
        "corner_orientation_permutation",
        coordinates::corner_orientation_count * coordinates::corner_permutation_count,
        [] {
            return buildCombinedPruningTable(
                move_tables::cornerOrientation(),
                move_tables::cornerPermutation());
        });
    return table;
}

const PruningTable& cornerPermutationSlice()
{
    static const PruningTable table = cachedPruningTable(
        "corner_permutation_slice",
        coordinates::corner_permutation_count * coordinates::slice_edge_count,
        [] {
            return buildCombinedPruningTable(
                move_tables::cornerPermutation(),
                move_tables::sliceEdges());
        });
    return table;
}

const PruningTable& cornerOrientationUpEdgePermutation()
{
    static const PruningTable table = cachedPruningTable(
        "corner_orientation_up_edge_permutation",
        coordinates::corner_orientation_count * coordinates::edge_group_permutation_count,
        [] {
            return buildCombinedPruningTable(
                move_tables::cornerOrientation(),
                move_tables::upEdgePermutation());
        });
    return table;
}

const PruningTable& cornerOrientationDownEdgePermutation()
{
    static const PruningTable table = cachedPruningTable(
        "corner_orientation_down_edge_permutation",
        coordinates::corner_orientation_count * coordinates::edge_group_permutation_count,
        [] {
            return buildCombinedPruningTable(
                move_tables::cornerOrientation(),
                move_tables::downEdgePermutation());
        });
    return table;
}

const PruningTable& cornerPermutationUpEdgePermutation()
{
    static const PruningTable table = cachedPruningTable(
        "corner_permutation_up_edge_permutation",
        coordinates::corner_permutation_count * coordinates::edge_group_permutation_count,
        [] {
            return buildCombinedPruningTable(
                move_tables::cornerPermutation(),
                move_tables::upEdgePermutation());
        });
    return table;
}

const PruningTable& cornerPermutationDownEdgePermutation()
{
    static const PruningTable table = cachedPruningTable(
        "corner_permutation_down_edge_permutation",
        coordinates::corner_permutation_count * coordinates::edge_group_permutation_count,
        [] {
            return buildCombinedPruningTable(
                move_tables::cornerPermutation(),
                move_tables::downEdgePermutation());
        });
    return table;
}

const PruningTable& edgeOrientationUpEdgePermutation()
{
    static const PruningTable table = cachedPruningTable(
        "edge_orientation_up_edge_permutation",
        coordinates::edge_orientation_count * coordinates::edge_group_permutation_count,
        [] {
            return buildCombinedPruningTable(
                move_tables::edgeOrientation(),
                move_tables::upEdgePermutation());
        });
    return table;
}

const PruningTable& edgeOrientationDownEdgePermutation()
{
    static const PruningTable table = cachedPruningTable(
        "edge_orientation_down_edge_permutation",
        coordinates::edge_orientation_count * coordinates::edge_group_permutation_count,
        [] {
            return buildCombinedPruningTable(
                move_tables::edgeOrientation(),
                move_tables::downEdgePermutation());
        });
    return table;
}

const PruningTable& upDownEdgePermutation()
{
    static const PruningTable table = cachedPruningTable(
        "up_down_edge_permutation",
        coordinates::edge_group_permutation_count * coordinates::edge_group_permutation_count,
        [] {
            return buildCombinedPruningTable(
                move_tables::upEdgePermutation(),
                move_tables::downEdgePermutation());
        });
    return table;
}

const PruningTable& cornerPermutationEdgeOrientation()
{
    static const PruningTable table = cachedPruningTable(
        "corner_permutation_edge_orientation",
        coordinates::corner_permutation_count * coordinates::edge_orientation_count,
        [] {
            return buildCombinedPruningTable(
                move_tables::cornerPermutation(),
                move_tables::edgeOrientation());
        });
    return table;
}

const PruningTable& phase2CornerSlicePermutation()
{
    static const PruningTable table = cachedPruningTable(
        "phase2_corner_slice_permutation",
        coordinates::corner_permutation_count * coordinates::slice_edge_permutation_count,
        [] {
            return buildPhase2CombinedPruningTable(
                move_tables::cornerPermutation(),
                move_tables::sliceEdgePermutation());
        });
    return table;
}

const PruningTable& phase2UpEdgeSlicePermutation()
{
    static const PruningTable table = cachedPruningTable(
        "phase2_up_edge_slice_permutation",
        coordinates::edge_group_permutation_count * coordinates::slice_edge_permutation_count,
        [] {
            return buildPhase2CombinedPruningTable(
                move_tables::upEdgePermutation(),
                move_tables::sliceEdgePermutation());
        });
    return table;
}

const PruningTable& phase2DownEdgeSlicePermutation()
{
    static const PruningTable table = cachedPruningTable(
        "phase2_down_edge_slice_permutation",
        coordinates::edge_group_permutation_count * coordinates::slice_edge_permutation_count,
        [] {
            return buildPhase2CombinedPruningTable(
                move_tables::downEdgePermutation(),
                move_tables::sliceEdgePermutation());
        });
    return table;
}

} // namespace rubik::pruning_tables
