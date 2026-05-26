cmake_minimum_required(VERSION 3.20)

project(rubik_cube_library VERSION 1.0.0 LANGUAGES CXX)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(RUBIK_BUILD_EXAMPLES "Build Rubik Cube Library examples" ON)
option(RUBIK_INSTALL_CLI "Install Rubik Cube Library CLI tools" ON)
option(RUBIK_ENABLE_NATIVE "Build with host-specific CPU tuning flags" OFF)
option(RUBIK_ENABLE_SANITIZERS "Build with address and undefined behavior sanitizers" OFF)

if (RUBIK_ENABLE_NATIVE)
    if (MSVC)
        add_compile_options(/arch:AVX2)
    else()
        add_compile_options(-march=native)
    endif()
endif()

if (RUBIK_ENABLE_SANITIZERS)
    if (MSVC)
        message(FATAL_ERROR "RUBIK_ENABLE_SANITIZERS is not supported with MSVC yet")
    else()
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()

add_library(rubik
    src/coordinates.cpp
    src/cubie_cube.cpp
    src/cube.cpp
    src/move_tables.cpp
    src/move.cpp
    src/phase1.cpp
    src/phase2.cpp
    src/pruning_tables.cpp
    src/solver.cpp
    src/symmetry.cpp
    src/symmetry_coordinates.cpp
    src/symmetry_pruning.cpp
    src/table_profiles.cpp
)

target_compile_features(rubik PUBLIC cxx_std_20)
target_include_directories(rubik
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if (MSVC)
    target_compile_options(rubik PRIVATE /W4)
else()
    target_compile_options(rubik PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()

add_executable(rubik_tests
    tests/rubik_tests.cpp
)

target_link_libraries(rubik_tests PRIVATE rubik)
add_test(NAME rubik_tests COMMAND rubik_tests)

add_executable(rubik-solve
    apps/rubik_solve.cpp
)
target_link_libraries(rubik-solve PRIVATE rubik)
add_test(
    NAME cli_solve_solved
    COMMAND rubik-solve UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB --timeout-ms 1000
)
set_tests_properties(cli_solve_solved PROPERTIES
    PASS_REGULAR_EXPRESSION "status: Solved"
)
add_test(
    NAME cli_solve_invalid
    COMMAND rubik-solve not-a-cube
)
set_tests_properties(cli_solve_invalid PROPERTIES
    WILL_FAIL TRUE
)

add_executable(rubik-bench
    apps/rubik_bench.cpp
)
target_link_libraries(rubik-bench PRIVATE rubik)
add_custom_target(rubik-benchmark-smoke
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_benchmark_suite.sh
        --suite smoke
        --cache-mode warm
        --build-dir ${CMAKE_CURRENT_BINARY_DIR}
        --cache-dir /tmp/rubik_cube_library_benchmark_cache
        --output-dir ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results
    DEPENDS rubik-bench
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running smoke benchmark suite"
)
add_custom_target(rubik-benchmark-smoke-cold
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_benchmark_suite.sh
        --suite smoke
        --cache-mode cold
        --build-dir ${CMAKE_CURRENT_BINARY_DIR}
        --cache-dir /tmp/rubik_cube_library_benchmark_cold_cache
        --output-dir ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results
    DEPENDS rubik-bench
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running cold-cache smoke benchmark suite"
)
add_custom_target(rubik-benchmark-profile-smoke
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_benchmark_suite.sh
        --suite profile-smoke
        --cache-mode warm
        --build-dir ${CMAKE_CURRENT_BINARY_DIR}
        --cache-dir /tmp/rubik_cube_library_profile_benchmark_cache
        --output-dir ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results/profile-smoke
    DEPENDS rubik-bench
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running embedded/default/performance smoke benchmark suite"
)
add_custom_target(rubik-benchmark-profile-realistic
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_benchmark_suite.sh
        --suite profile-realistic
        --cache-mode warm
        --build-dir ${CMAKE_CURRENT_BINARY_DIR}
        --cache-dir /tmp/rubik_cube_library_profile_realistic_cache
        --output-dir ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results/profile-realistic
        --realistic-fast-count 100
        --realistic-opt12-count 20
        --realistic-opt13-count 10
    DEPENDS rubik-bench
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running embedded/default/performance realistic benchmark suite"
)
add_custom_target(rubik-benchmark-profile-realistic-gates
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_benchmark_gates.sh
        --summary-file ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results/profile-realistic/warm_profile_realistic_summary.csv
        --gate embedded,fast,random_depth_20_count_100,100,350,500,700
        --gate embedded,optimal,random_depth_13_count_10,10,4000,4000,4000
        --gate default,optimal,random_depth_13_count_10,10,2500,2500,2500
        --gate performance,optimal,random_depth_13_count_10,10,2500,2500,2500
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Checking profile-realistic benchmark gates"
)
add_custom_target(rubik-benchmark-embedded-multiseed
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_benchmark_suite.sh
        --suite embedded-multiseed
        --cache-mode warm
        --build-dir ${CMAKE_CURRENT_BINARY_DIR}
        --cache-dir /tmp/rubik_cube_library_embedded_multiseed_cache
        --output-dir ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results/embedded-multiseed
        --seeds 12345,20260525,42,314159,271828,987654321,7,99,123456789,424242,8675309,20240525
        --realistic-fast-count 100
        --fast-max-depth 28
        --realistic-opt13-count 10
    DEPENDS rubik-bench
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running embedded multiseed robustness benchmark suite"
)
add_custom_target(rubik-benchmark-embedded-multiseed-gates
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_benchmark_gates.sh
        --summary-file ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results/embedded-multiseed/warm_embedded_multiseed_summary.csv
        --gate embedded,fast,random_seed_12345_depth_20_count_100,100,350,500,700
        --gate embedded,optimal,random_seed_12345_depth_13_count_10,10,4000,4000,4000
        --gate embedded,fast,random_seed_20260525_depth_20_count_100,100,350,500,700
        --gate embedded,optimal,random_seed_20260525_depth_13_count_10,10,4000,4000,4000
        --gate embedded,fast,random_seed_42_depth_20_count_100,100,350,500,700
        --gate embedded,optimal,random_seed_42_depth_13_count_10,10,4000,4000,4000
        --gate embedded,fast,random_seed_314159_depth_20_count_100,100,350,500,700
        --gate embedded,optimal,random_seed_314159_depth_13_count_10,10,4000,4000,4000
        --gate embedded,fast,random_seed_271828_depth_20_count_100,100,350,500,700
        --gate embedded,optimal,random_seed_271828_depth_13_count_10,10,4000,4000,4000
        --gate embedded,fast,random_seed_987654321_depth_20_count_100,100,350,500,700
        --gate embedded,optimal,random_seed_987654321_depth_13_count_10,10,4000,4000,4000
        --gate embedded,fast,random_seed_7_depth_20_count_100,100,350,500,700
        --gate embedded,optimal,random_seed_7_depth_13_count_10,10,4000,4000,4000
        --gate embedded,fast,random_seed_99_depth_20_count_100,100,350,500,700
        --gate embedded,optimal,random_seed_99_depth_13_count_10,10,4000,4000,4000
        --gate embedded,fast,random_seed_123456789_depth_20_count_100,100,350,500,700
        --gate embedded,optimal,random_seed_123456789_depth_13_count_10,10,4000,4000,4000
        --gate embedded,fast,random_seed_424242_depth_20_count_100,100,350,500,700
        --gate embedded,optimal,random_seed_424242_depth_13_count_10,10,4000,4000,4000
        --gate embedded,fast,random_seed_8675309_depth_20_count_100,100,350,500,700
        --gate embedded,optimal,random_seed_8675309_depth_13_count_10,10,4000,4000,4000
        --gate embedded,fast,random_seed_20240525_depth_20_count_100,100,350,500,700
        --gate embedded,optimal,random_seed_20240525_depth_13_count_10,10,4000,4000,4000
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Checking embedded multiseed benchmark gates"
)
add_custom_target(rubik-benchmark-optimal-stress
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_benchmark_suite.sh
        --suite optimal-stress
        --cache-mode warm
        --build-dir ${CMAKE_CURRENT_BINARY_DIR}
        --cache-dir /tmp/rubik_cube_library_optimal_stress_cache
        --output-dir ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results/optimal-stress
        --seeds 12345,20260525,42
        --realistic-opt13-count 10
    DEPENDS rubik-bench
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running optimal stress benchmark suite"
)
add_custom_target(rubik-benchmark-optimal-stress-gates
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_benchmark_gates.sh
        --summary-file ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results/optimal-stress/warm_optimal_stress_summary.csv
        --gate embedded,optimal,random_seed_12345_depth_13_count_10,10,4000,4000,4000
        --gate embedded,optimal,random_seed_20260525_depth_13_count_10,10,4000,4000,4000
        --gate embedded,optimal,random_seed_42_depth_13_count_10,10,4000,4000,4000
        --gate default,optimal,random_seed_12345_depth_13_count_10,10,2500,2500,2500
        --gate default,optimal,random_seed_20260525_depth_13_count_10,10,2500,2500,2500
        --gate default,optimal,random_seed_42_depth_13_count_10,10,2500,2500,2500
        --gate performance,optimal,random_seed_12345_depth_13_count_10,10,2500,2500,2500
        --gate performance,optimal,random_seed_20260525_depth_13_count_10,10,2500,2500,2500
        --gate performance,optimal,random_seed_42_depth_13_count_10,10,2500,2500,2500
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Checking optimal stress benchmark gates"
)
add_custom_target(rubik-benchmark-optimal-tail-cases
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_benchmark_suite.sh
        --suite optimal-tail-cases
        --cache-mode warm
        --build-dir ${CMAKE_CURRENT_BINARY_DIR}
        --cache-dir /tmp/rubik_cube_library_optimal_tail_cache
        --output-dir ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results/optimal-tail-cases
    DEPENDS rubik-bench
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running embedded optimal tail-case diagnostics"
)
add_custom_target(rubik-benchmark-optimal-tail-cases-gates
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_benchmark_gates.sh
        --summary-file ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results/optimal-tail-cases/warm_optimal_tail_cases_summary.csv
        --gate embedded,optimal,random_seed_12345_index_4_depth_13,1,4000,4000,4000
        --gate embedded,optimal,random_seed_42_index_2_depth_13,1,4000,4000,4000
        --gate embedded,optimal,random_seed_42_index_1_depth_13,1,4000,4000,4000
        --gate embedded,optimal,random_seed_20260525_index_7_depth_13,1,4000,4000,4000
        --gate embedded,optimal,random_seed_12345_index_2_depth_13,1,4000,4000,4000
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Checking embedded optimal tail-case gates"
)
add_custom_target(rubik-benchmark-optimal-deep-probe
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_benchmark_suite.sh
        --suite optimal-deep-probe
        --cache-mode warm
        --build-dir ${CMAKE_CURRENT_BINARY_DIR}
        --cache-dir /tmp/rubik_cube_library_optimal_deep_probe_cache
        --output-dir ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results/optimal-deep-probe
        --seeds 12345
        --deep-opt14-count 2
        --deep-opt15-count 1
    DEPENDS rubik-bench
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running optimal deep probe benchmark suite"
)
add_custom_target(rubik-benchmark-optimal-large-local
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_benchmark_suite.sh
        --suite optimal-large-local
        --cache-mode warm
        --build-dir ${CMAKE_CURRENT_BINARY_DIR}
        --cache-dir /tmp/rubik_cube_library_optimal_large_local_cache
        --output-dir ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results/optimal-large-local
        --seeds 12345,20260525,42,314159,271828,987654321,7,99,123456789,424242,8675309,20240525,111,222,333,444,555,666,777,888,999,13579,24680,112358
        --deep-opt15-count 1
        --threads 4
        --max-memory-mb 2048
    DEPENDS rubik-bench
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running large local optimal depth-15 benchmark suite"
)
add_custom_target(rubik-benchmark-optimal-large-local-gates
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_benchmark_gates.sh
        --summary-file ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results/optimal-large-local/warm_optimal_large_local_summary.csv
        --gate large-local,optimal,random_seed_12345_depth_15_count_1,1,30000,30000,30000
        --gate large-local,optimal,random_seed_20260525_depth_15_count_1,1,30000,30000,30000
        --gate large-local,optimal,random_seed_42_depth_15_count_1,1,30000,30000,30000
        --gate large-local,optimal,random_seed_314159_depth_15_count_1,1,30000,30000,30000
        --gate large-local,optimal,random_seed_271828_depth_15_count_1,1,30000,30000,30000
        --gate large-local,optimal,random_seed_987654321_depth_15_count_1,1,30000,30000,30000
        --gate large-local,optimal,random_seed_7_depth_15_count_1,1,30000,30000,30000
        --gate large-local,optimal,random_seed_99_depth_15_count_1,1,30000,30000,30000
        --gate large-local,optimal,random_seed_123456789_depth_15_count_1,1,30000,30000,30000
        --gate large-local,optimal,random_seed_424242_depth_15_count_1,1,30000,30000,30000
        --gate large-local,optimal,random_seed_8675309_depth_15_count_1,1,30000,30000,30000
        --gate large-local,optimal,random_seed_20240525_depth_15_count_1,1,30000,30000,30000
        --gate large-local,optimal,random_seed_111_depth_15_count_1,1,30000,30000,30000
        --gate large-local,optimal,random_seed_222_depth_15_count_1,1,30000,30000,30000
        --gate large-local,optimal,random_seed_333_depth_15_count_1,1,30000,30000,30000
        --gate large-local,optimal,random_seed_444_depth_15_count_1,1,30000,30000,30000
        --gate large-local,optimal,random_seed_555_depth_15_count_1,1,30000,30000,30000
        --gate large-local,optimal,random_seed_666_depth_15_count_1,1,30000,30000,30000
        --gate large-local,optimal,random_seed_777_depth_15_count_1,1,30000,30000,30000
        --gate large-local,optimal,random_seed_888_depth_15_count_1,1,30000,30000,30000
        --gate large-local,optimal,random_seed_999_depth_15_count_1,1,30000,30000,30000
        --gate large-local,optimal,random_seed_13579_depth_15_count_1,1,30000,30000,30000
        --gate large-local,optimal,random_seed_24680_depth_15_count_1,1,30000,30000,30000
        --gate large-local,optimal,random_seed_112358_depth_15_count_1,1,30000,30000,30000
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Checking large local optimal depth-15 gates"
)
add_custom_target(rubik-benchmark-optimal-large-local-tail-8threads
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_benchmark_suite.sh
        --suite optimal-large-local
        --cache-mode warm
        --build-dir ${CMAKE_CURRENT_BINARY_DIR}
        --cache-dir /tmp/rubik_cube_library_optimal_large_local_cache
        --output-dir ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results/optimal-large-local-tail-8threads
        --seeds 987654321,424242,666,555,99,888
        --deep-opt15-count 1
        --threads 8
        --max-memory-mb 2048
    DEPENDS rubik-bench
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running large local optimal depth-15 tail benchmark with 8 threads"
)
add_custom_target(rubik-benchmark-optimal-large-local-tail-8threads-gates
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_benchmark_gates.sh
        --summary-file ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results/optimal-large-local-tail-8threads/warm_optimal_large_local_summary.csv
        --gate large-local,optimal,random_seed_987654321_depth_15_count_1,1,18000,18000,18000
        --gate large-local,optimal,random_seed_424242_depth_15_count_1,1,18000,18000,18000
        --gate large-local,optimal,random_seed_666_depth_15_count_1,1,18000,18000,18000
        --gate large-local,optimal,random_seed_555_depth_15_count_1,1,18000,18000,18000
        --gate large-local,optimal,random_seed_99_depth_15_count_1,1,18000,18000,18000
        --gate large-local,optimal,random_seed_888_depth_15_count_1,1,18000,18000,18000
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Checking large local optimal 8-thread tail gates"
)
add_custom_target(rubik-benchmark-embedded-fast-tail-cases
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_benchmark_suite.sh
        --suite embedded-fast-tail-cases
        --cache-mode warm
        --build-dir ${CMAKE_CURRENT_BINARY_DIR}
        --cache-dir /tmp/rubik_cube_library_embedded_fast_tail_cache
        --output-dir ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results/embedded-fast-tail-cases
    DEPENDS rubik-bench
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running embedded fast tail-case diagnostics"
)
add_custom_target(rubik-benchmark-embedded-fast-failures
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_benchmark_suite.sh
        --suite embedded-fast-failures
        --cache-mode warm
        --build-dir ${CMAKE_CURRENT_BINARY_DIR}
        --cache-dir /tmp/rubik_cube_library_embedded_fast_failure_cache
        --output-dir ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results/embedded-fast-failures
    DEPENDS rubik-bench
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running embedded fast failure diagnostics"
)
add_custom_target(rubik-benchmark-fast100
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_benchmark_suite.sh
        --suite fast-100
        --cache-mode warm
        --build-dir ${CMAKE_CURRENT_BINARY_DIR}
        --cache-dir /tmp/rubik_cube_library_benchmark_cache
        --output-dir ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results
    DEPENDS rubik-bench
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running fast random 100 benchmark suite"
)
add_custom_target(rubik-benchmark-optimal-depth
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_benchmark_suite.sh
        --suite optimal-depth
        --cache-mode warm
        --build-dir ${CMAKE_CURRENT_BINARY_DIR}
        --cache-dir /tmp/rubik_cube_library_benchmark_cache
        --output-dir ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results
    DEPENDS rubik-bench
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running optimal deterministic depth benchmark suite"
)
add_custom_target(rubik-benchmark-optimal-ab
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_optimal_ab.sh
        --build-dir ${CMAKE_CURRENT_BINARY_DIR}
        --cache-dir /tmp/rubik_cube_library_optimal_ab_cache
        --output-dir ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results/optimal-ab
        --repetitions 3
        --case-set random
        --random-count 8
        --random-depth 12
        --random-seed 20260525
        --timeout-ms 5000
        --max-depth 12
    DEPENDS rubik-bench
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running optimal baseline vs three-phase A/B benchmark"
)
add_custom_target(rubik-benchmark-v2-optimal-baseline
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_v2_optimal_baseline.sh
        --build-dir ${CMAKE_CURRENT_BINARY_DIR}
        --cache-dir /tmp/rubik_cube_library_v2_optimal_baseline_cache
        --output-dir ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results/v2-optimal-baseline
    DEPENDS rubik-bench
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running V2 optimal baseline benchmark suite"
)
add_custom_target(rubik-benchmark-optimal-tail-experiments
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/run_optimal_tail_experiments.sh
        --build-dir ${CMAKE_CURRENT_BINARY_DIR}
        --cache-dir /tmp/rubik_cube_library_optimal_tail_experiments_cache
        --output-dir ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results/optimal-tail-experiments
    DEPENDS rubik-bench
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running optimal tail-case experiment matrix"
)
add_test(
    NAME cli_bench_deterministic_smoke
    COMMAND rubik-bench --mode optimal --timeout-ms 1000 --max-depth 1 --max-case-depth 1
)
set_tests_properties(cli_bench_deterministic_smoke PROPERTIES
    PASS_REGULAR_EXPRESSION "benchmark,three_phase1_effective,true"
)
add_test(
    NAME cli_bench_random_smoke
    COMMAND rubik-bench --mode fast --timeout-ms 5000 --max-depth 24 --case-set random --random-count 1 --random-depth 8 --random-seed 42
)
set_tests_properties(cli_bench_random_smoke PROPERTIES
    PASS_REGULAR_EXPRESSION "random_42_1"
)
add_test(
    NAME cli_bench_large_local_profile
    COMMAND rubik-bench --mode optimal --profile large-local --report-policy
)
set_tests_properties(cli_bench_large_local_profile PROPERTIES
    PASS_REGULAR_EXPRESSION "benchmark,profile,large-local"
)
add_test(
    NAME cli_bench_large_local_policy
    COMMAND rubik-bench --mode optimal --profile large-local --report-policy
)
set_tests_properties(cli_bench_large_local_policy PROPERTIES
    PASS_REGULAR_EXPRESSION "benchmark,corner_up_edge_bounds,profile_enabled"
)
add_test(
    NAME cli_solve_large_local_profile
    COMMAND rubik-solve UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB --mode optimal --profile large-local --max-depth 1 --timeout-ms 1000
)
set_tests_properties(cli_solve_large_local_profile PROPERTIES
    PASS_REGULAR_EXPRESSION "status: Solved"
)
add_test(
    NAME cli_bench_large_local_memory_report
    COMMAND rubik-bench --report-memory
)
set_tests_properties(cli_bench_large_local_memory_report PROPERTIES
    PASS_REGULAR_EXPRESSION "memory_summary,large_local_optimal,total_bytes"
)
add_test(
    NAME cli_bench_rejects_invalid_profile
    COMMAND bash -c "output=\"$($1 --profile typo --report-policy 2>&1)\"; status=$?; printf '%s\n' \"$output\"; test $status -ne 0 && printf '%s\n' \"$output\" | grep -q 'Invalid profile'" _ $<TARGET_FILE:rubik-bench>
)
add_test(
    NAME cli_solve_rejects_invalid_profile
    COMMAND bash -c "output=\"$($1 UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB --profile typo 2>&1)\"; status=$?; printf '%s\n' \"$output\"; test $status -ne 0 && printf '%s\n' \"$output\" | grep -q 'Invalid profile'" _ $<TARGET_FILE:rubik-solve>
)
add_test(
    NAME cli_bench_rejects_invalid_mode
    COMMAND bash -c "output=\"$($1 --mode typo --report-policy 2>&1)\"; status=$?; printf '%s\n' \"$output\"; test $status -ne 0 && printf '%s\n' \"$output\" | grep -q 'Invalid mode'" _ $<TARGET_FILE:rubik-bench>
)
add_test(
    NAME cli_solve_rejects_invalid_mode
    COMMAND bash -c "output=\"$($1 UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB --mode typo 2>&1)\"; status=$?; printf '%s\n' \"$output\"; test $status -ne 0 && printf '%s\n' \"$output\" | grep -q 'Invalid mode'" _ $<TARGET_FILE:rubik-solve>
)
add_test(
    NAME cli_bench_rejects_invalid_case_set
    COMMAND bash -c "output=\"$($1 --case-set typo --report-policy 2>&1)\"; status=$?; printf '%s\n' \"$output\"; test $status -ne 0 && printf '%s\n' \"$output\" | grep -q 'Invalid case set'" _ $<TARGET_FILE:rubik-bench>
)
add_test(
    NAME cli_solve_rejects_invalid_max_depth
    COMMAND bash -c "output=\"$($1 UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB --max-depth abc 2>&1)\"; status=$?; printf '%s\n' \"$output\"; test $status -ne 0 && printf '%s\n' \"$output\" | grep -q 'Invalid max-depth'" _ $<TARGET_FILE:rubik-solve>
)
add_test(
    NAME cli_bench_rejects_invalid_threads
    COMMAND bash -c "output=\"$($1 --threads abc --report-policy 2>&1)\"; status=$?; printf '%s\n' \"$output\"; test $status -ne 0 && printf '%s\n' \"$output\" | grep -q 'Invalid threads'" _ $<TARGET_FILE:rubik-bench>
)
add_test(
    NAME cli_bench_rejects_invalid_random_count
    COMMAND bash -c "output=\"$($1 --random-count abc --report-policy 2>&1)\"; status=$?; printf '%s\n' \"$output\"; test $status -ne 0 && printf '%s\n' \"$output\" | grep -q 'Invalid random-count'" _ $<TARGET_FILE:rubik-bench>
)
add_test(
    NAME cli_bench_percentile_summary
    COMMAND rubik-bench --mode fast --timeout-ms 5000 --max-depth 24 --case-set random --random-count 1 --random-depth 8 --random-seed 42
)
set_tests_properties(cli_bench_percentile_summary PROPERTIES
    PASS_REGULAR_EXPRESSION "summary,p99_elapsed_ms"
)
add_test(
    NAME cli_bench_random_start_index_smoke
    COMMAND rubik-bench --mode fast --timeout-ms 5000 --max-depth 24 --case-set random --random-count 1 --random-depth 8 --random-seed 42 --random-start-index 3
)
set_tests_properties(cli_bench_random_start_index_smoke PROPERTIES
    PASS_REGULAR_EXPRESSION "random_42_3"
)
add_test(
    NAME cli_bench_fast_diagnostics_smoke
    COMMAND rubik-bench --mode fast --timeout-ms 5000 --max-depth 24 --case-set random --random-count 1 --random-depth 8 --random-seed 42 --diagnose-fast
)
set_tests_properties(cli_bench_fast_diagnostics_smoke PROPERTIES
    PASS_REGULAR_EXPRESSION "diagnostic_phase1"
)
add_test(
    NAME cli_bench_symmetry_report
    COMMAND rubik-bench --report-symmetry
)
set_tests_properties(cli_bench_symmetry_report PROPERTIES
    PASS_REGULAR_EXPRESSION "symmetry_coordinate,corner_orientation"
)
add_test(
    NAME cli_bench_cache_report
    COMMAND rubik-bench --report-cache
)
set_tests_properties(cli_bench_cache_report PROPERTIES
    PASS_REGULAR_EXPRESSION "cache_summary,total_bytes"
)
add_test(
    NAME cli_bench_memory_report
    COMMAND rubik-bench --report-memory
)
set_tests_properties(cli_bench_memory_report PROPERTIES
    PASS_REGULAR_EXPRESSION "memory_summary,default_optimal,total_bytes"
)
add_test(
    NAME cli_bench_lower_bound_report
    COMMAND rubik-bench --benchmark-lower-bound --max-case-depth 1 --lower-bound-iterations 1
)
set_tests_properties(cli_bench_lower_bound_report PROPERTIES
    PASS_REGULAR_EXPRESSION "lower_bound_summary,total_evaluations,1"
)
add_test(
    NAME cli_bench_optimal_diagnostics_smoke
    COMMAND rubik-bench --mode optimal --timeout-ms 1000 --max-depth 1 --max-case-depth 1 --diagnose-optimal
)
set_tests_properties(cli_bench_optimal_diagnostics_smoke PROPERTIES
    PASS_REGULAR_EXPRESSION "diagnostic_optimal_bounds"
)
add_test(
    NAME benchmark_gates_pass
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_benchmark_gates.sh
        --summary-file ${CMAKE_CURRENT_SOURCE_DIR}/tests/benchmark_gate_summary.csv
        --gate embedded,fast,random_depth_20_count_100,100,300,572,655
        --gate embedded,optimal,random_depth_13_count_10,10,14667,14667,14667
)
set_tests_properties(benchmark_gates_pass PROPERTIES
    PASS_REGULAR_EXPRESSION "benchmark gate passed"
)
add_test(
    NAME benchmark_gates_fail
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_benchmark_gates.sh
        --summary-file ${CMAKE_CURRENT_SOURCE_DIR}/tests/benchmark_gate_summary.csv
        --gate embedded,fast,random_depth_20_count_100,100,299,572,655
)
set_tests_properties(benchmark_gates_fail PROPERTIES
    WILL_FAIL TRUE
)
add_test(
    NAME extract_slowest_cases
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/extract_slowest_cases.sh
        --input-dir ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/benchmark-results
        --limit 2
)
set_tests_properties(extract_slowest_cases PROPERTIES
    PASS_REGULAR_EXPRESSION "elapsed_ms,source_file,benchmark,mode,profile,slowest_rank,case_name,case_depth,scramble,status,move_count,nodes_expanded,solution.*900,.*sample_b.csv,fixture_tail,optimal,embedded,1,random_42_2,13,R U R',Optimal,13,9000,R U"
)

if (RUBIK_BUILD_EXAMPLES)
    add_executable(example-validate-input
        examples/validate_input.cpp
    )
    target_link_libraries(example-validate-input PRIVATE rubik)
    add_test(NAME example_validate_input COMMAND example-validate-input)

    add_executable(example-solve-fast
        examples/solve_fast.cpp
    )
    target_link_libraries(example-solve-fast PRIVATE rubik)
    add_test(NAME example_solve_fast COMMAND example-solve-fast)

    add_executable(example-solve-optimal
        examples/solve_optimal.cpp
    )
    target_link_libraries(example-solve-optimal PRIVATE rubik)
    add_test(NAME example_solve_optimal COMMAND example-solve-optimal)

    add_executable(example-cache-setup
        examples/cache_setup.cpp
    )
    target_link_libraries(example-cache-setup PRIVATE rubik)
    add_test(NAME example_cache_setup COMMAND example-cache-setup)
endif()

add_custom_target(rubik-check-install-consumer
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_install_consumer.sh
        --build-dir ${CMAKE_CURRENT_BINARY_DIR}
        --install-prefix ${CMAKE_CURRENT_BINARY_DIR}/install-consumer-prefix
        --consumer-build-dir ${CMAKE_CURRENT_BINARY_DIR}/install-consumer-build
    DEPENDS rubik rubik-solve rubik-bench
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Checking installed package from a standalone CMake consumer"
)

install(
    TARGETS rubik
    EXPORT rubikTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if (RUBIK_INSTALL_CLI)
    install(
        TARGETS rubik-solve rubik-bench
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

install(
    EXPORT rubikTargets
    NAMESPACE rubik::
    FILE rubikTargets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rubik
)

configure_package_config_file(
    cmake/rubikConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/rubikConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rubik
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/rubikConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/rubikConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/rubikConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rubik
)
