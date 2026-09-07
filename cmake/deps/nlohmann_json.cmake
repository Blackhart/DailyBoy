# nlohmann/json — JSON DOM for schema validation
# https://github.com/nlohmann/json

if(TARGET nlohmann_json::nlohmann_json)
    return()
endif()

set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    dailyboy_nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG "${DAILYBOY_NLOHMANN_JSON_GIT_TAG}"
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(dailyboy_nlohmann_json)

message(STATUS "deps: nlohmann/json ${DAILYBOY_NLOHMANN_JSON_GIT_TAG}")
