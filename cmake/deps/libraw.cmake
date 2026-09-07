# LibRaw — camera RAW for OpenImageIO (autotools)
# https://github.com/LibRaw/LibRaw

if(TARGET LibRaw::LibRaw)
    return()
endif()

if(NOT DAILYBOY_MAKE_EXECUTABLE)
    message(FATAL_ERROR "LibRaw bundled build requires 'make'.")
endif()

dailyboy_bundled_install_prefix(libraw DAILYBOY_LIBRAW_PREFIX)
set(DAILYBOY_LIBRAW_PREFIX "${DAILYBOY_LIBRAW_PREFIX}" CACHE INTERNAL "bundled LibRaw prefix")
file(MAKE_DIRECTORY "${DAILYBOY_LIBRAW_PREFIX}/include")
file(MAKE_DIRECTORY "${DAILYBOY_LIBRAW_PREFIX}/lib")
dailyboy_bundled_shared_lib_path("${DAILYBOY_LIBRAW_PREFIX}/lib" raw _dailyboy_libraw_lib)

dailyboy_join_pkg_config_path(_dailyboy_libraw_pc "${DAILYBOY_JPEG_TURBO_PREFIX}")

ExternalProject_Add(
    dailyboy_libraw
    DEPENDS dailyboy_jpeg_turbo
    GIT_REPOSITORY https://github.com/LibRaw/LibRaw.git
    GIT_TAG "${DAILYBOY_LIBRAW_GIT_TAG}"
    GIT_SHALLOW FALSE
    UPDATE_DISCONNECTED TRUE
    CONFIGURE_COMMAND
        ${CMAKE_COMMAND} -E env
        PKG_CONFIG_PATH=${_dailyboy_libraw_pc}
        CPPFLAGS=-I${DAILYBOY_JPEG_TURBO_PREFIX}/include
        LDFLAGS=-L${DAILYBOY_JPEG_TURBO_PREFIX}/lib
        bash -c "cd <SOURCE_DIR> && autoreconf --install && ./configure --prefix=${DAILYBOY_LIBRAW_PREFIX} --disable-static --enable-shared --disable-examples --disable-openmp"
    BUILD_COMMAND
        bash -c "cd <SOURCE_DIR> && ${DAILYBOY_MAKE_EXECUTABLE} -j${DAILYBOY_EP_JOBS}"
    INSTALL_COMMAND bash -c "cd <SOURCE_DIR> && ${DAILYBOY_MAKE_EXECUTABLE} install"
    BUILD_IN_SOURCE 1
    BUILD_BYPRODUCTS "${_dailyboy_libraw_lib}"
    USES_TERMINAL_BUILD TRUE
)

dailyboy_add_imported_shared(
    LibRaw::LibRaw dailyboy_libraw "${_dailyboy_libraw_lib}" "${DAILYBOY_LIBRAW_PREFIX}/include"
)
dailyboy_install_bundled_libs("${DAILYBOY_LIBRAW_PREFIX}" "${CMAKE_SHARED_LIBRARY_PREFIX}raw*")

message(STATUS "deps: LibRaw ${DAILYBOY_LIBRAW_GIT_TAG}")
unset(_dailyboy_libraw_lib)
unset(_dailyboy_libraw_pc)
