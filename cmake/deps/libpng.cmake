# libpng — PNG for OpenImageIO
# https://github.com/pnggroup/libpng

if(TARGET PNG::PNG)
    return()
endif()

dailyboy_bundled_install_prefix(libpng DAILYBOY_LIBPNG_PREFIX)
set(DAILYBOY_LIBPNG_PREFIX "${DAILYBOY_LIBPNG_PREFIX}" CACHE INTERNAL "bundled libpng prefix")
file(MAKE_DIRECTORY "${DAILYBOY_LIBPNG_PREFIX}/include")
file(MAKE_DIRECTORY "${DAILYBOY_LIBPNG_PREFIX}/lib")
dailyboy_bundled_shared_lib_path("${DAILYBOY_LIBPNG_PREFIX}/lib" png _dailyboy_png_lib)

dailyboy_join_pkg_config_path(_dailyboy_png_pc "${DAILYBOY_ZLIB_PREFIX}")
dailyboy_ep_cmake_args(_dailyboy_png_args "${DAILYBOY_LIBPNG_PREFIX}")
list(APPEND _dailyboy_png_args
    -DPNG_SHARED=ON
    -DPNG_STATIC=OFF
    -DPNG_TESTS=OFF
    -DSKIP_INSTALL_PROGRAMS=ON
    -DZLIB_ROOT=${DAILYBOY_ZLIB_PREFIX}
    -DCMAKE_PREFIX_PATH=${DAILYBOY_ZLIB_PREFIX}
)

ExternalProject_Add(
    dailyboy_libpng
    DEPENDS dailyboy_zlib
    GIT_REPOSITORY https://github.com/pnggroup/libpng.git
    GIT_TAG "${DAILYBOY_LIBPNG_GIT_TAG}"
    GIT_SHALLOW FALSE
    UPDATE_DISCONNECTED TRUE
    CONFIGURE_COMMAND
        ${CMAKE_COMMAND} -E env PKG_CONFIG_PATH=${_dailyboy_png_pc}
        ${CMAKE_COMMAND} -S <SOURCE_DIR> -B <BINARY_DIR> ${_dailyboy_png_args}
    BUILD_COMMAND
        ${CMAKE_COMMAND} --build <BINARY_DIR> --parallel ${DAILYBOY_EP_JOBS}
    INSTALL_COMMAND ${CMAKE_COMMAND} --install <BINARY_DIR>
    BUILD_BYPRODUCTS "${_dailyboy_png_lib}"
    USES_TERMINAL_BUILD TRUE
)

dailyboy_add_imported_shared(
    PNG::PNG dailyboy_libpng "${_dailyboy_png_lib}" "${DAILYBOY_LIBPNG_PREFIX}/include"
)
dailyboy_install_bundled_libs("${DAILYBOY_LIBPNG_PREFIX}" "${CMAKE_SHARED_LIBRARY_PREFIX}png*")

message(STATUS "deps: libpng ${DAILYBOY_LIBPNG_GIT_TAG}")
unset(_dailyboy_png_lib)
unset(_dailyboy_png_args)
unset(_dailyboy_png_pc)
