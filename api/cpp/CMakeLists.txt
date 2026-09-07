add_library(dailyboy_api SHARED
    src/makeDaily.cpp
)

dailyboy_target_cxx_std(dailyboy_api PUBLIC)
target_compile_definitions(dailyboy_api PRIVATE DAILYBOY_BUILD_SHARED)

target_include_directories(dailyboy_api
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/dailyboy/include
)

target_link_libraries(dailyboy_api PUBLIC dailyboy)
dailyboy_enable_sanitizers(dailyboy_api)

set_target_properties(dailyboy_api PROPERTIES
    OUTPUT_NAME dailyboy_api
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

add_executable(makeDaily src/cli_makeDaily.cpp)
dailyboy_target_cxx_std(makeDaily PRIVATE)
target_link_libraries(makeDaily PRIVATE dailyboy_api cxxopts::cxxopts)
dailyboy_enable_sanitizers(makeDaily)
set_target_properties(makeDaily PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
