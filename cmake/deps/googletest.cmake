# GoogleTest — FetchContent when BUILD_TESTING=ON
# https://github.com/google/googletest

if(TARGET GTest::gtest)
    return()
endif()

set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    dailyboy_googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG "${DAILYBOY_GTEST_GIT_TAG}"
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(dailyboy_googletest)

message(STATUS "deps: GoogleTest ${DAILYBOY_GTEST_GIT_TAG}")
