# spdlog — logging (header-only)
# https://github.com/gabime/spdlog

if(TARGET spdlog::spdlog_header_only)
    return()
endif()

set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_PIC ON CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    dailyboy_spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG "${DAILYBOY_SPDLOG_GIT_TAG}"
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(dailyboy_spdlog)

message(STATUS "deps: spdlog ${DAILYBOY_SPDLOG_GIT_TAG} (header-only)")
