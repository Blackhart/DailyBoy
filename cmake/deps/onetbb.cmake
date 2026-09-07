# oneTBB / Intel TBB — tag from DAILYBOY_ONETBB_* (CY2026: 2022.x; CY2025: 2021.x;
# CY2024: classic TBB 2020 Update 3 via make — no root CMakeLists.txt).
# https://github.com/uxlfoundation/oneTBB

if(TARGET TBB::tbb)
    return()
endif()

dailyboy_bundled_install_prefix(tbb DAILYBOY_TBB_PREFIX)
set(DAILYBOY_TBB_PREFIX "${DAILYBOY_TBB_PREFIX}" CACHE INTERNAL "bundled oneTBB prefix")
file(MAKE_DIRECTORY "${DAILYBOY_TBB_PREFIX}/include")
file(MAKE_DIRECTORY "${DAILYBOY_TBB_PREFIX}/lib")

dailyboy_bundled_build_type(_dailyboy_tbb_build_type)
if(_dailyboy_tbb_build_type STREQUAL "Debug")
    set(_dailyboy_tbb_lib_basename tbb_debug)
    set(_dailyboy_tbb_cfg debug)
else()
    set(_dailyboy_tbb_lib_basename tbb)
    set(_dailyboy_tbb_cfg release)
endif()
dailyboy_bundled_shared_lib_path(
    "${DAILYBOY_TBB_PREFIX}/lib" "${_dailyboy_tbb_lib_basename}" _dailyboy_tbb_lib
)

if(DAILYBOY_ONETBB_VERSION STREQUAL "2020")
    set(_dailyboy_tbb_make_prefix dailyboy)
    # Classic TBB: Makefile under src/; libs land in tbb_build_dir/<prefix>_<cfg>/.
    ExternalProject_Add(
        dailyboy_onetbb
        GIT_REPOSITORY https://github.com/uxlfoundation/oneTBB.git
        GIT_TAG "${DAILYBOY_ONETBB_GIT_TAG}"
        GIT_SHALLOW TRUE
        UPDATE_DISCONNECTED TRUE
        CONFIGURE_COMMAND ""
        # Classic TBB requires GNU make (Ninja is the top-level generator).
        BUILD_COMMAND
            make
            -C <SOURCE_DIR>/src
            tbb_${_dailyboy_tbb_cfg}
            tbbmalloc_${_dailyboy_tbb_cfg}
            tbb_root=<SOURCE_DIR>
            tbb_build_dir=<BINARY_DIR>
            tbb_build_prefix=${_dailyboy_tbb_make_prefix}
            compiler=gcc
            -j${DAILYBOY_EP_JOBS}
        INSTALL_COMMAND
            ${CMAKE_COMMAND} -E rm -rf
                "${DAILYBOY_TBB_PREFIX}/include/tbb"
                "${DAILYBOY_TBB_PREFIX}/include/serial"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${DAILYBOY_TBB_PREFIX}/include"
            "${DAILYBOY_TBB_PREFIX}/lib"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            <SOURCE_DIR>/include/tbb "${DAILYBOY_TBB_PREFIX}/include/tbb"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            <SOURCE_DIR>/include/serial "${DAILYBOY_TBB_PREFIX}/include/serial"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            <BINARY_DIR>/${_dailyboy_tbb_make_prefix}_${_dailyboy_tbb_cfg}/lib${_dailyboy_tbb_lib_basename}.so.2
            "${DAILYBOY_TBB_PREFIX}/lib/lib${_dailyboy_tbb_lib_basename}.so.2"
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            lib${_dailyboy_tbb_lib_basename}.so.2
            "${DAILYBOY_TBB_PREFIX}/lib/lib${_dailyboy_tbb_lib_basename}.so"
        BUILD_BYPRODUCTS "${_dailyboy_tbb_lib}"
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
        BUILD_BYPRODUCTS "${_dailyboy_tbb_lib}"
        USES_TERMINAL_BUILD TRUE
    )
endif()

dailyboy_add_imported_shared(
    TBB::tbb dailyboy_onetbb "${_dailyboy_tbb_lib}" "${DAILYBOY_TBB_PREFIX}/include"
)
set_target_properties(TBB::tbb PROPERTIES INTERFACE_COMPILE_FEATURES cxx_std_17)

dailyboy_install_bundled_libs("${DAILYBOY_TBB_PREFIX}" "${CMAKE_SHARED_LIBRARY_PREFIX}tbb*")

message(STATUS "deps: oneTBB ${DAILYBOY_ONETBB_GIT_TAG}")
unset(_dailyboy_tbb_build_type)
unset(_dailyboy_tbb_lib_basename)
unset(_dailyboy_tbb_lib)
unset(_dailyboy_tbb_args)
unset(_dailyboy_tbb_cfg)
unset(_dailyboy_tbb_make_prefix)
