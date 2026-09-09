# oneTBB / Intel TBB — tag from DAILYBOY_ONETBB_* (CY2026: 2022.x; CY2025: 2021.x;
# CY2024: classic TBB 2020 Update 3 via make — Unix gcc/clang, Windows MSVC cl).
# https://github.com/uxlfoundation/oneTBB

if(TARGET TBB::tbb)
    return()
endif()

dailyboy_bundled_install_prefix(tbb DAILYBOY_TBB_PREFIX)
set(DAILYBOY_TBB_PREFIX "${DAILYBOY_TBB_PREFIX}" CACHE INTERNAL "bundled oneTBB prefix")
file(MAKE_DIRECTORY "${DAILYBOY_TBB_PREFIX}/include")
file(MAKE_DIRECTORY "${DAILYBOY_TBB_PREFIX}/lib")
if(WIN32)
    file(MAKE_DIRECTORY "${DAILYBOY_TBB_PREFIX}/bin")
endif()

dailyboy_bundled_build_type(_dailyboy_tbb_build_type)
if(_dailyboy_tbb_build_type STREQUAL "Debug")
    set(_dailyboy_tbb_lib_basename tbb_debug)
    set(_dailyboy_tbb_cfg debug)
else()
    set(_dailyboy_tbb_lib_basename tbb)
    set(_dailyboy_tbb_cfg release)
endif()

# Classic TBB 2020: Makefile (Unix + Windows/MSVC). oneTBB 2021+: CMake.
set(_dailyboy_tbb_use_classic FALSE)
if(DAILYBOY_ONETBB_VERSION STREQUAL "2020")
    set(_dailyboy_tbb_use_classic TRUE)
endif()

# Windows oneTBB (CMake) OUTPUT_NAME is tbb12; classic 2020 keeps tbb[_debug].
# MSVC oneTBB also installs legacy tbb[_debug].lib copies as IMPORTED_IMPLIB.
set(_dailyboy_tbb_dll_basename "${_dailyboy_tbb_lib_basename}")
set(_dailyboy_tbb_implib "")
if(WIN32 AND NOT _dailyboy_tbb_use_classic)
    set(_dailyboy_tbb_binary_version "12")
    if(_dailyboy_tbb_build_type STREQUAL "Debug")
        set(_dailyboy_tbb_dll_basename "tbb${_dailyboy_tbb_binary_version}_debug")
    else()
        set(_dailyboy_tbb_dll_basename "tbb${_dailyboy_tbb_binary_version}")
    endif()
    dailyboy_bundled_implib_path(
        "${DAILYBOY_TBB_PREFIX}/lib" "${_dailyboy_tbb_lib_basename}" _dailyboy_tbb_implib
    )
endif()
dailyboy_bundled_shared_lib_path(
    "${DAILYBOY_TBB_PREFIX}/lib" "${_dailyboy_tbb_dll_basename}" _dailyboy_tbb_lib
)
if(_dailyboy_tbb_implib)
    dailyboy_ep_imported_byproducts(
        _dailyboy_tbb_byproducts "${_dailyboy_tbb_lib}" IMPLIB "${_dailyboy_tbb_implib}"
    )
else()
    dailyboy_ep_imported_byproducts(_dailyboy_tbb_byproducts "${_dailyboy_tbb_lib}")
endif()

# Classic TBB 2020 arch token for Darwin make (auto-detect is broken on Apple Silicon).
function(dailyboy_tbb2020_apple_arch out_arch)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
        set(${out_arch} arm64 PARENT_SCOPE)
    else()
        set(${out_arch} intel64 PARENT_SCOPE)
    endif()
endfunction()

# Classic TBB 2020: make extras and built shared-lib filename for this host.
function(dailyboy_tbb2020_host_vars lib_basename out_make_extras out_built_lib)
    if(WIN32)
        # windows.inc + windows.cl.inc: SHELL=cmd, cl.exe (needs VS env / msvc-dev-cmd).
        set(${out_make_extras} compiler=cl arch=intel64 PARENT_SCOPE)
        set(${out_built_lib} "${lib_basename}.dll" PARENT_SCOPE)
    elseif(APPLE)
        dailyboy_tbb2020_apple_arch(_arch)
        set(_extras compiler=clang "arch=${_arch}")
        if(CMAKE_OSX_DEPLOYMENT_TARGET)
            list(APPEND _extras "MACOSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}")
        endif()
        set(${out_make_extras} "${_extras}" PARENT_SCOPE)
        set(${out_built_lib} "lib${lib_basename}.dylib" PARENT_SCOPE)
    else()
        set(${out_make_extras} "compiler=gcc" PARENT_SCOPE)
        set(${out_built_lib} "lib${lib_basename}.so.2" PARENT_SCOPE)
    endif()
endfunction()

if(_dailyboy_tbb_use_classic)
    if(NOT DAILYBOY_MAKE_EXECUTABLE)
        message(
            FATAL_ERROR
            "Classic TBB 2020 requires GNU make "
            "(Windows: mingw32-make from MSYS2 on PATH)."
        )
    endif()
    set(_dailyboy_tbb_make_prefix dailyboy)
    dailyboy_tbb2020_host_vars(
        "${_dailyboy_tbb_lib_basename}"
        _dailyboy_tbb_make_extras
        _dailyboy_tbb_built_lib
    )
    set(_dailyboy_tbb_build_subdir
        "${_dailyboy_tbb_make_prefix}_${_dailyboy_tbb_cfg}"
    )
    # Classic TBB: Makefile under src/; libs land in tbb_build_dir/<prefix>_<cfg>/.
    set(_dailyboy_tbb_install_cmds
        COMMAND ${CMAKE_COMMAND} -E rm -rf
            "${DAILYBOY_TBB_PREFIX}/include/tbb"
            "${DAILYBOY_TBB_PREFIX}/include/serial"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${DAILYBOY_TBB_PREFIX}/include"
            "${DAILYBOY_TBB_PREFIX}/lib"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            <SOURCE_DIR>/include/tbb "${DAILYBOY_TBB_PREFIX}/include/tbb"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            <SOURCE_DIR>/include/serial "${DAILYBOY_TBB_PREFIX}/include/serial"
    )
    if(WIN32)
        list(
            APPEND _dailyboy_tbb_install_cmds
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DAILYBOY_TBB_PREFIX}/bin"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                <BINARY_DIR>/${_dailyboy_tbb_build_subdir}/${_dailyboy_tbb_built_lib}
                "${DAILYBOY_TBB_PREFIX}/bin/${_dailyboy_tbb_built_lib}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                <BINARY_DIR>/${_dailyboy_tbb_build_subdir}/${_dailyboy_tbb_lib_basename}.lib
                "${DAILYBOY_TBB_PREFIX}/lib/${_dailyboy_tbb_lib_basename}.lib"
        )
    else()
        list(
            APPEND _dailyboy_tbb_install_cmds
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                <BINARY_DIR>/${_dailyboy_tbb_build_subdir}/${_dailyboy_tbb_built_lib}
                "${DAILYBOY_TBB_PREFIX}/lib/${_dailyboy_tbb_built_lib}"
        )
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        list(
            APPEND _dailyboy_tbb_install_cmds
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                ${_dailyboy_tbb_built_lib}
                "${DAILYBOY_TBB_PREFIX}/lib/lib${_dailyboy_tbb_lib_basename}.so"
        )
    endif()

    ExternalProject_Add(
        dailyboy_onetbb
        GIT_REPOSITORY https://github.com/uxlfoundation/oneTBB.git
        GIT_TAG "${DAILYBOY_ONETBB_GIT_TAG}"
        GIT_SHALLOW TRUE
        UPDATE_DISCONNECTED TRUE
        CONFIGURE_COMMAND ""
        BUILD_COMMAND
            ${DAILYBOY_MAKE_EXECUTABLE}
            -C <SOURCE_DIR>/src
            tbb_${_dailyboy_tbb_cfg}
            tbbmalloc_${_dailyboy_tbb_cfg}
            tbb_root=<SOURCE_DIR>
            tbb_build_dir=<BINARY_DIR>
            tbb_build_prefix=${_dailyboy_tbb_make_prefix}
            ${_dailyboy_tbb_make_extras}
            -j${DAILYBOY_EP_JOBS}
        INSTALL_COMMAND ${_dailyboy_tbb_install_cmds}
        BUILD_BYPRODUCTS ${_dailyboy_tbb_byproducts}
        USES_TERMINAL_BUILD TRUE
    )
else()
    dailyboy_ep_cmake_args(_dailyboy_tbb_args "${DAILYBOY_TBB_PREFIX}")
    list(APPEND _dailyboy_tbb_args
        -DTBB_TEST=OFF
        -DTBB_EXAMPLES=OFF
        -DTBB_STRICT=OFF
        -DTBB4PY_BUILD=OFF
    )

    ExternalProject_Add(
        dailyboy_onetbb
        GIT_REPOSITORY https://github.com/uxlfoundation/oneTBB.git
        GIT_TAG "${DAILYBOY_ONETBB_GIT_TAG}"
        GIT_SHALLOW TRUE
        UPDATE_DISCONNECTED TRUE
        CMAKE_ARGS ${_dailyboy_tbb_args}
        BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --parallel ${DAILYBOY_EP_JOBS}
        INSTALL_COMMAND ${CMAKE_COMMAND} --install <BINARY_DIR>
        BUILD_BYPRODUCTS ${_dailyboy_tbb_byproducts}
        USES_TERMINAL_BUILD TRUE
    )
endif()

if(_dailyboy_tbb_implib)
    dailyboy_add_imported_shared(
        TBB::tbb
        dailyboy_onetbb
        "${_dailyboy_tbb_lib}"
        "${DAILYBOY_TBB_PREFIX}/include"
        IMPLIB "${_dailyboy_tbb_implib}"
    )
else()
    dailyboy_add_imported_shared(
        TBB::tbb dailyboy_onetbb "${_dailyboy_tbb_lib}" "${DAILYBOY_TBB_PREFIX}/include"
    )
endif()
set_target_properties(TBB::tbb PROPERTIES INTERFACE_COMPILE_FEATURES cxx_std_17)

dailyboy_install_bundled_libs("${DAILYBOY_TBB_PREFIX}" "${CMAKE_SHARED_LIBRARY_PREFIX}tbb*")

message(STATUS "deps: oneTBB ${DAILYBOY_ONETBB_GIT_TAG}")
unset(_dailyboy_tbb_build_type)
unset(_dailyboy_tbb_lib_basename)
unset(_dailyboy_tbb_dll_basename)
unset(_dailyboy_tbb_implib)
unset(_dailyboy_tbb_lib)
unset(_dailyboy_tbb_args)
unset(_dailyboy_tbb_cfg)
unset(_dailyboy_tbb_make_prefix)
unset(_dailyboy_tbb_make_extras)
unset(_dailyboy_tbb_built_lib)
unset(_dailyboy_tbb_build_subdir)
unset(_dailyboy_tbb_install_cmds)
unset(_dailyboy_tbb_byproducts)
unset(_dailyboy_tbb_use_classic)
unset(_dailyboy_tbb_binary_version)
