# nlohmann/json-schema-validator — JSON Schema validation in C++
# https://github.com/pboettch/json-schema-validator
# STATIC so cmake --install does not ship an orphan .so.

if(TARGET nlohmann_json_schema_validator::validator OR TARGET nlohmann_json_schema_validator)
    return()
endif()

set(JSON_VALIDATOR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(JSON_VALIDATOR_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JSON_VALIDATOR_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(nlohmann_json_schema_validator_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(JSON_VALIDATOR_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    dailyboy_json_schema_validator
    GIT_REPOSITORY https://github.com/pboettch/json-schema-validator.git
    GIT_TAG "${DAILYBOY_JSON_SCHEMA_VALIDATOR_GIT_TAG}"
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(dailyboy_json_schema_validator)

if(TARGET nlohmann_json_schema_validator)
    set_target_properties(
        nlohmann_json_schema_validator
        PROPERTIES POSITION_INDEPENDENT_CODE ON
    )
    if(NOT TARGET nlohmann_json_schema_validator::validator)
        add_library(
            nlohmann_json_schema_validator::validator ALIAS nlohmann_json_schema_validator
        )
    endif()
endif()

message(STATUS "deps: json-schema-validator ${DAILYBOY_JSON_SCHEMA_VALIDATOR_GIT_TAG} (STATIC)")
