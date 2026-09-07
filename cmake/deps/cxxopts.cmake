# cxxopts — C++ CLI argument parsing
# https://github.com/jarro2783/cxxopts

if(TARGET cxxopts::cxxopts)
    return()
endif()

set(CXXOPTS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CXXOPTS_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    dailyboy_cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    GIT_TAG "${DAILYBOY_CXXOPTS_GIT_TAG}"
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(dailyboy_cxxopts)

message(STATUS "deps: cxxopts ${DAILYBOY_CXXOPTS_GIT_TAG}")
