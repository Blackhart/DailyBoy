# zlib — shared by FFmpeg, libpng, libtiff, libheif, OpenImageIO
# https://github.com/madler/zlib

if(TARGET ZLIB::ZLIB)
    return()
endif()

dailyboy_bundled_install_prefix(zlib DAILYBOY_ZLIB_PREFIX)
set(DAILYBOY_ZLIB_PREFIX "${DAILYBOY_ZLIB_PREFIX}" CACHE INTERNAL "bundled zlib prefix")
file(MAKE_DIRECTORY "${DAILYBOY_ZLIB_PREFIX}/include")
file(MAKE_DIRECTORY "${DAILYBOY_ZLIB_PREFIX}/lib")
dailyboy_bundled_shared_lib_path("${DAILYBOY_ZLIB_PREFIX}/lib" z _dailyboy_zlib_lib)

dailyboy_ep_cmake_args(_dailyboy_zlib_args "${DAILYBOY_ZLIB_PREFIX}")
list(APPEND _dailyboy_zlib_args -DZLIB_BUILD_EXAMPLES=OFF)

ExternalProject_Add(
    dailyboy_zlib
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG "${DAILYBOY_ZLIB_GIT_TAG}"
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED TRUE
    CMAKE_ARGS ${_dailyboy_zlib_args}
    BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --parallel ${DAILYBOY_EP_JOBS}
    INSTALL_COMMAND ${CMAKE_COMMAND} --install <BINARY_DIR>
    BUILD_BYPRODUCTS "${_dailyboy_zlib_lib}"
    USES_TERMINAL_BUILD TRUE
)

dailyboy_add_imported_shared(
    ZLIB::ZLIB dailyboy_zlib "${_dailyboy_zlib_lib}" "${DAILYBOY_ZLIB_PREFIX}/include"
)
dailyboy_install_bundled_libs("${DAILYBOY_ZLIB_PREFIX}" "${CMAKE_SHARED_LIBRARY_PREFIX}z*")

message(STATUS "deps: zlib ${DAILYBOY_ZLIB_GIT_TAG}")
unset(_dailyboy_zlib_lib)
unset(_dailyboy_zlib_args)
