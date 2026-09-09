# libtiff — TIFF for OpenImageIO
# https://gitlab.com/libtiff/libtiff

if(TARGET TIFF::TIFF)
    return()
endif()

dailyboy_bundled_install_prefix(libtiff DAILYBOY_LIBTIFF_PREFIX)
set(DAILYBOY_LIBTIFF_PREFIX "${DAILYBOY_LIBTIFF_PREFIX}" CACHE INTERNAL "bundled libtiff prefix")
file(MAKE_DIRECTORY "${DAILYBOY_LIBTIFF_PREFIX}/include")
file(MAKE_DIRECTORY "${DAILYBOY_LIBTIFF_PREFIX}/lib")
if(WIN32)
    file(MAKE_DIRECTORY "${DAILYBOY_LIBTIFF_PREFIX}/bin")
    dailyboy_win_shared_basename(tiff _dailyboy_tiff_basename)
else()
    set(_dailyboy_tiff_basename tiff)
endif()
dailyboy_bundled_shared_lib_path(
    "${DAILYBOY_LIBTIFF_PREFIX}/lib" "${_dailyboy_tiff_basename}" _dailyboy_tiff_lib
)

dailyboy_ep_pkg_config_path_env(
    _dailyboy_tiff_pc_env "${DAILYBOY_ZLIB_PREFIX}" "${DAILYBOY_JPEG_TURBO_PREFIX}"
)
dailyboy_ep_cmake_args(_dailyboy_tiff_args "${DAILYBOY_LIBTIFF_PREFIX}")
list(APPEND _dailyboy_tiff_args
    -Dtiff-tools=OFF
    -Dtiff-tests=OFF
    -Dtiff-docs=OFF
    -Dtiff-contrib=OFF
    -Dcxx=OFF
    -Djpeg=ON
    -Dzlib=ON
    -Dlzma=OFF
    -Dzstd=OFF
    -Dwebp=OFF
    -DJPEG_ROOT=${DAILYBOY_JPEG_TURBO_PREFIX}
    -DZLIB_ROOT=${DAILYBOY_ZLIB_PREFIX}
    -DCMAKE_PREFIX_PATH=${DAILYBOY_ZLIB_PREFIX}|${DAILYBOY_JPEG_TURBO_PREFIX}
)
if(MSVC)
    list(APPEND _dailyboy_tiff_args -DCMAKE_DEBUG_POSTFIX=d)
endif()

dailyboy_ep_imported_byproducts(_dailyboy_ep_byproducts "${_dailyboy_tiff_lib}")

ExternalProject_Add(
    dailyboy_libtiff
    DEPENDS dailyboy_zlib dailyboy_jpeg_turbo
    GIT_REPOSITORY https://gitlab.com/libtiff/libtiff.git
    GIT_TAG "${DAILYBOY_LIBTIFF_GIT_TAG}"
    GIT_SHALLOW FALSE
    UPDATE_DISCONNECTED TRUE
    LIST_SEPARATOR |
    CONFIGURE_COMMAND
        ${CMAKE_COMMAND} -E env "${_dailyboy_tiff_pc_env}"
        ${CMAKE_COMMAND} -S <SOURCE_DIR> -B <BINARY_DIR> ${_dailyboy_tiff_args}
    BUILD_COMMAND
        ${CMAKE_COMMAND} --build <BINARY_DIR> --parallel ${DAILYBOY_EP_JOBS}
    INSTALL_COMMAND ${CMAKE_COMMAND} --install <BINARY_DIR>
    BUILD_BYPRODUCTS ${_dailyboy_ep_byproducts}
    USES_TERMINAL_BUILD TRUE
)

dailyboy_add_imported_shared(
    TIFF::TIFF dailyboy_libtiff "${_dailyboy_tiff_lib}" "${DAILYBOY_LIBTIFF_PREFIX}/include"
)
dailyboy_install_bundled_libs("${DAILYBOY_LIBTIFF_PREFIX}" "${CMAKE_SHARED_LIBRARY_PREFIX}tiff*")

message(STATUS "deps: libtiff ${DAILYBOY_LIBTIFF_GIT_TAG}")
unset(_dailyboy_tiff_lib)
unset(_dailyboy_tiff_args)
unset(_dailyboy_tiff_pc_env)
unset(_dailyboy_tiff_basename)
unset(_dailyboy_ep_byproducts)
