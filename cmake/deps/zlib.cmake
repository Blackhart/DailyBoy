# zlib — shared by FFmpeg, libpng, libtiff, libheif, OpenImageIO
# https://github.com/madler/zlib

if(TARGET ZLIB::ZLIB)
    return()
endif()

dailyboy_bundled_install_prefix(zlib DAILYBOY_ZLIB_PREFIX)
set(DAILYBOY_ZLIB_PREFIX "${DAILYBOY_ZLIB_PREFIX}" CACHE INTERNAL "bundled zlib prefix")
file(MAKE_DIRECTORY "${DAILYBOY_ZLIB_PREFIX}/include")
file(MAKE_DIRECTORY "${DAILYBOY_ZLIB_PREFIX}/lib")
if(WIN32)
    file(MAKE_DIRECTORY "${DAILYBOY_ZLIB_PREFIX}/bin")
endif()

# Windows DLL is zlib1.dll / zlibd1.dll; import lib stays zlib.lib / zlibd.lib.
set(_dailyboy_zlib_implib "")
if(WIN32)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(_dailyboy_zlib_dll_stem zlibd1)
        set(_dailyboy_zlib_implib_stem zlibd)
    else()
        set(_dailyboy_zlib_dll_stem zlib1)
        set(_dailyboy_zlib_implib_stem zlib)
    endif()
    dailyboy_bundled_implib_path(
        "${DAILYBOY_ZLIB_PREFIX}/lib" "${_dailyboy_zlib_implib_stem}" _dailyboy_zlib_implib
    )
    dailyboy_bundled_shared_lib_path(
        "${DAILYBOY_ZLIB_PREFIX}/lib" "${_dailyboy_zlib_dll_stem}" _dailyboy_zlib_lib
    )
    dailyboy_ep_imported_byproducts(
        _dailyboy_ep_byproducts "${_dailyboy_zlib_lib}" IMPLIB "${_dailyboy_zlib_implib}"
    )
else()
    set(_dailyboy_zlib_dll_stem z)
    dailyboy_bundled_shared_lib_path(
        "${DAILYBOY_ZLIB_PREFIX}/lib" "${_dailyboy_zlib_dll_stem}" _dailyboy_zlib_lib
    )
    set(_dailyboy_ep_byproducts "${_dailyboy_zlib_lib}")
endif()

dailyboy_ep_cmake_args(_dailyboy_zlib_args "${DAILYBOY_ZLIB_PREFIX}")
list(APPEND _dailyboy_zlib_args -DZLIB_BUILD_EXAMPLES=OFF)
if(MSVC)
    list(APPEND _dailyboy_zlib_args -DCMAKE_DEBUG_POSTFIX=d)
endif()

ExternalProject_Add(
    dailyboy_zlib
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG "${DAILYBOY_ZLIB_GIT_TAG}"
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED TRUE
    CMAKE_ARGS ${_dailyboy_zlib_args}
    BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --parallel ${DAILYBOY_EP_JOBS}
    INSTALL_COMMAND ${CMAKE_COMMAND} --install <BINARY_DIR>
    BUILD_BYPRODUCTS ${_dailyboy_ep_byproducts}
    USES_TERMINAL_BUILD TRUE
)

if(WIN32)
    dailyboy_add_imported_shared(
        ZLIB::ZLIB
        dailyboy_zlib
        "${_dailyboy_zlib_lib}"
        "${DAILYBOY_ZLIB_PREFIX}/include"
        IMPLIB "${_dailyboy_zlib_implib}"
    )
else()
    dailyboy_add_imported_shared(
        ZLIB::ZLIB dailyboy_zlib "${_dailyboy_zlib_lib}" "${DAILYBOY_ZLIB_PREFIX}/include"
    )
endif()
if(WIN32)
    dailyboy_install_bundled_libs("${DAILYBOY_ZLIB_PREFIX}" "zlib*")
else()
    dailyboy_install_bundled_libs("${DAILYBOY_ZLIB_PREFIX}" "${CMAKE_SHARED_LIBRARY_PREFIX}z*")
endif()

message(STATUS "deps: zlib ${DAILYBOY_ZLIB_GIT_TAG}")
unset(_dailyboy_zlib_lib)
unset(_dailyboy_zlib_args)
unset(_dailyboy_zlib_dll_stem)
unset(_dailyboy_zlib_implib_stem)
unset(_dailyboy_zlib_implib)
unset(_dailyboy_ep_byproducts)
