# yaml-cpp — YAML parse/load for job files
# https://github.com/jbeder/yaml-cpp

if(TARGET yaml-cpp::yaml-cpp)
    return()
endif()

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    dailyboy_yaml_cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG "${DAILYBOY_YAML_CPP_GIT_TAG}"
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(dailyboy_yaml_cpp)

message(STATUS "deps: yaml-cpp ${DAILYBOY_YAML_CPP_GIT_TAG}")
