# libheif — HEIF/AVIF for OpenImageIO (libde265 + x265 + libaom)
# https://github.com/strukturag/libheif

if(TARGET heif::heif)
    return()
endif()

dailyboy_bundled_install_prefix(libheif DAILYBOY_LIBHEIF_PREFIX)
set(DAILYBOY_LIBHEIF_PREFIX "${DAILYBOY_LIBHEIF_PREFIX}" CACHE INTERNAL "bundled libheif prefix")
file(MAKE_DIRECTORY "${DAILYBOY_LIBHEIF_PREFIX}/include")
file(MAKE_DIRECTORY "${DAILYBOY_LIBHEIF_PREFIX}/lib")
if(WIN32)
    file(MAKE_DIRECTORY "${DAILYBOY_LIBHEIF_PREFIX}/bin")
endif()
dailyboy_bundled_shared_lib_path("${DAILYBOY_LIBHEIF_PREFIX}/lib" heif _dailyboy_heif_lib)

dailyboy_join_pkg_config_path(
    _dailyboy_heif_pc
    "${DAILYBOY_ZLIB_PREFIX}"
    "${DAILYBOY_JPEG_TURBO_PREFIX}"
    "${DAILYBOY_LIBDE265_PREFIX}"
    "${DAILYBOY_X265_PREFIX}"
    "${DAILYBOY_AOM_PREFIX}"
)
dailyboy_ep_cmake_args(_dailyboy_heif_args "${DAILYBOY_LIBHEIF_PREFIX}")
list(APPEND _dailyboy_heif_args
    -DWITH_LIBDE265=ON
    -DWITH_X265=ON
    -DWITH_AOM_DECODER=ON
    -DWITH_AOM_ENCODER=ON
    -DWITH_DAV1D=OFF
    -DWITH_SvtEnc=OFF
    -DWITH_RAV1E=OFF
    -DWITH_JPEG_DECODER=ON
    -DWITH_JPEG_ENCODER=ON
    -DWITH_OpenJPEG_DECODER=OFF
    -DWITH_OpenJPEG_ENCODER=OFF
    -DWITH_FFMPEG_DECODER=OFF
    -DWITH_EXAMPLES=OFF
    -DBUILD_TESTING=OFF
    -DCMAKE_PREFIX_PATH=${DAILYBOY_ZLIB_PREFIX}|${DAILYBOY_JPEG_TURBO_PREFIX}|${DAILYBOY_LIBDE265_PREFIX}|${DAILYBOY_X265_PREFIX}|${DAILYBOY_AOM_PREFIX}
)

ExternalProject_Add(
    dailyboy_libheif
    DEPENDS dailyboy_zlib dailyboy_jpeg_turbo dailyboy_libde265 dailyboy_x265 dailyboy_aom
    GIT_REPOSITORY https://github.com/strukturag/libheif.git
    GIT_TAG "${DAILYBOY_LIBHEIF_GIT_TAG}"
    GIT_SHALLOW FALSE
    UPDATE_DISCONNECTED TRUE
    LIST_SEPARATOR |
    CONFIGURE_COMMAND
        ${CMAKE_COMMAND} -E env PKG_CONFIG_PATH=${_dailyboy_heif_pc}
        ${CMAKE_COMMAND} -S <SOURCE_DIR> -B <BINARY_DIR> ${_dailyboy_heif_args}
    BUILD_COMMAND
        ${CMAKE_COMMAND} --build <BINARY_DIR> --parallel ${DAILYBOY_EP_JOBS}
    INSTALL_COMMAND ${CMAKE_COMMAND} --install <BINARY_DIR>
    BUILD_BYPRODUCTS "${_dailyboy_heif_lib}"
    USES_TERMINAL_BUILD TRUE
)

dailyboy_add_imported_shared(
    heif::heif dailyboy_libheif "${_dailyboy_heif_lib}" "${DAILYBOY_LIBHEIF_PREFIX}/include"
)
dailyboy_install_bundled_libs("${DAILYBOY_LIBHEIF_PREFIX}" "${CMAKE_SHARED_LIBRARY_PREFIX}heif*")

message(STATUS "deps: libheif ${DAILYBOY_LIBHEIF_GIT_TAG}")
unset(_dailyboy_heif_lib)
unset(_dailyboy_heif_args)
unset(_dailyboy_heif_pc)
