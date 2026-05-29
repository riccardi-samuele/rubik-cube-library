#include "rubik/coordinates.hpp"
#include "rubik/detail/symmetry_coordinates.hpp"
#include "rubik/detail/symmetry_pruning.hpp"
#include "rubik/detail/symmetry.hpp"
#include "rubik/detail/adaptive_scheduler.hpp"
#include "rubik/detail/auto_plan.hpp"
#include "rubik/detail/optimal_plan.hpp"
#include "rubik/detail/table_profiles.hpp"
#include "rubik/experimental/phase1.hpp"
#include "rubik/experimental/phase2.hpp"
#include "rubik/move_tables.hpp"
#include "rubik/phase1.hpp"
#include "rubik/phase2.hpp"
#include "rubik/pruning_tables.hpp"
#include "rubik/cache.hpp"
#include "rubik/solver.hpp"
#include "rubik/version.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <queue>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

void expect(bool condition)
{
    if (!condition) {
        std::cerr << "test expectation failed\n";
        std::abort();
    }
}

void testVersionMetadata()
{
    assert(rubik::version_major == 5);
    assert(rubik::version_minor == 0);
    assert(rubik::version_patch == 0);
    assert(std::string(rubik::version_string) == "5.0.0");
}

void testV3AdaptiveApiDefaults()
{
    rubik::SolveOptions options;
    expect(options.profile == rubik::SolveProfile::Default);
    expect(options.cachePolicy == rubik::CachePolicy::Auto);

    rubik::SolvePlan plan;
    expect(plan.requestedProfile == rubik::SolveProfile::Default);
    expect(plan.effectiveProfile == rubik::SolveProfile::Default);
    expect(plan.cachePolicy == rubik::CachePolicy::Auto);
    expect(plan.requestedThreads == 1);
    expect(plan.effectiveThreads == 1);
    expect(plan.requestedMaxMemoryBytes == 0);
    expect(plan.effectiveMaxMemoryBytes == 0);
    expect(plan.estimatedTablePayloadBytes == 0);
    expect(!plan.diskCacheEnabled);
    expect(!plan.diskCacheWarm);
    expect(!plan.builtCacheDuringSolve);
    expect(plan.strategyName.empty());
    expect(plan.optimalMoveOrdering == "base_bound");
    expect(plan.rootOrderingProfile.empty());
    expect(plan.boundsUsed.empty());

    rubik::SolveResult result;
    expect(result.plan.cachePolicy == rubik::CachePolicy::Auto);
}

void testAutoPlannerRejectsFastMode()
{
    rubik::SolveOptions options;
    options.mode = rubik::SolveMode::Fast;
    options.profile = rubik::SolveProfile::Auto;

    const auto plan = rubik::detail::makeOptimalPlan(options);
    expect(!plan.supported);
    expect(plan.status == rubik::SolveStatus::UnsupportedOptions);
}

void testAutoPlannerInternalDecisionMatchesPublicPlan()
{
    rubik::SolveOptions options;
    options.mode = rubik::SolveMode::Optimal;
    options.metric = rubik::Metric::HTM;
    options.profile = rubik::SolveProfile::Auto;
    options.maxDepth = 13;
    options.maxMemoryBytes = 0;
    options.threads = 0;

    const auto decision = rubik::detail::makeAutoPlan(options);
    const auto plan = rubik::detail::makeOptimalPlan(options);

    expect(decision.supported);
    expect(decision.effectiveProfile == rubik::SolveProfile::Performance);
    expect(std::string(decision.strategyName) == "auto_shallow_optimal");
    expect(plan.publicPlan.effectiveProfile == decision.effectiveProfile);
    expect(plan.publicPlan.strategyName == decision.strategyName);
    expect(plan.publicPlan.effectiveThreads == decision.effectiveThreads);
    expect(plan.publicPlan.effectiveMaxMemoryBytes == decision.effectiveMaxMemoryBytes);
}

void testAutoPlannerUsesPerformanceForShallowOptimal()
{
    rubik::SolveOptions options;
    options.mode = rubik::SolveMode::Optimal;
    options.metric = rubik::Metric::HTM;
    options.profile = rubik::SolveProfile::Auto;
    options.maxDepth = 13;
    options.maxMemoryBytes = 0;
    options.threads = 0;

    const auto plan = rubik::detail::makeOptimalPlan(options);
    expect(plan.supported);
    expect(plan.publicPlan.requestedProfile == rubik::SolveProfile::Auto);
    expect(plan.publicPlan.effectiveProfile == rubik::SolveProfile::Performance);
    expect(plan.publicPlan.effectiveMaxMemoryBytes == 2ull * 1024ull * 1024ull * 1024ull);
    expect(plan.publicPlan.effectiveThreads >= 1);
    expect(plan.publicPlan.effectiveThreads <= 16);
    expect(plan.publicPlan.strategyName == "auto_shallow_optimal");
    expect(plan.publicPlan.optimalMoveOrdering == "base_bound");
    expect(!plan.publicPlan.boundsUsed.empty());
}

void testAutoPlannerUsesLargeLocalForDeepOptimal()
{
    rubik::SolveOptions options;
    options.mode = rubik::SolveMode::Optimal;
    options.metric = rubik::Metric::HTM;
    options.profile = rubik::SolveProfile::Auto;
    options.maxDepth = 15;
    options.maxMemoryBytes = 0;
    options.threads = 0;

    const auto plan = rubik::detail::makeOptimalPlan(options);
    expect(plan.supported);
    expect(plan.publicPlan.requestedProfile == rubik::SolveProfile::Auto);
    expect(plan.publicPlan.effectiveProfile == rubik::SolveProfile::LargeLocal);
    expect(plan.publicPlan.strategyName == "auto_desktop_tail");
}

void testAutoPlannerFallsBackWhenLargeLocalMemoryIsTooSmall()
{
    rubik::SolveOptions options;
    options.mode = rubik::SolveMode::Optimal;
    options.metric = rubik::Metric::HTM;
    options.profile = rubik::SolveProfile::Auto;
    options.maxDepth = 15;
    options.maxMemoryBytes = 700ull * 1024ull * 1024ull;
    options.threads = 0;

    const auto plan = rubik::detail::makeOptimalPlan(options);
    expect(plan.supported);
    expect(plan.publicPlan.effectiveProfile == rubik::SolveProfile::Performance);
    expect(plan.publicPlan.strategyName == "auto_memory_fallback");
    expect(plan.publicPlan.estimatedTablePayloadBytes < 700ull * 1024ull * 1024ull);
}

void testAutoPlannerRejectsWhenMemoryIsTooSmallForAuto()
{
    rubik::SolveOptions options;
    options.mode = rubik::SolveMode::Optimal;
    options.metric = rubik::Metric::HTM;
    options.profile = rubik::SolveProfile::Auto;
    options.maxDepth = 15;
    options.maxMemoryBytes = 128ull * 1024ull * 1024ull;
    options.threads = 0;

    const auto plan = rubik::detail::makeOptimalPlan(options);
    expect(!plan.supported);
    expect(plan.status == rubik::SolveStatus::MemoryLimitExceeded);
}

void testAutoPlannerReportsColdLargeLocalRequirement()
{
    rubik::SolveOptions options;
    options.mode = rubik::SolveMode::Optimal;
    options.metric = rubik::Metric::HTM;
    options.profile = rubik::SolveProfile::Auto;
    options.maxDepth = 15;
    options.cachePolicy = rubik::CachePolicy::RequireWarm;
    options.maxMemoryBytes = 0;
    options.threads = 0;

    const auto decision = rubik::detail::makeAutoPlan(options, false);
    expect(!decision.supported);
    expect(decision.status == rubik::SolveStatus::CacheNotReady);
}

void testAutoOptimalPlanUsesRealCacheWarmth()
{
    const std::filesystem::path cacheDirectory =
        std::filesystem::temp_directory_path() / "rubik_auto_plan_uses_real_cache_warmth";
    std::error_code error;
    std::filesystem::remove_all(cacheDirectory, error);
    setenv("RUBIK_TABLE_CACHE_DIR", cacheDirectory.string().c_str(), 1);

    rubik::SolveOptions options;
    options.mode = rubik::SolveMode::Optimal;
    options.metric = rubik::Metric::HTM;
    options.profile = rubik::SolveProfile::Auto;
    options.maxDepth = 15;
    options.cachePolicy = rubik::CachePolicy::RequireWarm;
    options.maxMemoryBytes = 0;
    options.threads = 0;

    const auto plan = rubik::detail::makeOptimalPlan(options);
    expect(!plan.supported);
    expect(plan.status == rubik::SolveStatus::CacheNotReady);
    expect(plan.publicPlan.effectiveProfile == rubik::SolveProfile::LargeLocal);
    expect(plan.publicPlan.strategyName == "auto_desktop_tail");
    expect(plan.publicPlan.diskCacheEnabled);
    expect(!plan.publicPlan.diskCacheWarm);

    unsetenv("RUBIK_TABLE_CACHE_DIR");
    std::filesystem::remove_all(cacheDirectory, error);
}

void testAutoPlannerFallsBackForColdTightTimeout()
{
    rubik::SolveOptions options;
    options.mode = rubik::SolveMode::Optimal;
    options.metric = rubik::Metric::HTM;
    options.profile = rubik::SolveProfile::Auto;
    options.maxDepth = 15;
    options.timeout = std::chrono::milliseconds{1500};
    options.cachePolicy = rubik::CachePolicy::Auto;
    options.maxMemoryBytes = 0;
    options.threads = 0;

    const auto decision = rubik::detail::makeAutoPlan(options, false);
    expect(decision.supported);
    expect(decision.effectiveProfile == rubik::SolveProfile::Performance);
    expect(std::string(decision.strategyName) == "auto_timeout_fallback");
}

void testAutoOptimalPlanUsesRealTimeoutCacheWarmth()
{
    const std::filesystem::path cacheDirectory =
        std::filesystem::temp_directory_path() / "rubik_auto_plan_uses_real_timeout_cache_warmth";
    std::error_code error;
    std::filesystem::remove_all(cacheDirectory, error);
    setenv("RUBIK_TABLE_CACHE_DIR", cacheDirectory.string().c_str(), 1);

    rubik::SolveOptions options;
    options.mode = rubik::SolveMode::Optimal;
    options.metric = rubik::Metric::HTM;
    options.profile = rubik::SolveProfile::Auto;
    options.maxDepth = 15;
    options.timeout = std::chrono::milliseconds{1500};
    options.cachePolicy = rubik::CachePolicy::Auto;
    options.maxMemoryBytes = 0;
    options.threads = 0;

    const auto plan = rubik::detail::makeOptimalPlan(options);
    expect(plan.supported);
    expect(plan.publicPlan.effectiveProfile == rubik::SolveProfile::Performance);
    expect(plan.publicPlan.strategyName == "auto_timeout_fallback");

    unsetenv("RUBIK_TABLE_CACHE_DIR");
    std::filesystem::remove_all(cacheDirectory, error);
}

void testAutoPlannerReportsFullTailPayload()
{
    rubik::SolveOptions options;
    options.mode = rubik::SolveMode::Optimal;
    options.metric = rubik::Metric::HTM;
    options.profile = rubik::SolveProfile::Auto;
    options.maxDepth = 15;
    options.maxMemoryBytes = 0;
    options.threads = 0;

    const auto plan = rubik::detail::makeOptimalPlan(options);
    expect(plan.supported);
    expect(plan.publicPlan.estimatedTablePayloadBytes > 1024ull * 1024ull * 1024ull);
}

void testAutoStrongMoveOrderingPolicy()
{
    rubik::SolveOptions requested;
    requested.mode = rubik::SolveMode::Optimal;
    requested.metric = rubik::Metric::HTM;
    requested.profile = rubik::SolveProfile::Auto;
    requested.maxDepth = 15;

    const auto plan = rubik::detail::makeOptimalPlan(requested);
    expect(plan.supported);
    expect(rubik::detail::autoStrongMoveOrderingEnabled(requested, plan.effectiveOptions, 9));
    expect(!rubik::detail::autoStrongMoveOrderingEnabled(requested, plan.effectiveOptions, 9, false));
    expect(!rubik::detail::autoStrongMoveOrderingEnabled(requested, plan.effectiveOptions, 9, true, false, 4));
    expect(!rubik::detail::autoStrongMoveOrderingEnabled(requested, plan.effectiveOptions, 9, true, false, 7));
    expect(rubik::detail::autoStrongMoveOrderingEnabled(requested, plan.effectiveOptions, 9, true, true, 7));
    expect(rubik::detail::autoStrongMoveOrderingEnabled(requested, plan.effectiveOptions, 9, true, false, 14));
    expect(!rubik::detail::autoStrongMoveOrderingEnabled(requested, plan.effectiveOptions, 8));
    expect(rubik::detail::autoStrongMoveOrderingEnabled(requested, plan.effectiveOptions, 8, true, true, 6));
    expect(!rubik::detail::autoStrongMoveOrderingEnabled(requested, plan.effectiveOptions, 8, true, true, 7));
    expect(!rubik::detail::autoStrongMoveOrderingEnabled(requested, plan.effectiveOptions, 8, true, false, 6));
    expect(!rubik::detail::autoStrongMoveOrderingEnabled(requested, plan.effectiveOptions, 10));

    rubik::SolveOptions manual = requested;
    manual.profile = rubik::SolveProfile::LargeLocal;
    expect(!rubik::detail::autoStrongMoveOrderingEnabled(manual, manual, 9));

    rubik::SolveOptions nonOptimal = requested;
    nonOptimal.mode = rubik::SolveMode::Fast;
    expect(!rubik::detail::autoStrongMoveOrderingEnabled(nonOptimal, plan.effectiveOptions, 9));
}

void testAutoSolveReportsPlan()
{
    rubik::Solver solver;
    rubik::SolveOptions options;
    options.mode = rubik::SolveMode::Optimal;
    options.metric = rubik::Metric::HTM;
    options.profile = rubik::SolveProfile::Auto;
    options.maxDepth = 0;
    options.timeout = std::chrono::milliseconds{1000};
    options.threads = 0;

    const rubik::SolveResult result = solver.solve(rubik::Cube::solved(), options);
    expect(result.status == rubik::SolveStatus::Solved);
    expect(result.plan.requestedProfile == rubik::SolveProfile::Auto);
    expect(result.plan.effectiveProfile == rubik::SolveProfile::Performance);
    expect(result.plan.effectiveThreads >= 1);
    expect(result.plan.effectiveMaxMemoryBytes == 2ull * 1024ull * 1024ull * 1024ull);
    expect(result.plan.strategyName == "auto_shallow_optimal");
    expect(result.plan.optimalMoveOrdering == "base_bound");
    expect(result.plan.rootOrderingProfile.empty());
}

void testAutoFastSolveIsUnsupported()
{
    rubik::Solver solver;
    rubik::SolveOptions options;
    options.mode = rubik::SolveMode::Fast;
    options.profile = rubik::SolveProfile::Auto;

    const rubik::SolveResult result = solver.solve(rubik::Cube::solved(), options);
    expect(result.status == rubik::SolveStatus::UnsupportedOptions);
    expect(result.plan.requestedProfile == rubik::SolveProfile::Auto);
}

void testPrepareCacheDryRun()
{
    rubik::CacheSetupOptions options;
    options.profile = rubik::SolveProfile::Auto;
    options.cachePolicy = rubik::CachePolicy::AllowBuild;
    options.maxMemoryBytes = 0;
    options.threads = 0;
    options.dryRun = true;

    const rubik::CacheSetupResult result = rubik::prepareCache(options);
    expect(result.ready);
    expect(result.plan.requestedProfile == rubik::SolveProfile::Auto);
    expect(result.plan.effectiveProfile == rubik::SolveProfile::LargeLocal);
    expect(result.bytesPrepared == 0);
    expect(!result.message.empty());
}

void testPrepareCacheDryRunReportsColdCache()
{
    const std::filesystem::path cacheDirectory =
        std::filesystem::temp_directory_path() / "rubik_cache_dry_run_reports_cold_cache";
    std::error_code error;
    std::filesystem::remove_all(cacheDirectory, error);

    rubik::CacheSetupOptions options;
    options.profile = rubik::SolveProfile::Auto;
    options.cachePolicy = rubik::CachePolicy::AllowBuild;
    options.maxMemoryBytes = 0;
    options.threads = 0;
    options.cacheDirectory = cacheDirectory;
    options.dryRun = true;

    const rubik::CacheSetupResult result = rubik::prepareCache(options);
    expect(result.ready);
    expect(!result.cacheWarm);
    expect(!result.plan.diskCacheWarm);
    expect(result.bytesPrepared == 0);
    expect(result.bytesMissing == result.plan.estimatedTablePayloadBytes);
    expect(result.message.find("cache cold") != std::string::npos);

    std::filesystem::remove_all(cacheDirectory, error);
}

void testPrepareCacheRequireWarmReportsColdCache()
{
    const std::filesystem::path cacheDirectory =
        std::filesystem::temp_directory_path() / "rubik_cache_require_warm_reports_cold_cache";
    std::error_code error;
    std::filesystem::remove_all(cacheDirectory, error);

    rubik::CacheSetupOptions options;
    options.profile = rubik::SolveProfile::Auto;
    options.cachePolicy = rubik::CachePolicy::RequireWarm;
    options.maxMemoryBytes = 0;
    options.threads = 0;
    options.cacheDirectory = cacheDirectory;

    const rubik::CacheSetupResult result = rubik::prepareCache(options);
    expect(!result.ready);
    expect(!result.cacheWarm);
    expect(result.bytesPrepared == 0);
    expect(result.bytesMissing == result.plan.estimatedTablePayloadBytes);
    expect(result.message.find("cache cold") != std::string::npos);

    std::filesystem::remove_all(cacheDirectory, error);
}

void testRequireWarmSolveRejectsColdCache()
{
    const std::filesystem::path cacheDirectory =
        std::filesystem::temp_directory_path() / "rubik_require_warm_rejects_cold_cache";
    std::error_code error;
    std::filesystem::remove_all(cacheDirectory, error);
    setenv("RUBIK_TABLE_CACHE_DIR", cacheDirectory.string().c_str(), 1);

    rubik::Cube cube = rubik::Cube::solved();
    cube.apply(rubik::parseMoves("R"));

    rubik::Solver solver;
    rubik::SolveOptions options;
    options.mode = rubik::SolveMode::Optimal;
    options.metric = rubik::Metric::HTM;
    options.profile = rubik::SolveProfile::Auto;
    options.cachePolicy = rubik::CachePolicy::RequireWarm;
    options.maxDepth = 1;
    options.timeout = std::chrono::seconds(1);
    options.threads = 0;

    const rubik::SolveResult result = solver.solve(cube, options);
    expect(result.status == rubik::SolveStatus::CacheNotReady);
    expect(!result.plan.diskCacheWarm);
    expect(result.nodesExpanded == 0);

    unsetenv("RUBIK_TABLE_CACHE_DIR");
    std::filesystem::remove_all(cacheDirectory, error);
}

std::vector<std::uint8_t> buildCornerEdgeOrientationPruningForTest()
{
    constexpr std::uint8_t unvisited = 0xff;
    const auto& cornerMoves = rubik::move_tables::cornerOrientation();
    const auto& edgeMoves = rubik::move_tables::edgeOrientation();
    const std::uint32_t edgeCount = rubik::coordinates::edge_orientation_count;
    std::vector<std::uint8_t> table(cornerMoves.size() * edgeMoves.size(), unvisited);
    std::queue<std::uint32_t> frontier;

    table[0] = 0;
    frontier.push(0);

    while (!frontier.empty()) {
        const std::uint32_t state = frontier.front();
        frontier.pop();
        const std::uint32_t corner = state / edgeCount;
        const std::uint32_t edge = state % edgeCount;
        const std::uint8_t nextDepth = static_cast<std::uint8_t>(table[state] + 1);

        for (int move = 0; move < rubik::move_count; ++move) {
            const std::uint32_t next =
                cornerMoves[corner][move] * edgeCount +
                edgeMoves[edge][move];
            if (table[next] == unvisited) {
                table[next] = nextDepth;
                frontier.push(next);
            }
        }
    }

    return table;
}

void testSolvedCube()
{
    const rubik::Cube cube = rubik::Cube::solved();
    assert(cube.isSolved());
    assert(cube.isValid());
    assert(cube.toString() == "UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB");
}

void testStructuredStickerInput()
{
    const auto parsed = rubik::Cube::fromStickers(
        "UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB");
    assert(parsed);
    assert(parsed.cube.isSolved());

    const auto invalid = rubik::Cube::fromStickers("not-a-cube");
    assert(!invalid);
    assert(invalid.error.code == rubik::CubeErrorCode::InvalidStickerCount);
}

void testPhysicalValidation()
{
    rubik::Cube scrambled = rubik::Cube::solved();
    scrambled.apply(rubik::parseMoves("R U R' U' F2"));
    assert(scrambled.isValid());

    std::string flippedEdge = rubik::Cube::solved().toString();
    std::swap(flippedEdge[5], flippedEdge[10]);

    const auto invalid = rubik::Cube::fromStickers(flippedEdge);
    assert(!invalid);
    assert(invalid.error.code == rubik::CubeErrorCode::InvalidEdgeOrientation);
}

void testCoordinates()
{
    const auto solved = rubik::CubieCube::fromCube(rubik::Cube::solved());
    assert(solved);
    assert(rubik::coordinates::cornerOrientation(solved.cube) == 0);
    assert(rubik::coordinates::edgeOrientation(solved.cube) == 0);
    assert(rubik::coordinates::cornerPermutation(solved.cube) == 0);
    assert(rubik::coordinates::edgePermutation(solved.cube) == 0);
    assert(rubik::coordinates::sliceEdges(solved.cube) == 0);
    assert(rubik::coordinates::sliceEdgePermutation(solved.cube) == 0);
    assert(rubik::coordinates::upEdgePermutation(solved.cube) == 0);
    assert(rubik::coordinates::downEdgePermutation(solved.cube) == 0);

    rubik::Cube scrambled = rubik::Cube::solved();
    scrambled.apply(rubik::parseMoves("R U R' U' F2 D L2"));
    const auto cubie = rubik::CubieCube::fromCube(scrambled);
    assert(cubie);

    assert(rubik::coordinates::cornerOrientation(cubie.cube) < rubik::coordinates::corner_orientation_count);
    assert(rubik::coordinates::edgeOrientation(cubie.cube) < rubik::coordinates::edge_orientation_count);
    assert(rubik::coordinates::cornerPermutation(cubie.cube) < rubik::coordinates::corner_permutation_count);
    assert(rubik::coordinates::edgePermutation(cubie.cube) < rubik::coordinates::edge_permutation_count);
    assert(rubik::coordinates::sliceEdges(cubie.cube) < rubik::coordinates::slice_edge_count);
    assert(rubik::coordinates::upEdgePermutation(cubie.cube) < rubik::coordinates::edge_group_permutation_count);
    assert(rubik::coordinates::downEdgePermutation(cubie.cube) < rubik::coordinates::edge_group_permutation_count);
}

void testCubieMoveMatchesStickerMove()
{
    rubik::Cube stickerCube = rubik::Cube::solved();
    auto cubie = rubik::CubieCube::fromCube(stickerCube);
    assert(cubie);

    const auto moves = rubik::parseMoves("R U R' U' F2 D L2 B");
    stickerCube.apply(moves);
    cubie.cube.apply(moves);

    const auto expected = rubik::CubieCube::fromCube(stickerCube);
    assert(expected);
    assert(cubie.cube == expected.cube);
    assert(cubie.cube.toCube() == stickerCube);
}

void testOrientationMoveTables()
{
    const auto& cornerTable = rubik::move_tables::cornerOrientation();
    const auto& edgeTable = rubik::move_tables::edgeOrientation();
    const auto& sliceTable = rubik::move_tables::sliceEdges();
    const auto& slicePermutationTable = rubik::move_tables::sliceEdgePermutation();
    const auto& cornerPermutationTable = rubik::move_tables::cornerPermutation();
    const auto& upEdgeTable = rubik::move_tables::upEdgePermutation();
    const auto& downEdgeTable = rubik::move_tables::downEdgePermutation();
    assert(cornerTable.size() == rubik::coordinates::corner_orientation_count);
    assert(edgeTable.size() == rubik::coordinates::edge_orientation_count);
    assert(sliceTable.size() == rubik::coordinates::slice_edge_count);
    assert(slicePermutationTable.size() == rubik::coordinates::slice_edge_permutation_count);
    assert(cornerPermutationTable.size() == rubik::coordinates::corner_permutation_count);
    assert(upEdgeTable.size() == rubik::coordinates::edge_group_permutation_count);
    assert(downEdgeTable.size() == rubik::coordinates::edge_group_permutation_count);

    for (rubik::Move move : rubik::allMoves()) {
        rubik::CubieCube cube = rubik::CubieCube::solved();
        cube.apply(move);

        assert(cornerTable[0][static_cast<int>(move)] == rubik::coordinates::cornerOrientation(cube));
        assert(edgeTable[0][static_cast<int>(move)] == rubik::coordinates::edgeOrientation(cube));
        assert(sliceTable[0][static_cast<int>(move)] == rubik::coordinates::sliceEdges(cube));
        assert(cornerPermutationTable[0][static_cast<int>(move)] == rubik::coordinates::cornerPermutation(cube));
        assert(upEdgeTable[0][static_cast<int>(move)] == rubik::coordinates::upEdgePermutation(cube));
        assert(downEdgeTable[0][static_cast<int>(move)] == rubik::coordinates::downEdgePermutation(cube));
    }

    for (rubik::Move move : {
             rubik::Move::U,
             rubik::Move::U2,
             rubik::Move::Up,
             rubik::Move::D,
             rubik::Move::D2,
             rubik::Move::Dp,
             rubik::Move::R2,
             rubik::Move::F2,
             rubik::Move::L2,
             rubik::Move::B2,
         }) {
        rubik::CubieCube cube = rubik::CubieCube::solved();
        cube.apply(move);
        assert(slicePermutationTable[0][static_cast<int>(move)] == rubik::coordinates::sliceEdgePermutation(cube));
    }
}

void testCoordinateMoveTablesAcrossSequences()
{
    rubik::CubieCube cube = rubik::CubieCube::solved();
    std::uint32_t cornerOrientation = 0;
    std::uint32_t edgeOrientation = 0;
    std::uint32_t sliceEdges = 0;
    std::uint32_t cornerPermutation = 0;
    std::uint32_t upEdgePermutation = 0;
    std::uint32_t downEdgePermutation = 0;

    const auto moves = rubik::parseMoves("R U R' U' F2 D L2 B R2 F D' L U2 B' R F2");
    for (rubik::Move move : moves) {
        const int moveIndex = static_cast<int>(move);
        cube.apply(move);

        cornerOrientation = rubik::move_tables::cornerOrientation()[cornerOrientation][moveIndex];
        edgeOrientation = rubik::move_tables::edgeOrientation()[edgeOrientation][moveIndex];
        sliceEdges = rubik::move_tables::sliceEdges()[sliceEdges][moveIndex];
        cornerPermutation = rubik::move_tables::cornerPermutation()[cornerPermutation][moveIndex];
        upEdgePermutation = rubik::move_tables::upEdgePermutation()[upEdgePermutation][moveIndex];
        downEdgePermutation = rubik::move_tables::downEdgePermutation()[downEdgePermutation][moveIndex];

        assert(cornerOrientation == rubik::coordinates::cornerOrientation(cube));
        assert(edgeOrientation == rubik::coordinates::edgeOrientation(cube));
        assert(sliceEdges == rubik::coordinates::sliceEdges(cube));
        assert(cornerPermutation == rubik::coordinates::cornerPermutation(cube));
        assert(upEdgePermutation == rubik::coordinates::upEdgePermutation(cube));
        assert(downEdgePermutation == rubik::coordinates::downEdgePermutation(cube));
    }
}

void testCubeRotationSymmetries()
{
    using rubik::detail::SymmetryId;

    const auto& symmetries = rubik::detail::cubeRotationSymmetries();
    assert(symmetries.size() == rubik::detail::cube_rotation_symmetry_count);
    assert(rubik::detail::identitySymmetry() == 0);

    const rubik::Cube solved = rubik::Cube::solved();
    for (SymmetryId symmetry = 0; symmetry < symmetries.size(); ++symmetry) {
        const rubik::Cube transformedSolved = rubik::detail::applySymmetry(solved, symmetry);
        assert(transformedSolved.isSolved());
        assert(transformedSolved.isValid());

        const SymmetryId inverse = rubik::detail::inverseSymmetry(symmetry);
        assert(rubik::detail::composeSymmetries(symmetry, inverse) == rubik::detail::identitySymmetry());
        assert(rubik::detail::composeSymmetries(inverse, symmetry) == rubik::detail::identitySymmetry());
    }

    rubik::Cube cube = rubik::Cube::solved();
    cube.apply(rubik::parseMoves("R U R' U' F2 D L2 B"));
    const auto cubie = rubik::CubieCube::fromCube(cube);
    assert(cubie);

    for (SymmetryId symmetry = 0; symmetry < symmetries.size(); ++symmetry) {
        const SymmetryId inverse = rubik::detail::inverseSymmetry(symmetry);

        const rubik::Cube transformed = rubik::detail::applySymmetry(cube, symmetry);
        assert(transformed.isValid());
        assert(rubik::detail::applySymmetry(transformed, inverse) == cube);

        const rubik::CubieCube transformedCubie = rubik::detail::applySymmetry(cubie.cube, symmetry);
        assert(transformedCubie.validate().code == rubik::CubeErrorCode::None);
        assert(rubik::detail::applySymmetry(transformedCubie, inverse) == cubie.cube);
        assert(transformedCubie.toCube() == transformed);
    }
}

bool canSolveWithinDepth(const rubik::Cube& start, int maxDepth)
{
    if (start.isSolved()) {
        return true;
    }
    if (maxDepth == 0) {
        return false;
    }

    for (rubik::Move move : rubik::allMoves()) {
        rubik::Cube next = start;
        next.apply(move);
        if (canSolveWithinDepth(next, maxDepth - 1)) {
            return true;
        }
    }

    return false;
}

void testCubeRotationSymmetriesPreserveOptimalDistance()
{
    using rubik::detail::SymmetryId;

    for (const std::string& scramble : {"R", "R U", "F2 D"}) {
        rubik::Cube cube = rubik::Cube::solved();
        const auto scrambleMoves = rubik::parseMoves(scramble);
        cube.apply(scrambleMoves);

        for (SymmetryId symmetry = 0; symmetry < rubik::detail::cubeRotationSymmetries().size(); ++symmetry) {
            const rubik::Cube transformed = rubik::detail::applySymmetry(cube, symmetry);
            assert(canSolveWithinDepth(transformed, static_cast<int>(scrambleMoves.size())));
            assert(!canSolveWithinDepth(transformed, static_cast<int>(scrambleMoves.size()) - 1));
        }
    }
}

void testCoordinateSymmetryTables()
{
    const auto& cornerOrientation = rubik::detail::cornerOrientationSymmetries();
    const auto& edgeOrientation = rubik::detail::edgeOrientationSymmetries();
    const auto& sliceEdges = rubik::detail::sliceEdgeSymmetries();

    assert(cornerOrientation.size() == rubik::coordinates::corner_orientation_count);
    assert(edgeOrientation.size() == rubik::coordinates::edge_orientation_count);
    assert(sliceEdges.size() == rubik::coordinates::slice_edge_count);

    const auto identity = rubik::detail::identitySymmetry();
    for (std::uint32_t state = 0; state < rubik::coordinates::corner_orientation_count; ++state) {
        assert(cornerOrientation[state][identity] == state);
    }
    for (std::uint32_t state = 0; state < rubik::coordinates::edge_orientation_count; ++state) {
        assert(edgeOrientation[state][identity] == state);
    }
    for (std::uint32_t state = 0; state < rubik::coordinates::slice_edge_count; ++state) {
        assert(sliceEdges[state][identity] == state);
    }

    for (rubik::detail::SymmetryId first = 0; first < rubik::detail::cube_rotation_symmetry_count; ++first) {
        for (rubik::detail::SymmetryId second = 0; second < rubik::detail::cube_rotation_symmetry_count; ++second) {
            const auto composed = rubik::detail::composeSymmetries(first, second);

            for (std::uint32_t state : {0U, 1U, 17U, 2186U}) {
                assert(cornerOrientation[cornerOrientation[state][second]][first] ==
                    cornerOrientation[state][composed]);
            }
            for (std::uint32_t state : {0U, 1U, 31U, 2047U}) {
                assert(edgeOrientation[edgeOrientation[state][second]][first] ==
                    edgeOrientation[state][composed]);
            }
            if (rubik::detail::preservesUdSlice(first) && rubik::detail::preservesUdSlice(second)) {
                assert(rubik::detail::preservesUdSlice(composed));
                for (std::uint32_t state : {0U, 1U, 137U, 494U}) {
                    assert(sliceEdges[sliceEdges[state][second]][first] ==
                        sliceEdges[state][composed]);
                }
            }
        }
    }
}

void testCoordinateSymmetryReductions()
{
    const auto& cornerTable = rubik::detail::cornerOrientationSymmetries();
    const auto& edgeTable = rubik::detail::edgeOrientationSymmetries();
    const auto& cornerReduction = rubik::detail::cornerOrientationSymmetryReduction();
    const auto& edgeReduction = rubik::detail::edgeOrientationSymmetryReduction();

    assert(cornerReduction.canonicalState.size() == rubik::coordinates::corner_orientation_count);
    assert(cornerReduction.canonicalSymmetry.size() == rubik::coordinates::corner_orientation_count);
    assert(cornerReduction.orbitIndex.size() == rubik::coordinates::corner_orientation_count);
    assert(cornerReduction.orbitRepresentative.size() == cornerReduction.orbitCount);
    assert(cornerReduction.orbitCount > 0);
    assert(cornerReduction.orbitCount < rubik::coordinates::corner_orientation_count);

    assert(edgeReduction.canonicalState.size() == rubik::coordinates::edge_orientation_count);
    assert(edgeReduction.canonicalSymmetry.size() == rubik::coordinates::edge_orientation_count);
    assert(edgeReduction.orbitIndex.size() == rubik::coordinates::edge_orientation_count);
    assert(edgeReduction.orbitRepresentative.size() == edgeReduction.orbitCount);
    assert(edgeReduction.orbitCount > 0);
    assert(edgeReduction.orbitCount < rubik::coordinates::edge_orientation_count);

    for (std::uint32_t state : {0U, 1U, 17U, 123U, 2186U}) {
        const std::uint32_t canonical = cornerReduction.canonicalState[state];
        assert(cornerReduction.canonicalState[canonical] == canonical);
        assert(cornerReduction.orbitIndex[state] == cornerReduction.orbitIndex[canonical]);

        for (rubik::detail::SymmetryId symmetry = 0; symmetry < rubik::detail::cube_rotation_symmetry_count; ++symmetry) {
            const std::uint32_t related = cornerTable[state][symmetry];
            assert(cornerReduction.canonicalState[related] == canonical);
            assert(cornerReduction.orbitIndex[related] == cornerReduction.orbitIndex[state]);
        }
    }

    for (std::uint32_t state : {0U, 1U, 31U, 127U, 2047U}) {
        const std::uint32_t canonical = edgeReduction.canonicalState[state];
        assert(edgeReduction.canonicalState[canonical] == canonical);
        assert(edgeReduction.orbitIndex[state] == edgeReduction.orbitIndex[canonical]);

        for (rubik::detail::SymmetryId symmetry = 0; symmetry < rubik::detail::cube_rotation_symmetry_count; ++symmetry) {
            const std::uint32_t related = edgeTable[state][symmetry];
            assert(edgeReduction.canonicalState[related] == canonical);
            assert(edgeReduction.orbitIndex[related] == edgeReduction.orbitIndex[state]);
        }
    }
}

void testCombinedCoordinateSymmetryReduction()
{
    const auto& reduction = rubik::detail::cornerEdgeOrientationSymmetryReduction();
    const auto& cornerTable = rubik::detail::cornerOrientationSymmetries();
    const auto& edgeTable = rubik::detail::edgeOrientationSymmetries();
    const std::uint32_t edgeCount = rubik::coordinates::edge_orientation_count;
    const std::uint32_t stateCount =
        rubik::coordinates::corner_orientation_count * rubik::coordinates::edge_orientation_count;

    assert(reduction.canonicalState.size() == stateCount);
    assert(reduction.canonicalSymmetry.size() == stateCount);
    assert(reduction.orbitIndex.size() == stateCount);
    assert(reduction.orbitRepresentative.size() == reduction.orbitCount);
    assert(reduction.orbitCount > 0);
    assert(reduction.orbitCount < stateCount);

    for (std::uint32_t state : {0U, 1U, 2048U, 123456U, stateCount - 1}) {
        const std::uint32_t canonical = reduction.canonicalState[state];
        assert(reduction.canonicalState[canonical] == canonical);
        assert(reduction.orbitIndex[state] == reduction.orbitIndex[canonical]);

        const std::uint32_t corner = state / edgeCount;
        const std::uint32_t edge = state % edgeCount;
        for (rubik::detail::SymmetryId symmetry = 0; symmetry < rubik::detail::cube_rotation_symmetry_count; ++symmetry) {
            const std::uint32_t related =
                cornerTable[corner][symmetry] * edgeCount +
                edgeTable[edge][symmetry];
            assert(reduction.canonicalState[related] == canonical);
            assert(reduction.orbitIndex[related] == reduction.orbitIndex[state]);
        }
    }
}

void testEdgeOrientationSliceSymmetryReduction()
{
    const auto& reduction = rubik::detail::edgeOrientationSliceEdgeSymmetryReduction();
    const auto& edgeTable = rubik::detail::edgeOrientationSymmetries();
    const auto& sliceTable = rubik::detail::sliceEdgeSymmetries();
    const std::uint32_t sliceCount = rubik::coordinates::slice_edge_count;
    const std::uint32_t stateCount =
        rubik::coordinates::edge_orientation_count * rubik::coordinates::slice_edge_count;

    assert(reduction.canonicalState.size() == stateCount);
    assert(reduction.canonicalSymmetry.size() == stateCount);
    assert(reduction.orbitIndex.size() == stateCount);
    assert(reduction.orbitRepresentative.size() == reduction.orbitCount);
    assert(reduction.orbitCount > 0);
    assert(reduction.orbitCount < stateCount);

    for (std::uint32_t state : {0U, 1U, 495U, 54321U, stateCount - 1}) {
        const std::uint32_t canonical = reduction.canonicalState[state];
        assert(reduction.canonicalState[canonical] == canonical);
        assert(reduction.orbitIndex[state] == reduction.orbitIndex[canonical]);

        const std::uint32_t edge = state / sliceCount;
        const std::uint32_t slice = state % sliceCount;
        for (rubik::detail::SymmetryId symmetry = 0; symmetry < rubik::detail::cube_rotation_symmetry_count; ++symmetry) {
            if (!rubik::detail::preservesUdSlice(symmetry)) {
                continue;
            }
            const std::uint32_t related =
                edgeTable[edge][symmetry] * sliceCount +
                sliceTable[slice][symmetry];
            assert(reduction.canonicalState[related] == canonical);
            assert(reduction.orbitIndex[related] == reduction.orbitIndex[state]);
        }
    }
}

void testCornerOrientationSliceSymmetryReduction()
{
    const auto& reduction = rubik::detail::cornerOrientationSliceEdgeSymmetryReduction();
    const auto& cornerTable = rubik::detail::cornerOrientationSymmetries();
    const auto& sliceTable = rubik::detail::sliceEdgeSymmetries();
    const std::uint32_t sliceCount = rubik::coordinates::slice_edge_count;
    const std::uint32_t stateCount =
        rubik::coordinates::corner_orientation_count * rubik::coordinates::slice_edge_count;

    assert(reduction.canonicalState.size() == stateCount);
    assert(reduction.canonicalSymmetry.size() == stateCount);
    assert(reduction.orbitIndex.size() == stateCount);
    assert(reduction.orbitRepresentative.size() == reduction.orbitCount);
    assert(reduction.orbitCount > 0);
    assert(reduction.orbitCount < stateCount);

    for (std::uint32_t state : {0U, 1U, 495U, 54321U, stateCount - 1}) {
        const std::uint32_t canonical = reduction.canonicalState[state];
        assert(reduction.canonicalState[canonical] == canonical);
        assert(reduction.orbitIndex[state] == reduction.orbitIndex[canonical]);

        const std::uint32_t corner = state / sliceCount;
        const std::uint32_t slice = state % sliceCount;
        for (rubik::detail::SymmetryId symmetry = 0; symmetry < rubik::detail::cube_rotation_symmetry_count; ++symmetry) {
            if (!rubik::detail::preservesUdSlice(symmetry)) {
                continue;
            }
            const std::uint32_t related =
                cornerTable[corner][symmetry] * sliceCount +
                sliceTable[slice][symmetry];
            assert(reduction.canonicalState[related] == canonical);
            assert(reduction.orbitIndex[related] == reduction.orbitIndex[state]);
        }
    }
}

void testReducedSymmetryPruningTables()
{
    const auto& cornerReduced = rubik::detail::reducedCornerOrientationPruning();
    const auto& edgeReduced = rubik::detail::reducedEdgeOrientationPruning();
    const auto& cornerReduction = rubik::detail::cornerOrientationSymmetryReduction();
    const auto& edgeReduction = rubik::detail::edgeOrientationSymmetryReduction();
    const auto& cornerPruning = rubik::pruning_tables::cornerOrientation();
    const auto& edgePruning = rubik::pruning_tables::edgeOrientation();

    assert(cornerReduced.size() == cornerReduction.orbitCount);
    assert(edgeReduced.size() == edgeReduction.orbitCount);
    assert(cornerReduced[cornerReduction.orbitIndex[0]] == 0);
    assert(edgeReduced[edgeReduction.orbitIndex[0]] == 0);

    for (std::uint32_t state = 0; state < rubik::coordinates::corner_orientation_count; ++state) {
        assert(cornerReduced[cornerReduction.orbitIndex[state]] != 0xff);
        assert(cornerReduced[cornerReduction.orbitIndex[state]] <= cornerPruning[state]);
    }

    for (std::uint32_t state = 0; state < rubik::coordinates::edge_orientation_count; ++state) {
        assert(edgeReduced[edgeReduction.orbitIndex[state]] != 0xff);
        assert(edgeReduced[edgeReduction.orbitIndex[state]] <= edgePruning[state]);
    }
}

void testCombinedReducedSymmetryPruningTable()
{
    const auto& reduced = rubik::detail::reducedCornerEdgeOrientationPruning();
    const auto& reduction = rubik::detail::cornerEdgeOrientationSymmetryReduction();
    const auto fullPruning = buildCornerEdgeOrientationPruningForTest();

    assert(reduced.size() == reduction.orbitCount);
    assert(reduced[reduction.orbitIndex[0]] == 0);

    for (std::uint32_t state : {0U, 1U, 2048U, 123456U, 4478975U}) {
        assert(reduced[reduction.orbitIndex[state]] != 0xff);
        assert(reduced[reduction.orbitIndex[state]] <= fullPruning[state]);
    }
}

void testExperimentalSymmetryLowerBoundDoesNotWeaken()
{
    const rubik::Solver solver;
    for (const std::string& scramble : {
             "R U R' U' F2 D L2",
             "R U R' U' F2 D L2 B R2 F",
             "F R U R' U' D L2 B2",
         }) {
        rubik::Cube cube = rubik::Cube::solved();
        cube.apply(rubik::parseMoves(scramble));

        unsetenv("RUBIK_EXPERIMENTAL_SYMMETRY_BOUNDS");
        const int baseline = solver.lowerBound(cube);
        setenv("RUBIK_EXPERIMENTAL_SYMMETRY_BOUNDS", "1", 1);
        const int experimental = solver.lowerBound(cube);
        unsetenv("RUBIK_EXPERIMENTAL_SYMMETRY_BOUNDS");

        assert(experimental >= baseline);
    }
}

void testExperimentalThreePhase1LowerBoundDoesNotWeaken()
{
    const rubik::Solver solver;
    for (const std::string& scramble : {
             "R U R' U' F2 D L2",
             "R U R' U' F2 D L2 B R2 F",
             "F R U R' U' D L2 B2",
         }) {
        rubik::Cube cube = rubik::Cube::solved();
        cube.apply(rubik::parseMoves(scramble));

        unsetenv("RUBIK_EXPERIMENTAL_THREE_PHASE1_BOUNDS");
        setenv("RUBIK_DISABLE_THREE_PHASE1_BOUNDS", "1", 1);
        const int baseline = solver.lowerBound(cube);
        unsetenv("RUBIK_DISABLE_THREE_PHASE1_BOUNDS");
        setenv("RUBIK_EXPERIMENTAL_THREE_PHASE1_BOUNDS", "1", 1);
        const int experimental = solver.lowerBound(cube);
        unsetenv("RUBIK_EXPERIMENTAL_THREE_PHASE1_BOUNDS");
        unsetenv("RUBIK_DISABLE_THREE_PHASE1_BOUNDS");

        assert(experimental >= baseline);
    }
}

void testThreePhase1Policy()
{
    const rubik::Solver solver;
    rubik::Cube cube = rubik::Cube::solved();
    cube.apply(rubik::parseMoves("R U R' U' F2 D L2 B R2 F"));

    unsetenv("RUBIK_EXPERIMENTAL_THREE_PHASE1_BOUNDS");
    unsetenv("RUBIK_DISABLE_THREE_PHASE1_BOUNDS");
    const int defaultProfile = solver.lowerBound(cube, rubik::Metric::HTM, rubik::SolveProfile::Default);

    setenv("RUBIK_DISABLE_THREE_PHASE1_BOUNDS", "1", 1);
    const int disabled = solver.lowerBound(cube, rubik::Metric::HTM, rubik::SolveProfile::Default);
    const int embedded = solver.lowerBound(cube, rubik::Metric::HTM, rubik::SolveProfile::Embedded);
    unsetenv("RUBIK_DISABLE_THREE_PHASE1_BOUNDS");

    const int embeddedDefault = solver.lowerBound(cube, rubik::Metric::HTM, rubik::SolveProfile::Embedded);

    assert(defaultProfile >= disabled);
    assert(embeddedDefault >= embedded);

    const auto fast = solver.solve(cube, {
        .mode = rubik::SolveMode::Fast,
        .maxDepth = 16,
        .timeout = std::chrono::milliseconds(1000),
        .profile = rubik::SolveProfile::Default,
        .collectDiagnostics = true,
    });
    assert(fast.status == rubik::SolveStatus::Found ||
        fast.status == rubik::SolveStatus::Solved ||
        fast.status == rubik::SolveStatus::Optimal);
    assert(fast.boundDiagnostics.threePhaseNodeChecks == 0);
    assert(fast.boundDiagnostics.threePhaseCandidateChecks == 0);
}

void testEdgeOrientationSliceReducedSymmetryPruningTable()
{
    const auto& reduced = rubik::detail::reducedEdgeOrientationSliceEdgePruning();
    const auto& reduction = rubik::detail::edgeOrientationSliceEdgeSymmetryReduction();
    const auto& fullPruning = rubik::pruning_tables::edgeOrientationSlice();

    assert(reduced.size() == reduction.orbitCount);
    assert(reduced[reduction.orbitIndex[0]] == 0);

    for (std::uint32_t state : {0U, 1U, 495U, 54321U, 1013759U}) {
        assert(reduced[reduction.orbitIndex[state]] != 0xff);
        assert(reduced[reduction.orbitIndex[state]] <= fullPruning[state]);
    }
}

void testCornerOrientationSliceReducedSymmetryPruningTable()
{
    const auto& reduced = rubik::detail::reducedCornerOrientationSliceEdgePruning();
    const auto& reduction = rubik::detail::cornerOrientationSliceEdgeSymmetryReduction();
    const auto& fullPruning = rubik::pruning_tables::cornerOrientationSlice();

    assert(reduced.size() == reduction.orbitCount);
    assert(reduced[reduction.orbitIndex[0]] == 0);

    for (std::uint32_t state : {0U, 1U, 495U, 54321U, 1082564U}) {
        assert(reduced[reduction.orbitIndex[state]] != 0xff);
        assert(reduced[reduction.orbitIndex[state]] <= fullPruning[state]);
    }
}

void testOrientationPruningTables()
{
    const auto& cornerPruning = rubik::pruning_tables::cornerOrientation();
    const auto& edgePruning = rubik::pruning_tables::edgeOrientation();
    const auto& slicePruning = rubik::pruning_tables::sliceEdges();
    const auto& cornerPermutationPruning = rubik::pruning_tables::cornerPermutation();
    const auto& upEdgePruning = rubik::pruning_tables::upEdgePermutation();
    const auto& downEdgePruning = rubik::pruning_tables::downEdgePermutation();
    const auto& cornerOrientationSlicePruning = rubik::pruning_tables::cornerOrientationSlice();
    const auto& edgeOrientationSlicePruning = rubik::pruning_tables::edgeOrientationSlice();
    const auto& cornerPermutationSlicePruning = rubik::pruning_tables::cornerPermutationSlice();
    const auto& cornerOrientationUpEdgePruning = rubik::pruning_tables::cornerOrientationUpEdgePermutation();
    const auto& cornerOrientationDownEdgePruning = rubik::pruning_tables::cornerOrientationDownEdgePermutation();
    const auto& edgeOrientationUpEdgePruning = rubik::pruning_tables::edgeOrientationUpEdgePermutation();
    const auto& edgeOrientationDownEdgePruning = rubik::pruning_tables::edgeOrientationDownEdgePermutation();
    const auto& cornerPermutationEdgeOrientationPruning = rubik::pruning_tables::cornerPermutationEdgeOrientation();
    const auto& phase2CornerSlicePruning = rubik::pruning_tables::phase2CornerSlicePermutation();
    const auto& phase2UpEdgeSlicePruning = rubik::pruning_tables::phase2UpEdgeSlicePermutation();
    const auto& phase2DownEdgeSlicePruning = rubik::pruning_tables::phase2DownEdgeSlicePermutation();

    assert(cornerPruning.size() == rubik::coordinates::corner_orientation_count);
    assert(edgePruning.size() == rubik::coordinates::edge_orientation_count);
    assert(slicePruning.size() == rubik::coordinates::slice_edge_count);
    assert(cornerPermutationPruning.size() == rubik::coordinates::corner_permutation_count);
    assert(upEdgePruning.size() == rubik::coordinates::edge_group_permutation_count);
    assert(downEdgePruning.size() == rubik::coordinates::edge_group_permutation_count);
    assert(cornerOrientationSlicePruning.size() ==
        rubik::coordinates::corner_orientation_count * rubik::coordinates::slice_edge_count);
    assert(edgeOrientationSlicePruning.size() ==
        rubik::coordinates::edge_orientation_count * rubik::coordinates::slice_edge_count);
    assert(cornerPermutationSlicePruning.size() ==
        rubik::coordinates::corner_permutation_count * rubik::coordinates::slice_edge_count);
    assert(cornerOrientationUpEdgePruning.size() ==
        rubik::coordinates::corner_orientation_count * rubik::coordinates::edge_group_permutation_count);
    assert(cornerOrientationDownEdgePruning.size() ==
        rubik::coordinates::corner_orientation_count * rubik::coordinates::edge_group_permutation_count);
    assert(edgeOrientationUpEdgePruning.size() ==
        rubik::coordinates::edge_orientation_count * rubik::coordinates::edge_group_permutation_count);
    assert(edgeOrientationDownEdgePruning.size() ==
        rubik::coordinates::edge_orientation_count * rubik::coordinates::edge_group_permutation_count);
    assert(cornerPermutationEdgeOrientationPruning.size() ==
        rubik::coordinates::corner_permutation_count * rubik::coordinates::edge_orientation_count);
    assert(phase2CornerSlicePruning.size() ==
        rubik::coordinates::corner_permutation_count * rubik::coordinates::slice_edge_permutation_count);
    assert(phase2UpEdgeSlicePruning.size() ==
        rubik::coordinates::edge_group_permutation_count * rubik::coordinates::slice_edge_permutation_count);
    assert(phase2DownEdgeSlicePruning.size() ==
        rubik::coordinates::edge_group_permutation_count * rubik::coordinates::slice_edge_permutation_count);
    assert(cornerPruning[0] == 0);
    assert(edgePruning[0] == 0);
    assert(slicePruning[0] == 0);
    assert(cornerPermutationPruning[0] == 0);
    assert(upEdgePruning[0] == 0);
    assert(downEdgePruning[0] == 0);
    assert(cornerOrientationSlicePruning[0] == 0);
    assert(edgeOrientationSlicePruning[0] == 0);
    assert(cornerPermutationSlicePruning[0] == 0);
    assert(cornerOrientationUpEdgePruning[0] == 0);
    assert(cornerOrientationDownEdgePruning[0] == 0);
    assert(edgeOrientationUpEdgePruning[0] == 0);
    assert(edgeOrientationDownEdgePruning[0] == 0);
    assert(cornerPermutationEdgeOrientationPruning[0] == 0);
    assert(phase2CornerSlicePruning[0] == 0);
    assert(phase2UpEdgeSlicePruning[0] == 0);
    assert(phase2DownEdgeSlicePruning[0] == 0);

    for (std::uint8_t value : cornerPruning) {
        assert(value != 0xff);
    }
    for (std::uint8_t value : edgePruning) {
        assert(value != 0xff);
    }
    for (std::uint8_t value : slicePruning) {
        assert(value != 0xff);
    }
    for (std::uint8_t value : cornerPermutationPruning) {
        assert(value != 0xff);
    }
    for (std::uint8_t value : upEdgePruning) {
        assert(value != 0xff);
    }
    for (std::uint8_t value : downEdgePruning) {
        assert(value != 0xff);
    }
    for (std::uint8_t value : cornerOrientationSlicePruning) {
        assert(value != 0xff);
    }
    for (std::uint8_t value : edgeOrientationSlicePruning) {
        assert(value != 0xff);
    }

    for (rubik::Move move : rubik::allMoves()) {
        rubik::CubieCube cube = rubik::CubieCube::solved();
        cube.apply(move);
        assert(cornerPruning[rubik::coordinates::cornerOrientation(cube)] <= 1);
        assert(edgePruning[rubik::coordinates::edgeOrientation(cube)] <= 1);
        assert(slicePruning[rubik::coordinates::sliceEdges(cube)] <= 1);
        assert(cornerPermutationPruning[rubik::coordinates::cornerPermutation(cube)] <= 1);
        assert(upEdgePruning[rubik::coordinates::upEdgePermutation(cube)] <= 1);
        assert(downEdgePruning[rubik::coordinates::downEdgePermutation(cube)] <= 1);

        const auto cornerSliceIndex =
            rubik::coordinates::cornerOrientation(cube) * rubik::coordinates::slice_edge_count +
            rubik::coordinates::sliceEdges(cube);
        const auto edgeSliceIndex =
            rubik::coordinates::edgeOrientation(cube) * rubik::coordinates::slice_edge_count +
            rubik::coordinates::sliceEdges(cube);
        assert(cornerOrientationSlicePruning[cornerSliceIndex] <= 1);
        assert(edgeOrientationSlicePruning[edgeSliceIndex] <= 1);
        const auto cornerPermutationSliceIndex =
            rubik::coordinates::cornerPermutation(cube) * rubik::coordinates::slice_edge_count +
            rubik::coordinates::sliceEdges(cube);
        assert(cornerPermutationSlicePruning[cornerPermutationSliceIndex] <= 1);
    }

    for (rubik::Move move : {
             rubik::Move::U,
             rubik::Move::U2,
             rubik::Move::Up,
             rubik::Move::D,
             rubik::Move::D2,
             rubik::Move::Dp,
             rubik::Move::R2,
             rubik::Move::F2,
             rubik::Move::L2,
             rubik::Move::B2,
         }) {
        rubik::CubieCube cube = rubik::CubieCube::solved();
        cube.apply(move);

        const auto slicePermutation = rubik::coordinates::sliceEdgePermutation(cube);
        const auto phase2CornerSliceIndex =
            rubik::coordinates::cornerPermutation(cube) * rubik::coordinates::slice_edge_permutation_count +
            slicePermutation;
        const auto phase2UpEdgeSliceIndex =
            rubik::coordinates::upEdgePermutation(cube) * rubik::coordinates::slice_edge_permutation_count +
            slicePermutation;
        const auto phase2DownEdgeSliceIndex =
            rubik::coordinates::downEdgePermutation(cube) * rubik::coordinates::slice_edge_permutation_count +
            slicePermutation;

        assert(phase2CornerSlicePruning[phase2CornerSliceIndex] <= 1);
        assert(phase2UpEdgeSlicePruning[phase2UpEdgeSliceIndex] <= 1);
        assert(phase2DownEdgeSlicePruning[phase2DownEdgeSliceIndex] <= 1);
    }
}

void assertTableProfileMatchesLoadedTables(std::span<const rubik::detail::TableProfileEntry> profile)
{
    std::size_t manifestBytes = 0;
    std::size_t loadedBytes = 0;

    for (const rubik::detail::TableProfileEntry& entry : profile) {
        const auto& table = entry.table();
        assert(entry.entries == table.size());
        manifestBytes += entry.entries;
        loadedBytes += table.size();
    }

    assert(rubik::detail::tableProfilePayloadBytes(profile) == manifestBytes);
    assert(manifestBytes == loadedBytes);
}

void testTableProfileManifest()
{
    assertTableProfileMatchesLoadedTables(
        rubik::detail::optimalTableProfile(rubik::SolveProfile::Embedded));
    assertTableProfileMatchesLoadedTables(
        rubik::detail::optimalTableProfile(rubik::SolveProfile::Default));
    assertTableProfileMatchesLoadedTables(
        rubik::detail::optimalTableProfile(rubik::SolveProfile::Performance));
    assertTableProfileMatchesLoadedTables(
        rubik::detail::optimalTableProfile(rubik::SolveProfile::LargeLocal));
    assertTableProfileMatchesLoadedTables(rubik::detail::phase1BaseTableProfile());
    assertTableProfileMatchesLoadedTables(rubik::detail::phase2TableProfile());

    assert(rubik::detail::optimalTablePayloadBytes(rubik::SolveProfile::Embedded) == 22123535);
    assert(rubik::detail::optimalTablePayloadBytes(rubik::SolveProfile::Default) == 205322495);
    assert(rubik::detail::optimalTablePayloadBytes(rubik::SolveProfile::Performance) == 346456895);
    assert(rubik::detail::optimalTablePayloadBytes(rubik::SolveProfile::LargeLocal) == 346456895);
    assert(rubik::detail::fastTwoPhaseTablePayloadBytes() == 3638975);
}

void testMoveInverse()
{
    for (rubik::Move move : rubik::allMoves()) {
        rubik::Cube cube = rubik::Cube::solved();
        cube.apply(move);
        cube.apply(rubik::inverse(move));
        assert(cube.isSolved());
    }
}

void testFourQuarterTurns()
{
    for (rubik::Move move : {rubik::Move::U, rubik::Move::R, rubik::Move::F, rubik::Move::D, rubik::Move::L, rubik::Move::B}) {
        rubik::Cube cube = rubik::Cube::solved();
        cube.apply(move);
        cube.apply(move);
        cube.apply(move);
        cube.apply(move);
        assert(cube.isSolved());
    }
}

void testParseMoves()
{
    const auto moves = rubik::parseMoves("R U R' U'");
    assert(moves.size() == 4);
    assert(rubik::toString(moves) == "R U R' U'");
}

void testOptimalTinyScramble()
{
    for (const std::string& scramble : {"R", "R U", "F2 D"}) {
        rubik::Cube cube = rubik::Cube::solved();
        cube.apply(rubik::parseMoves(scramble));

        const rubik::Solver solver;
        const auto result = solver.solve(cube, {.maxDepth = 4});

        assert(result.status == rubik::SolveStatus::Optimal);
        assert(result.isOptimal);
        assert(result.moveCount == static_cast<int>(rubik::parseMoves(scramble).size()));
        assert(result.metric == rubik::Metric::HTM);
        assert(result.nodesExpanded > 0);

        cube.apply(result.moves);
        assert(cube.isSolved());
    }
}

void testOptimalThreadedTinyScramble()
{
    rubik::Cube cube = rubik::Cube::solved();
    const auto scramble = rubik::parseMoves("R U F");
    cube.apply(scramble);

    const rubik::Solver solver;
    const auto result = solver.solve(cube, {
        .mode = rubik::SolveMode::Optimal,
        .maxDepth = 5,
        .threads = 4,
        .profile = rubik::SolveProfile::Default,
    });

    assert(result.status == rubik::SolveStatus::Optimal);
    assert(result.isOptimal);
    assert(result.moveCount == static_cast<int>(scramble.size()));
    assert(result.nodesExpanded > 0);

    cube.apply(result.moves);
    assert(cube.isSolved());
}

void testOptimalRootOrderingReportsSolutionRank()
{
    rubik::Cube cube = rubik::Cube::solved();
    cube.apply(rubik::parseMoves("R U"));

    const rubik::Solver solver;
    const auto result = solver.solve(cube, {
        .mode = rubik::SolveMode::Optimal,
        .maxDepth = 4,
        .profile = rubik::SolveProfile::Default,
    });

    expect(result.status == rubik::SolveStatus::Optimal);
    expect(result.isOptimal);
    expect(result.moveCount == 2);
    expect(result.plan.rootOrderingProfile.find("solution_first=") != std::string::npos);
    expect(result.plan.rootOrderingProfile.find("solution_rank=") != std::string::npos);
}

void testExperimentalRootOrderingReverseTieChangesRootOrderOnly()
{
    rubik::Cube cube = rubik::Cube::solved();
    cube.apply(rubik::parseMoves("R U F"));

    const rubik::Solver solver;
    unsetenv("RUBIK_EXPERIMENTAL_ROOT_ORDERING");
    const auto defaultResult = solver.solve(cube, {
        .mode = rubik::SolveMode::Optimal,
        .maxDepth = 5,
        .threads = 4,
        .profile = rubik::SolveProfile::Default,
    });

    setenv("RUBIK_EXPERIMENTAL_ROOT_ORDERING", "reverse_tie", 1);
    const auto reverseTieResult = solver.solve(cube, {
        .mode = rubik::SolveMode::Optimal,
        .maxDepth = 5,
        .threads = 4,
        .profile = rubik::SolveProfile::Default,
    });
    unsetenv("RUBIK_EXPERIMENTAL_ROOT_ORDERING");

    expect(defaultResult.status == rubik::SolveStatus::Optimal);
    expect(reverseTieResult.status == rubik::SolveStatus::Optimal);
    expect(defaultResult.isOptimal);
    expect(reverseTieResult.isOptimal);
    expect(defaultResult.moveCount == reverseTieResult.moveCount);
    expect(defaultResult.plan.rootOrderingProfile.find("root_ordering_mode=default") != std::string::npos);
    expect(reverseTieResult.plan.rootOrderingProfile.find("root_ordering_mode=reverse_tie") != std::string::npos);
    expect(defaultResult.plan.rootOrderingProfile != reverseTieResult.plan.rootOrderingProfile);
}

void testExperimentalRootOrderingHighBoundFirstChangesRootOrderOnly()
{
    rubik::Cube cube = rubik::Cube::solved();
    cube.apply(rubik::parseMoves("R U F"));

    const rubik::Solver solver;
    unsetenv("RUBIK_EXPERIMENTAL_ROOT_ORDERING");
    const auto defaultResult = solver.solve(cube, {
        .mode = rubik::SolveMode::Optimal,
        .maxDepth = 5,
        .threads = 4,
        .profile = rubik::SolveProfile::Default,
    });

    setenv("RUBIK_EXPERIMENTAL_ROOT_ORDERING", "high_bound_first", 1);
    const auto highBoundResult = solver.solve(cube, {
        .mode = rubik::SolveMode::Optimal,
        .maxDepth = 5,
        .threads = 4,
        .profile = rubik::SolveProfile::Default,
    });
    unsetenv("RUBIK_EXPERIMENTAL_ROOT_ORDERING");

    expect(defaultResult.status == rubik::SolveStatus::Optimal);
    expect(highBoundResult.status == rubik::SolveStatus::Optimal);
    expect(defaultResult.isOptimal);
    expect(highBoundResult.isOptimal);
    expect(defaultResult.moveCount == highBoundResult.moveCount);
    expect(defaultResult.plan.rootOrderingProfile.find("root_ordering_mode=default") != std::string::npos);
    expect(highBoundResult.plan.rootOrderingProfile.find("root_ordering_mode=high_bound_first") != std::string::npos);
    expect(defaultResult.plan.rootOrderingProfile != highBoundResult.plan.rootOrderingProfile);
}

void testParallelOptimalReportsRootSearchProfile()
{
    rubik::Cube cube = rubik::Cube::solved();
    cube.apply(rubik::parseMoves("R U F"));

    const rubik::Solver solver;
    const auto result = solver.solve(cube, {
        .mode = rubik::SolveMode::Optimal,
        .maxDepth = 5,
        .threads = 4,
        .profile = rubik::SolveProfile::Default,
    });

    expect(result.status == rubik::SolveStatus::Optimal);
    expect(result.isOptimal);
    expect(result.plan.rootOrderingProfile.find("root_search=") != std::string::npos);
    expect(result.plan.rootOrderingProfile.find("root_workers=") != std::string::npos);
    expect(result.plan.rootOrderingProfile.find("worker_search=") != std::string::npos);
    expect(result.plan.rootOrderingProfile.find(":found:") != std::string::npos);

    const std::string_view profile(result.plan.rootOrderingProfile);
    const std::size_t rootSearchStart = profile.find("root_search=");
    expect(rootSearchStart != std::string_view::npos);
    const std::size_t firstEntryStart = rootSearchStart + std::string_view("root_search=").size();
    const std::size_t firstEntryEnd = profile.find('|', firstEntryStart);
    const std::string_view firstEntry = firstEntryEnd == std::string_view::npos
        ? profile.substr(firstEntryStart)
        : profile.substr(firstEntryStart, firstEntryEnd - firstEntryStart);
    expect(firstEntry.find(':') != std::string_view::npos);
    const std::size_t firstSeparator = firstEntry.find(':');
    const std::size_t secondSeparator = firstEntry.find(':', firstSeparator + 1);
    expect(secondSeparator != std::string_view::npos);
    const std::size_t thirdSeparator = firstEntry.find(':', secondSeparator + 1);
    expect(thirdSeparator != std::string_view::npos);
}

void testParallelOptimalReportsRootBoundDiagnosticsWhenCollected()
{
    rubik::Cube cube = rubik::Cube::solved();
    cube.apply(rubik::parseMoves("R U F"));

    const rubik::Solver solver;
    const auto result = solver.solve(cube, {
        .mode = rubik::SolveMode::Optimal,
        .maxDepth = 5,
        .threads = 4,
        .profile = rubik::SolveProfile::Default,
        .collectDiagnostics = true,
    });

    expect(result.status == rubik::SolveStatus::Optimal);
    expect(result.isOptimal);
    expect(result.plan.rootOrderingProfile.find("root_bound_diagnostics=") != std::string::npos);
    expect(result.plan.rootOrderingProfile.find(":found:") != std::string::npos);
}

void testExperimentalDeepRootSplitReportsDiagnostics()
{
    rubik::Cube cube = rubik::Cube::solved();
    cube.apply(rubik::parseMoves("R U F D"));

    const rubik::Solver solver;
    setenv("RUBIK_EXPERIMENTAL_DEEP_ROOT_SPLIT", "1", 1);
    const auto result = solver.solve(cube, {
        .mode = rubik::SolveMode::Optimal,
        .maxDepth = 8,
        .threads = 4,
        .profile = rubik::SolveProfile::Default,
        .collectDiagnostics = true,
    });
    unsetenv("RUBIK_EXPERIMENTAL_DEEP_ROOT_SPLIT");

    expect(result.status == rubik::SolveStatus::Optimal);
    expect(result.isOptimal);
    expect(result.moveCount == 4);
    expect(result.plan.rootOrderingProfile.find("deep_root_split=enabled") != std::string::npos);
    expect(result.plan.rootOrderingProfile.find("split_tasks=") != std::string::npos);
    expect(result.plan.rootOrderingProfile.find("worker_search=") != std::string::npos);
}

void testExperimentalAdaptiveDeepSplitReportsDecision()
{
    rubik::Cube cube = rubik::Cube::solved();
    cube.apply(rubik::parseMoves("R U F D"));

    const rubik::Solver solver;
    setenv("RUBIK_EXPERIMENTAL_ADAPTIVE_DEEP_SPLIT", "1", 1);
    const auto result = solver.solve(cube, {
        .mode = rubik::SolveMode::Optimal,
        .maxDepth = 8,
        .threads = 4,
        .profile = rubik::SolveProfile::Default,
        .collectDiagnostics = true,
    });
    unsetenv("RUBIK_EXPERIMENTAL_ADAPTIVE_DEEP_SPLIT");

    expect(result.status == rubik::SolveStatus::Optimal);
    expect(result.isOptimal);
    expect(result.moveCount == 4);
    expect(result.plan.rootOrderingProfile.find("scheduler=adaptive") != std::string::npos);
    expect(result.plan.rootOrderingProfile.find("adaptive_decision=") != std::string::npos);
    expect(result.plan.rootOrderingProfile.find("adaptive_reason=") != std::string::npos);
}

void testAutoOptimalReportsAdaptiveSchedulerDecision()
{
    rubik::Cube cube = rubik::Cube::solved();
    cube.apply(rubik::parseMoves("R U F D"));

    const rubik::Solver solver;
    const auto result = solver.solve(cube, {
        .mode = rubik::SolveMode::Optimal,
        .maxDepth = 8,
        .threads = 4,
        .profile = rubik::SolveProfile::Auto,
        .collectDiagnostics = true,
    });

    expect(result.status == rubik::SolveStatus::Optimal);
    expect(result.isOptimal);
    expect(result.moveCount == 4);
    expect(result.plan.rootOrderingProfile.find("scheduler=adaptive") != std::string::npos);
    expect(result.plan.rootOrderingProfile.find("adaptive_decision=") != std::string::npos);
}

void testAdaptiveSchedulerSelectsV6Lb8StableTailCase()
{
    const auto decision = rubik::detail::chooseAdaptiveDeepSplit({
        .initialLowerBound = 8,
        .maxDepth = 15,
        .threads = 16,
        .strongMinCount = 7,
        .firstMoveDiffers = false,
    });

    expect(decision.scheduler == rubik::detail::OptimalSchedulerDecision::DeepSplit);
    expect(decision.reason == "lb8_stable_mid_strong_min");
}

void testAdaptiveSchedulerKeepsV6Lb8DivergentTailCaseOnRoot()
{
    const auto decision = rubik::detail::chooseAdaptiveDeepSplit({
        .initialLowerBound = 8,
        .maxDepth = 15,
        .threads = 16,
        .strongMinCount = 11,
        .firstMoveDiffers = true,
    });

    expect(decision.scheduler == rubik::detail::OptimalSchedulerDecision::Root);
}

void testAdaptiveSchedulerSelectsV6Depth14ConservativeCase()
{
    const auto decision = rubik::detail::chooseAdaptiveDeepSplit({
        .initialLowerBound = 8,
        .maxDepth = 14,
        .threads = 16,
        .strongMinCount = 11,
        .firstMoveDiffers = true,
    });

    expect(decision.scheduler == rubik::detail::OptimalSchedulerDecision::DeepSplit);
    expect(decision.reason == "depth14_conservative_root");
}

void testAdaptiveSchedulerSelectsV6Lb9LowStrongMinTailCase()
{
    const auto decision = rubik::detail::chooseAdaptiveDeepSplit({
        .initialLowerBound = 9,
        .maxDepth = 15,
        .threads = 16,
        .strongMinCount = 1,
        .firstMoveDiffers = false,
    });

    expect(decision.scheduler == rubik::detail::OptimalSchedulerDecision::DeepSplit);
    expect(decision.reason == "lb9_low_strong_min");
}

void testAdaptiveRootOrderingSelectsV6Lb9StableMidStrongMinCase()
{
    const auto decision = rubik::detail::chooseAdaptiveRootOrdering({
        .initialLowerBound = 9,
        .maxDepth = 15,
        .threads = 16,
        .strongMinCount = 4,
        .firstMoveDiffers = false,
    });

    expect(decision == rubik::detail::AdaptiveRootOrderingDecision::ReverseTie);
}

void testAdaptiveRootOrderingKeepsHighStrongMinCaseOnDefault()
{
    const auto decision = rubik::detail::chooseAdaptiveRootOrdering({
        .initialLowerBound = 9,
        .maxDepth = 15,
        .threads = 16,
        .strongMinCount = 14,
        .firstMoveDiffers = false,
    });

    expect(decision == rubik::detail::AdaptiveRootOrderingDecision::Default);
}

void testSolveMemoryLimit()
{
    rubik::Cube cube = rubik::Cube::solved();
    cube.apply(rubik::parseMoves("R"));

    const rubik::Solver solver;
    const auto result = solver.solve(cube, {
        .mode = rubik::SolveMode::Optimal,
        .maxDepth = 4,
        .maxMemoryBytes = 1,
        .profile = rubik::SolveProfile::Default,
    });

    assert(result.status == rubik::SolveStatus::MemoryLimitExceeded);
    assert(!result.isOptimal);
    assert(result.moveCount == -1);
    assert(result.nodesExpanded == 0);
    assert(result.memoryUsedBytes > 1);
}

void testFastTinyScramble()
{
    rubik::Cube cube = rubik::Cube::solved();
    cube.apply(rubik::parseMoves("R U F"));

    const rubik::Solver solver;
    const auto result = solver.solve(cube, {
        .mode = rubik::SolveMode::Fast,
        .maxDepth = 8,
    });

    assert(result.status == rubik::SolveStatus::Found);
    assert(!result.isOptimal);
    assert(result.moveCount > 0);

    cube.apply(result.moves);
    assert(cube.isSolved());
}

void testPhase1TinyScramble()
{
    rubik::Cube cube = rubik::Cube::solved();
    cube.apply(rubik::parseMoves("R U F D L B"));

    const auto result = rubik::solvePhase1(cube, {
        .maxDepth = 8,
    });

    assert(result.status == rubik::SolveStatus::Found);
    assert(result.moveCount > 0);

    cube.apply(result.moves);
    assert(rubik::isPhase1Solved(cube));
}

void testPhase1Candidates()
{
    rubik::Cube cube = rubik::Cube::solved();
    cube.apply(rubik::parseMoves("R U F D L B"));

    const auto result = rubik::findPhase1Candidates(cube, {
        .maxDepth = 9,
        .maxCandidates = 3,
    });

    assert(result.status == rubik::SolveStatus::Found);
    assert(!result.candidates.empty());
    assert(result.candidates.size() <= 3);

    for (const auto& candidate : result.candidates) {
        rubik::Cube candidateCube = cube;
        candidateCube.apply(candidate);
        assert(rubik::isPhase1Solved(candidateCube));
    }
}

void testPhase2TinyScramble()
{
    rubik::Cube cube = rubik::Cube::solved();
    cube.apply(rubik::parseMoves("U R2 F2 D L2 B2 U'"));

    assert(rubik::isPhase1Solved(cube));

    const auto result = rubik::solvePhase2(cube, {
        .maxDepth = 8,
    });

    assert(result.status == rubik::SolveStatus::Found);
    assert(result.moveCount > 0);

    cube.apply(result.moves);
    assert(cube.isSolved());
}

void testExperimentalPhaseAliases()
{
    rubik::Cube cube = rubik::Cube::solved();
    cube.apply(rubik::parseMoves("R U F D L B"));

    const auto phase1 = rubik::experimental::solvePhase1(cube, {
        .maxDepth = 8,
    });

    assert(phase1.status == rubik::SolveStatus::Found);
    cube.apply(phase1.moves);
    assert(rubik::experimental::isPhase1Solved(cube));

    const auto phase2 = rubik::experimental::solvePhase2(cube, {
        .maxDepth = 18,
    });

    assert(phase2.status == rubik::SolveStatus::Found ||
        phase2.status == rubik::SolveStatus::Solved);
    cube.apply(phase2.moves);
    assert(cube.isSolved());
}

} // namespace

int main()
{
    testVersionMetadata();
    testV3AdaptiveApiDefaults();
    testAutoPlannerRejectsFastMode();
    testAutoPlannerInternalDecisionMatchesPublicPlan();
    testAutoPlannerUsesPerformanceForShallowOptimal();
    testAutoPlannerUsesLargeLocalForDeepOptimal();
    testAutoPlannerFallsBackWhenLargeLocalMemoryIsTooSmall();
    testAutoPlannerRejectsWhenMemoryIsTooSmallForAuto();
    testAutoPlannerReportsColdLargeLocalRequirement();
    testAutoOptimalPlanUsesRealCacheWarmth();
    testAutoPlannerFallsBackForColdTightTimeout();
    testAutoOptimalPlanUsesRealTimeoutCacheWarmth();
    testAutoPlannerReportsFullTailPayload();
    testAutoStrongMoveOrderingPolicy();
    testAutoSolveReportsPlan();
    testAutoFastSolveIsUnsupported();
    testPrepareCacheDryRun();
    testPrepareCacheDryRunReportsColdCache();
    testPrepareCacheRequireWarmReportsColdCache();
    testRequireWarmSolveRejectsColdCache();
    testSolvedCube();
    testStructuredStickerInput();
    testPhysicalValidation();
    testCoordinates();
    testCubieMoveMatchesStickerMove();
    testOrientationMoveTables();
    testCoordinateMoveTablesAcrossSequences();
    testCubeRotationSymmetries();
    testCubeRotationSymmetriesPreserveOptimalDistance();
    testCoordinateSymmetryTables();
    testCoordinateSymmetryReductions();
    testCombinedCoordinateSymmetryReduction();
    testCornerOrientationSliceSymmetryReduction();
    testEdgeOrientationSliceSymmetryReduction();
    testReducedSymmetryPruningTables();
    testCombinedReducedSymmetryPruningTable();
    testExperimentalSymmetryLowerBoundDoesNotWeaken();
    testExperimentalThreePhase1LowerBoundDoesNotWeaken();
    testThreePhase1Policy();
    testCornerOrientationSliceReducedSymmetryPruningTable();
    testEdgeOrientationSliceReducedSymmetryPruningTable();
    testOrientationPruningTables();
    testTableProfileManifest();
    testMoveInverse();
    testFourQuarterTurns();
    testParseMoves();
    testOptimalTinyScramble();
    testOptimalThreadedTinyScramble();
    testOptimalRootOrderingReportsSolutionRank();
    testExperimentalRootOrderingReverseTieChangesRootOrderOnly();
    testExperimentalRootOrderingHighBoundFirstChangesRootOrderOnly();
    testParallelOptimalReportsRootSearchProfile();
    testParallelOptimalReportsRootBoundDiagnosticsWhenCollected();
    testExperimentalDeepRootSplitReportsDiagnostics();
    testExperimentalAdaptiveDeepSplitReportsDecision();
    testAutoOptimalReportsAdaptiveSchedulerDecision();
    testAdaptiveSchedulerSelectsV6Lb8StableTailCase();
    testAdaptiveSchedulerKeepsV6Lb8DivergentTailCaseOnRoot();
    testAdaptiveSchedulerSelectsV6Depth14ConservativeCase();
    testAdaptiveSchedulerSelectsV6Lb9LowStrongMinTailCase();
    testAdaptiveRootOrderingSelectsV6Lb9StableMidStrongMinCase();
    testAdaptiveRootOrderingKeepsHighStrongMinCaseOnDefault();
    testSolveMemoryLimit();
    testFastTinyScramble();
    testPhase1TinyScramble();
    testPhase1Candidates();
    testPhase2TinyScramble();
    testExperimentalPhaseAliases();

    std::cout << "rubik tests passed\n";
    return 0;
}
