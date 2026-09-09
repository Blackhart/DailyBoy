# DailyBoy global CMake config.
# Included from the root CMakeLists.txt before add_subdirectory(dailyboy/api).

include(GNUInstallDirs)
include(FetchContent)
include(ExternalProject)
include(ProcessorCount)
include(CMakeDependentOption)

# CMake 4+ rejects cmake_minimum_required < 3.5; FetchContent deps (yaml-cpp, …)
# still declare older floors. ExternalProject gets the same via dailyboy_ep_cmake_args.
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0")
    set(CMAKE_POLICY_VERSION_MINIMUM "3.5" CACHE STRING
        "Floor for cmake_minimum_required in bundled deps (CMake 4+)")
endif()

# ---------------------------------------------------------------------------
# Module path (Find*.cmake for bundled prefixes only)
# ---------------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/modules")

# ---------------------------------------------------------------------------
# Versions
# ---------------------------------------------------------------------------
include("${CMAKE_CURRENT_LIST_DIR}/Versions.cmake")

# ---------------------------------------------------------------------------
# Install prefix (overridable: cmake --install build --prefix …)
# ---------------------------------------------------------------------------
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(
        CMAKE_INSTALL_PREFIX
        "${CMAKE_BINARY_DIR}/install"
        CACHE PATH "Install directory (cmake --install)"
        FORCE
    )
endif()

# ---------------------------------------------------------------------------
# Platform — VFX Reference Platform (DAILYBOY_VFX_PLATFORM)
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD "${DAILYBOY_CXX_STANDARD}")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

if(APPLE)
    set(
        CMAKE_OSX_DEPLOYMENT_TARGET
        "${DAILYBOY_OSX_DEPLOYMENT_TARGET}"
        CACHE STRING
        "macOS minimum (VFX ${DAILYBOY_VFX_PLATFORM_LABEL})"
    )
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "${DAILYBOY_LINUX_GCC_VERSION}")
        message(
            WARNING
            "DailyBoy ${DAILYBOY_VFX_PLATFORM_LABEL}: gcc ${DAILYBOY_LINUX_GCC_VERSION} recommended "
            "(current compiler: ${CMAKE_CXX_COMPILER_VERSION})"
        )
    endif()
endif()

if(WIN32 AND MSVC)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "${DAILYBOY_WINDOWS_MSVC_VERSION}")
        message(
            WARNING
            "DailyBoy ${DAILYBOY_VFX_PLATFORM_LABEL}: Visual Studio 2022 "
            "v${DAILYBOY_WINDOWS_VS_VERSION}+ (MSVC ${DAILYBOY_WINDOWS_MSVC_VERSION}+) "
            "recommended (current compiler: ${CMAKE_CXX_COMPILER_VERSION})"
        )
    endif()
    set(_dailyboy_windows_sdk "")
    if(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
        set(_dailyboy_windows_sdk "${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")
    elseif(DEFINED ENV{WindowsSDKVersion})
        string(REGEX REPLACE "[\\/]+$" "" _dailyboy_windows_sdk "$ENV{WindowsSDKVersion}")
    endif()
    if(_dailyboy_windows_sdk)
        if(_dailyboy_windows_sdk VERSION_LESS "${DAILYBOY_WINDOWS_SDK_VERSION}")
            message(
                WARNING
                "DailyBoy ${DAILYBOY_VFX_PLATFORM_LABEL}: Windows SDK "
                "${DAILYBOY_WINDOWS_SDK_VERSION}+ recommended "
                "(current: ${_dailyboy_windows_sdk})"
            )
        endif()
    else()
        message(
            STATUS
            "DailyBoy ${DAILYBOY_VFX_PLATFORM_LABEL}: Windows SDK version not detected; "
            "VFX requires ${DAILYBOY_WINDOWS_SDK_VERSION}+"
        )
    endif()
    unset(_dailyboy_windows_sdk)
endif()

message(
    STATUS
    "DailyBoy targets VFX Reference Platform ${DAILYBOY_VFX_PLATFORM_LABEL} "
    "(C++${CMAKE_CXX_STANDARD}, Python ${DAILYBOY_PYTHON_VERSION}.x)"
)

# ---------------------------------------------------------------------------
# macOS
# ---------------------------------------------------------------------------
if(APPLE)
    set(CMAKE_MACOSX_RPATH ON)
    set(CMAKE_INSTALL_RPATH "@loader_path/../lib")
endif()

# ---------------------------------------------------------------------------
# Linux
# ---------------------------------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
endif()

# ---------------------------------------------------------------------------
# Windows
# ---------------------------------------------------------------------------
if(WIN32)
    # Shared deps install DLLs under bin/; import libs under lib/.
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    if(MSVC AND NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    endif()
endif()

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(BUILD_TESTING "Build tests (CTest, GoogleTest) and test-only dependencies" ON)
option(DAILYBOY_BUILD_PYTHON "Build Python bindings (api/python)" ON)
option(
    DAILYBOY_PYTHON_BUILD_IN_SOURCE
    "Place _dailyboy_native under api/python/dailyboy/ (OFF = CMake build tree)"
    ON
)
option(
    DAILYBOY_EXPORT_COMPILE_COMMANDS
    "Generate compile_commands.json in the build directory"
    ON
)
option(
    DAILYBOY_COMPILE_COMMANDS_SYMLINK
    "Symlink compile_commands.json to the repository root"
    ON
)
cmake_dependent_option(
    DAILYBOY_ENABLE_COVERAGE
    "Instrument DailyBoy targets (not bundled deps) with GCC/Clang gcov"
    OFF
    "BUILD_TESTING"
    OFF
)
if(BUILD_TESTING AND DAILYBOY_ENABLE_COVERAGE)
    set(
        DAILYBOY_GCOVR_HTML_THEME
        "green"
        CACHE STRING
        "gcovr HTML coverage theme (green, blue, github.green, github.blue, github.dark-green, github.dark-blue)"
    )
else()
    unset(DAILYBOY_GCOVR_HTML_THEME CACHE)
endif()

# ---------------------------------------------------------------------------
# Debug / compile_commands
# ---------------------------------------------------------------------------
if(DAILYBOY_EXPORT_COMPILE_COMMANDS)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    message(STATUS "compile_commands.json → ${CMAKE_BINARY_DIR}/compile_commands.json")
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Zi")
else()
    string(APPEND CMAKE_CXX_FLAGS_DEBUG " -fno-omit-frame-pointer")
    string(APPEND CMAKE_CXX_FLAGS_RELWITHDEBINFO " -fno-omit-frame-pointer")
endif()

if(CMAKE_BUILD_TYPE MATCHES "^(Debug|RelWithDebInfo)$")
    set(CMAKE_INSTALL_DO_STRIP OFF)
endif()

# ---------------------------------------------------------------------------
# Helpers — ExternalProject layout, C++ std, and build-tree runtime paths
# ---------------------------------------------------------------------------
ProcessorCount(DAILYBOY_EP_JOBS)
if(DAILYBOY_EP_JOBS LESS_EQUAL 0)
    set(DAILYBOY_EP_JOBS 1)
endif()

find_program(DAILYBOY_MAKE_EXECUTABLE NAMES gmake make)
find_program(DAILYBOY_NASM_EXECUTABLE NAMES nasm)
if(WIN32)
    find_program(
        DAILYBOY_MSYS2_BASH
        NAMES bash.exe bash
        PATHS
            ENV MSYS2_BASH
            "C:/msys64/usr/bin"
            "C:/tools/msys64/usr/bin"
            "$ENV{MSYS2_PATH}/usr/bin"
        DOC "MSYS2 bash for x264/FFmpeg configure on Windows"
    )
endif()

function(dailyboy_bundled_build_type out_var)
    if(CMAKE_BUILD_TYPE)
        set(${out_var} "${CMAKE_BUILD_TYPE}" PARENT_SCOPE)
    else()
        set(${out_var} "Release" PARENT_SCOPE)
    endif()
endfunction()

function(dailyboy_bundled_layout_slug out_var)
    dailyboy_bundled_build_type(_build_type)
    string(TOLOWER "${_build_type}" _slug)
    set(${out_var} "${_slug}" PARENT_SCOPE)
endfunction()

function(dailyboy_bundled_install_prefix component out_var)
    dailyboy_bundled_layout_slug(_slug)
    set(${out_var} "${CMAKE_BINARY_DIR}/_deps/${component}/${_slug}" PARENT_SCOPE)
endfunction()

# Shared library location in a bundled prefix (DLL under bin/ on Windows).
function(dailyboy_bundled_shared_lib_path libdir basename out_var)
    if(WIN32)
        get_filename_component(_prefix "${libdir}" DIRECTORY)
        set(
            ${out_var}
            "${_prefix}/bin/${basename}${CMAKE_SHARED_LIBRARY_SUFFIX}"
            PARENT_SCOPE
        )
    else()
        set(
            ${out_var}
            "${libdir}/${CMAKE_SHARED_LIBRARY_PREFIX}${basename}${CMAKE_SHARED_LIBRARY_SUFFIX}"
            PARENT_SCOPE
        )
    endif()
endfunction()

# MSVC import library next to a bundled shared install.
function(dailyboy_bundled_implib_path libdir basename out_var)
    set(${out_var} "${libdir}/${basename}.lib" PARENT_SCOPE)
endfunction()

# Debug postfix for WIN32 shared basenames when CMAKE_BUILD_TYPE is Debug.
function(dailyboy_win_shared_basename basename out_var)
    if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(${out_var} "${basename}d" PARENT_SCOPE)
    else()
        set(${out_var} "${basename}" PARENT_SCOPE)
    endif()
endfunction()

function(dailyboy_bundled_static_lib_path libdir basename out_var)
    if(WIN32)
        set(${out_var} "${libdir}/${basename}.lib" PARENT_SCOPE)
    else()
        set(${out_var} "${libdir}/lib${basename}.a" PARENT_SCOPE)
    endif()
endfunction()

# Standard CMAKE_ARGS for a CMake ExternalProject (shared, PIC, same compilers).
function(dailyboy_ep_cmake_args out_var install_prefix)
    dailyboy_bundled_build_type(_bt)
    set(_args
        -DCMAKE_INSTALL_PREFIX=${install_prefix}
        -DCMAKE_BUILD_TYPE=${_bt}
        -DCMAKE_INSTALL_LIBDIR=lib
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DBUILD_SHARED_LIBS=ON
    )
    # CMake 4+ rejects cmake_minimum_required < 3.5; many bundled deps still use it.
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0")
        list(APPEND _args -DCMAKE_POLICY_VERSION_MINIMUM=3.5)
    endif()
    if(MSVC)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            list(APPEND _args -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebugDLL)
        else()
            list(APPEND _args -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL)
        endif()
    endif()
    if(CMAKE_CXX_COMPILER)
        list(APPEND _args "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")
    endif()
    if(CMAKE_C_COMPILER)
        list(APPEND _args "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}")
    endif()
    if(CMAKE_TOOLCHAIN_FILE)
        list(APPEND _args "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
    endif()
    if(APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET)
        list(APPEND _args "-DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}")
    endif()
    set(${out_var} "${_args}" PARENT_SCOPE)
endfunction()

function(dailyboy_add_imported_shared name ep_target location include_dir)
    add_library(${name} SHARED IMPORTED GLOBAL)
    add_dependencies(${name} ${ep_target})
    set_target_properties(
        ${name}
        PROPERTIES
            IMPORTED_LOCATION "${location}"
            INTERFACE_INCLUDE_DIRECTORIES "${include_dir}"
    )
    if(WIN32)
        get_filename_component(_dll_we "${location}" NAME_WE)
        get_filename_component(_dll_dir "${location}" DIRECTORY)
        get_filename_component(_prefix "${_dll_dir}" DIRECTORY)
        set_target_properties(
            ${name}
            PROPERTIES IMPORTED_IMPLIB "${_prefix}/lib/${_dll_we}.lib"
        )
    endif()
endfunction()

function(dailyboy_install_bundled_libs prefix glob)
    install(
        DIRECTORY "${prefix}/lib/"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        FILES_MATCHING PATTERN "${glob}"
    )
    if(WIN32 AND IS_DIRECTORY "${prefix}/bin")
        install(
            DIRECTORY "${prefix}/bin/"
            DESTINATION ${CMAKE_INSTALL_BINDIR}
            FILES_MATCHING PATTERN "${glob}"
        )
    endif()
endfunction()

function(dailyboy_join_pkg_config_path out_var)
    set(_pcs "")
    foreach(_p IN LISTS ARGN)
        if(IS_DIRECTORY "${_p}/lib/pkgconfig")
            list(APPEND _pcs "${_p}/lib/pkgconfig")
        elseif(NOT "${_p}" STREQUAL "")
            # Prefix may not exist yet at configure time; still pass pkgconfig dir.
            list(APPEND _pcs "${_p}/lib/pkgconfig")
        endif()
    endforeach()
    if(WIN32)
        string(JOIN ";" _joined ${_pcs})
    else()
        string(JOIN ":" _joined ${_pcs})
    endif()
    set(${out_var} "${_joined}" PARENT_SCOPE)
endfunction()

# Absolute bundled runtime dirs in the build tree (FFmpeg, x264, OIIO, engine, API).
function(dailyboy_bundled_runtime_lib_dirs out_var)
    file(GLOB _dirs "${CMAKE_BINARY_DIR}/_deps/*/*/lib")
    if(WIN32)
        file(GLOB _bins "${CMAKE_BINARY_DIR}/_deps/*/*/bin")
        list(APPEND _dirs ${_bins})
        list(
            APPEND _dirs
            "${CMAKE_BINARY_DIR}/bin"
            "${CMAKE_BINARY_DIR}/dailyboy"
            "${CMAKE_BINARY_DIR}/api/cpp"
        )
    else()
        list(APPEND _dirs "${CMAKE_BINARY_DIR}/dailyboy" "${CMAKE_BINARY_DIR}/api/cpp")
    endif()
    set(${out_var} "${_dirs}" PARENT_SCOPE)
endfunction()

# Apply the selected C++ standard to a target (PUBLIC / PRIVATE / INTERFACE).
function(dailyboy_target_cxx_std target visibility)
    target_compile_features(
        ${target}
        ${visibility}
        cxx_std_${CMAKE_CXX_STANDARD}
    )
endfunction()

# Joined runtime library search path for build-tree binaries and CTest.
# libavcodec NEEDs libx264; GNU ld.so ignores the exe DT_RUNPATH for that.
# Apple: DYLD_LIBRARY_PATH; Windows: PATH; else LD_LIBRARY_PATH.
function(dailyboy_bundled_ld_library_path out_var)
    dailyboy_bundled_runtime_lib_dirs(_dirs)
    if(WIN32)
        string(JOIN ";" _path ${_dirs})
        if(DEFINED ENV{PATH} AND NOT "$ENV{PATH}" STREQUAL "")
            string(APPEND _path ";$ENV{PATH}")
        endif()
    else()
        string(JOIN ":" _path ${_dirs})
        if(APPLE)
            if(DEFINED ENV{DYLD_LIBRARY_PATH} AND NOT "$ENV{DYLD_LIBRARY_PATH}" STREQUAL "")
                string(APPEND _path ":$ENV{DYLD_LIBRARY_PATH}")
            endif()
        elseif(DEFINED ENV{LD_LIBRARY_PATH} AND NOT "$ENV{LD_LIBRARY_PATH}" STREQUAL "")
            string(APPEND _path ":$ENV{LD_LIBRARY_PATH}")
        endif()
    endif()
    set(${out_var} "${_path}" PARENT_SCOPE)
endfunction()

# ENVIRONMENT property value for CTest (PATH=… / DYLD_… / LD_…).
function(dailyboy_bundled_runtime_path_env out_var)
    dailyboy_bundled_ld_library_path(_path)
    if(WIN32)
        set(${out_var} "PATH=${_path}" PARENT_SCOPE)
    elseif(APPLE)
        set(${out_var} "DYLD_LIBRARY_PATH=${_path}" PARENT_SCOPE)
    else()
        set(${out_var} "LD_LIBRARY_PATH=${_path}" PARENT_SCOPE)
    endif()
endfunction()

# ---------------------------------------------------------------------------
# Dependencies — FetchContent then ExternalProject (imaging / video)
# ---------------------------------------------------------------------------
include("${CMAKE_CURRENT_LIST_DIR}/deps/cxxopts.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/deps/spdlog.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/deps/yaml-cpp.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/deps/nlohmann_json.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/deps/json_schema_validator.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/deps/fileseq.cmake")

if(DAILYBOY_BUILD_PYTHON)
    include("${CMAKE_CURRENT_LIST_DIR}/deps/pybind11.cmake")
endif()

if(BUILD_TESTING)
    include("${CMAKE_CURRENT_LIST_DIR}/deps/googletest.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/deps/googlebenchmark.cmake")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/deps/zlib.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/deps/onetbb.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/deps/opencolorio.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/deps/jpeg_turbo.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/deps/libpng.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/deps/libtiff.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/deps/libraw.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/deps/libde265.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/deps/x264.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/deps/x265.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/deps/aom.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/deps/libheif.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/deps/ffmpeg.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/deps/openimageio.cmake")

# ---------------------------------------------------------------------------
# clang-format
# ---------------------------------------------------------------------------
find_program(CLANG_FORMAT NAMES clang-format clang-format-18 clang-format-17)

set(_dailyboy_clang_format_dirs
    "${CMAKE_SOURCE_DIR}/dailyboy"
    "${CMAKE_SOURCE_DIR}/api/cpp"
    "${CMAKE_SOURCE_DIR}/api/python"
)
set(DAILYBOY_CLANG_FORMAT_SOURCES "")
foreach(_dir IN LISTS _dailyboy_clang_format_dirs)
    if(NOT IS_DIRECTORY "${_dir}")
        continue()
    endif()
    file(
        GLOB_RECURSE _sources
        CONFIGURE_DEPENDS
        "${_dir}/*.cpp"
        "${_dir}/*.hpp"
        "${_dir}/*.h"
        "${_dir}/*.cc"
    )
    list(APPEND DAILYBOY_CLANG_FORMAT_SOURCES ${_sources})
endforeach()
list(REMOVE_DUPLICATES DAILYBOY_CLANG_FORMAT_SOURCES)
list(SORT DAILYBOY_CLANG_FORMAT_SOURCES)

if(CLANG_FORMAT AND DAILYBOY_CLANG_FORMAT_SOURCES)
    add_custom_target(
        clang-format
        COMMAND ${CLANG_FORMAT} -i --style=file ${DAILYBOY_CLANG_FORMAT_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Formatting C++ sources with clang-format (Google)"
        VERBATIM
    )
    add_custom_target(
        clang-format-check
        COMMAND
            ${CLANG_FORMAT} --dry-run --Werror --style=file
            ${DAILYBOY_CLANG_FORMAT_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Checking C++ formatting (clang-format, Google)"
        VERBATIM
    )
    message(STATUS "clang-format: targets clang-format, clang-format-check")
elseif(NOT CLANG_FORMAT)
    message(STATUS "clang-format not found — install it for format targets")
endif()
unset(_dailyboy_clang_format_dirs)
unset(_dir)
unset(_sources)

# ---------------------------------------------------------------------------
# Static analysis (clang-tidy)
# ---------------------------------------------------------------------------
# Production code only (skip unit / perf / load tests under /tests/).
set(DAILYBOY_CLANG_TIDY_SOURCES "")
foreach(_src IN LISTS DAILYBOY_CLANG_FORMAT_SOURCES)
    if(_src MATCHES "/tests/")
        continue()
    endif()
    list(APPEND DAILYBOY_CLANG_TIDY_SOURCES "${_src}")
endforeach()
unset(_src)

find_program(CLANG_TIDY NAMES clang-tidy clang-tidy-18 clang-tidy-17)
if(CLANG_TIDY AND DAILYBOY_CLANG_TIDY_SOURCES)
    add_custom_target(
        clang-tidy-check
        COMMAND
            ${CLANG_TIDY} -p ${CMAKE_BINARY_DIR}
            --config-file=${CMAKE_SOURCE_DIR}/.clang-tidy
            ${DAILYBOY_CLANG_TIDY_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-tidy on DailyBoy sources"
        VERBATIM
    )
    message(STATUS "clang-tidy: target clang-tidy-check (${CLANG_TIDY})")
elseif(NOT CLANG_TIDY)
    message(STATUS "clang-tidy not found — install it for tidy-check")
endif()

# ---------------------------------------------------------------------------
# Sanitizers (ASan + UBSan)
# ---------------------------------------------------------------------------
option(
    DAILYBOY_ENABLE_SANITIZERS
    "Instrument DailyBoy targets (not bundled deps) with ASan+UBSan"
    OFF
)
# On DailyBoy targets only — never pass into ExternalProject.
function(dailyboy_enable_sanitizers target)
    if(NOT DAILYBOY_ENABLE_SANITIZERS)
        return()
    endif()
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "DAILYBOY_ENABLE_SANITIZERS requires GCC or Clang")
    endif()
    target_compile_options(
        ${target}
        PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer
    )
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
endfunction()
if(DAILYBOY_ENABLE_SANITIZERS)
    message(STATUS "ASan+UBSan ON (DailyBoy targets only)")
endif()

# ---------------------------------------------------------------------------
# Testing / Coverage (gtest discovery after add_subdirectory via DEFER)
# ---------------------------------------------------------------------------
if(BUILD_TESTING)
    enable_testing()
    include(GoogleTest)

    # gcov on DailyBoy only — never pass --coverage into ExternalProject / FetchContent.
    function(dailyboy_enable_coverage target)
        if(NOT DAILYBOY_ENABLE_COVERAGE)
            return()
        endif()
        if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            message(FATAL_ERROR "DAILYBOY_ENABLE_COVERAGE requires GCC or Clang")
        endif()
        target_compile_options(${target} PRIVATE --coverage)
        target_link_options(${target} PRIVATE --coverage)
    endfunction()

    if(DAILYBOY_ENABLE_COVERAGE)
        message(STATUS "gcov coverage ON (DailyBoy targets only)")
        set(_dailyboy_gcovr_html_themes
            green
            blue
            github.green
            github.blue
            github.dark-green
            github.dark-blue
        )
        set_property(
            CACHE DAILYBOY_GCOVR_HTML_THEME
            PROPERTY STRINGS ${_dailyboy_gcovr_html_themes}
        )
        if(NOT DAILYBOY_GCOVR_HTML_THEME IN_LIST _dailyboy_gcovr_html_themes)
            message(
                FATAL_ERROR
                "DAILYBOY_GCOVR_HTML_THEME must be one of: ${_dailyboy_gcovr_html_themes}"
            )
        endif()
        unset(_dailyboy_gcovr_html_themes)
        unset(DAILYBOY_GCOVR CACHE)
        find_program(
            DAILYBOY_GCOVR
            NAMES gcovr
            HINTS "${CMAKE_BINARY_DIR}/.venv-gcovr/bin"
        )
        if(DAILYBOY_GCOVR)
            message(STATUS "gcovr: ${DAILYBOY_GCOVR}")
            message(STATUS "gcovr HTML theme: ${DAILYBOY_GCOVR_HTML_THEME}")
        else()
            message(
                WARNING
                "gcovr not found — coverage-report target is skipped. Install with:\n"
                "  python3 -m venv \"${CMAKE_BINARY_DIR}/.venv-gcovr\" && "
                "\"${CMAKE_BINARY_DIR}/.venv-gcovr/bin/pip\" install gcovr"
            )
        endif()
    endif()

    # gtest PRE_TEST discovery runs the binary; CROSSCOMPILING_EMULATOR is
    # cmake -E env (not a cross compiler) so libx264 is visible.
    function(dailyboy_gtest_discover name)
        dailyboy_bundled_runtime_path_env(_env)
        set_property(
            TARGET ${name}
            PROPERTY CROSSCOMPILING_EMULATOR
                "${CMAKE_COMMAND}" -E env "${_env}"
        )
        gtest_discover_tests(
            ${name}
            PROPERTIES
                LABELS unit
                ENVIRONMENT "${_env}"
            DISCOVERY_MODE PRE_TEST
        )
    endfunction()
endif()

# ---------------------------------------------------------------------------
# Finalize — runs at end of root CMakeLists (after dailyboy/ and api/)
# ---------------------------------------------------------------------------
function(_dailyboy_symlink_compile_commands)
    set(_src "${CMAKE_BINARY_DIR}/compile_commands.json")
    set(_dst "${CMAKE_SOURCE_DIR}/compile_commands.json")
    if(NOT EXISTS "${_src}")
        return()
    endif()
    if(EXISTS "${_dst}" AND NOT IS_SYMLINK "${_dst}")
        message(
            WARNING
            "${_dst} exists and is not a symlink — remove it to enable "
            "DAILYBOY_COMPILE_COMMANDS_SYMLINK"
        )
        return()
    endif()
    file(CREATE_LINK "${_src}" "${_dst}" SYMBOLIC)
    message(STATUS "Symlink: compile_commands.json → build/compile_commands.json")
endfunction()

function(_dailyboy_finalize)
    install(
        TARGETS dailyboy dailyboy_api makeDaily
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    if(WIN32)
        install(
            FILES "${CMAKE_SOURCE_DIR}/cmake/dailyboy-env.windows.ps1.in"
            DESTINATION ${CMAKE_INSTALL_DATADIR}/dailyboy
            RENAME env.ps1
        )
    else()
        if(APPLE)
            set(_dailyboy_env_sh "${CMAKE_SOURCE_DIR}/cmake/dailyboy-env.macos.sh.in")
        else()
            set(_dailyboy_env_sh "${CMAKE_SOURCE_DIR}/cmake/dailyboy-env.linux.sh.in")
        endif()
        install(
            FILES "${_dailyboy_env_sh}"
            DESTINATION ${CMAKE_INSTALL_DATADIR}/dailyboy
            RENAME env.sh
        )
    endif()

    install(
        DIRECTORY "${CMAKE_SOURCE_DIR}/dailyboy/include/"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING
        PATTERN "*.hpp"
        PATTERN "*.h"
    )
    install(
        DIRECTORY "${CMAKE_SOURCE_DIR}/api/cpp/include/"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING
        PATTERN "*.hpp"
        PATTERN "*.h"
    )
    install(
        DIRECTORY "${CMAKE_SOURCE_DIR}/dailyboy/schemas/"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/dailyboy
        FILES_MATCHING PATTERN "*.json"
    )

    if(DAILYBOY_BUILD_PYTHON AND TARGET _dailyboy_native)
        set(_dailyboy_py_site "python${Python3_VERSION_MAJOR}/site-packages/dailyboy")
        install(
            TARGETS _dailyboy_native
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${_dailyboy_py_site}
            RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/${_dailyboy_py_site}
        )
        install(
            FILES
                "${CMAKE_SOURCE_DIR}/api/python/dailyboy/__init__.py"
                "${CMAKE_SOURCE_DIR}/api/python/dailyboy/cli.py"
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/${_dailyboy_py_site}
        )
    endif()

    if(BUILD_TESTING)
        add_test(NAME makeDaily_help COMMAND makeDaily --help)
        dailyboy_bundled_runtime_path_env(_dailyboy_help_env)
        set_tests_properties(
            makeDaily_help
            PROPERTIES ENVIRONMENT "${_dailyboy_help_env}"
        )
        if(TARGET clang-format-check)
            add_test(
                NAME clang_format_check
                COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clang-format-check
            )
        endif()
        if(DAILYBOY_ENABLE_COVERAGE AND DAILYBOY_GCOVR)
            add_custom_target(
                coverage-report
                COMMAND ${CMAKE_COMMAND} -E make_directory
                        ${CMAKE_BINARY_DIR}/coverage
                COMMAND
                    ${DAILYBOY_GCOVR}
                    --root ${CMAKE_SOURCE_DIR}
                    --filter dailyboy/src/
                    --exclude dailyboy/tests/
                    --exclude-directories=.*/tests/.*
                    --gcov-ignore-parse-errors
                    --gcov-ignore-errors=all
                    --html-theme ${DAILYBOY_GCOVR_HTML_THEME}
                    --html-details ${CMAKE_BINARY_DIR}/coverage/index.html
                    --lcov ${CMAKE_BINARY_DIR}/coverage/coverage.info
                    --cobertura ${CMAKE_BINARY_DIR}/coverage/coverage.xml
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                COMMENT "Writing coverage HTML + LCOV + Cobertura"
                VERBATIM
            )
        endif()
    endif()

    if(DAILYBOY_EXPORT_COMPILE_COMMANDS AND DAILYBOY_COMPILE_COMMANDS_SYMLINK)
        _dailyboy_symlink_compile_commands()
    endif()

    if(DAILYBOY_BUILD_PYTHON)
        message(
            STATUS
            "DailyBoy ${PROJECT_VERSION} — dailyboy, dailyboy_api, makeDaily, Python"
        )
    else()
        message(STATUS "DailyBoy ${PROJECT_VERSION} — dailyboy, dailyboy_api, makeDaily")
    endif()
    message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
endfunction()

cmake_language(DEFER CALL _dailyboy_finalize)
