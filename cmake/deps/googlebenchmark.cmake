# Google Benchmark — FetchContent when BUILD_TESTING=ON
# https://github.com/google/benchmark

if(TARGET benchmark::benchmark)
    return()
endif()

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    dailyboy_googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG "${DAILYBOY_GBENCH_GIT_TAG}"
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(dailyboy_googlebenchmark)

message(STATUS "deps: Google Benchmark ${DAILYBOY_GBENCH_GIT_TAG}")
