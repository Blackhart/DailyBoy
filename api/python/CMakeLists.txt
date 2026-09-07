# pybind11: loaded by cmake/deps/pybind11.cmake (via cmake/DailyBoy.cmake)
#
# The extension is always written in the CMake build tree so Debug/Release
# do not share a .so (in-source output broke cmake --install RPATH rewrite).

pybind11_add_module(_dailyboy_native bindings.cpp)

target_link_libraries(_dailyboy_native PRIVATE dailyboy Python3::Python)

target_include_directories(
    _dailyboy_native
    PRIVATE
        ${CMAKE_SOURCE_DIR}/dailyboy/include
)

set(_dailyboy_py_build_dir "${CMAKE_CURRENT_BINARY_DIR}/dailyboy")
file(MAKE_DIRECTORY "${_dailyboy_py_build_dir}")

set_target_properties(
    _dailyboy_native
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${_dailyboy_py_build_dir}"
        RUNTIME_OUTPUT_DIRECTORY "${_dailyboy_py_build_dir}"
)

add_custom_command(
    TARGET _dailyboy_native
    POST_BUILD
    COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/dailyboy/__init__.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/dailyboy/cli.py"
        "${_dailyboy_py_build_dir}/"
    COMMENT "Sync dailyboy Python package into build tree"
)

if(DAILYBOY_PYTHON_BUILD_IN_SOURCE)
    add_custom_command(
        TARGET _dailyboy_native
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:_dailyboy_native>
            "${CMAKE_CURRENT_SOURCE_DIR}/dailyboy/"
        COMMENT "Copy _dailyboy_native into source tree"
    )
endif()

message(STATUS "Python bindings: ${_dailyboy_py_build_dir}/_dailyboy_native*.so")
