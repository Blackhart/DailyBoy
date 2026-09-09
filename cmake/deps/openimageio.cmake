# OpenImageIO — tag from DAILYBOY_OPENIMAGEIO_* (CY2026: 3.0.x; CY2025: 3.0.x
# fallback — see cmake/versions/cy2025.cmake).
# https://github.com/AcademySoftwareFoundation/OpenImageIO
#
# ExternalProject (not FetchContent): OIIO configure needs already-installed
# prefixes for OCIO, TBB, JPEG, PNG, TIFF, LibRaw, HEIF, FFmpeg, zlib.
# OpenEXR / Imath: OpenImageIO_BUILD_MISSING_DEPS (OIIO 3.x).
# Formats: EXR, TIFF, OCIO, JPEG, PNG, RAW, FFmpeg, TBB, HEIF, FreeType.

if(TARGET OpenImageIO::OpenImageIO)
    return()
endif()

dailyboy_bundled_install_prefix(oiio DAILYBOY_OIIO_PREFIX)
set(DAILYBOY_OIIO_PREFIX "${DAILYBOY_OIIO_PREFIX}" CACHE INTERNAL "bundled OpenImageIO prefix")
file(MAKE_DIRECTORY "${DAILYBOY_OIIO_PREFIX}/include")
file(MAKE_DIRECTORY "${DAILYBOY_OIIO_PREFIX}/lib")
if(WIN32)
    file(MAKE_DIRECTORY "${DAILYBOY_OIIO_PREFIX}/bin")
endif()

dailyboy_bundled_build_type(_dailyboy_oiio_build_type)
if(_dailyboy_oiio_build_type STREQUAL "Debug")
    set(_dailyboy_oiio_lib_basename OpenImageIO_d)
    set(_dailyboy_oiio_util_basename OpenImageIO_Util_d)
else()
    set(_dailyboy_oiio_lib_basename OpenImageIO)
    set(_dailyboy_oiio_util_basename OpenImageIO_Util)
endif()
dailyboy_bundled_shared_lib_path(
    "${DAILYBOY_OIIO_PREFIX}/lib" "${_dailyboy_oiio_util_basename}" _dailyboy_oiio_util_lib
)
dailyboy_bundled_shared_lib_path(
    "${DAILYBOY_OIIO_PREFIX}/lib" "${_dailyboy_oiio_lib_basename}" _dailyboy_oiio_lib
)
dailyboy_ep_imported_byproducts(
    _dailyboy_oiio_byproducts
    "${_dailyboy_oiio_util_lib}" "${_dailyboy_oiio_lib}"
)

# Prefer *_ROOT / CMAKE_PREFIX_PATH over PKG_CONFIG_PATH (Windows-safe).
set(_dailyboy_oiio_prefix_path
    "${DAILYBOY_ZLIB_PREFIX}|${DAILYBOY_JPEG_TURBO_PREFIX}|${DAILYBOY_LIBPNG_PREFIX}|${DAILYBOY_LIBTIFF_PREFIX}|${DAILYBOY_LIBDE265_PREFIX}|${DAILYBOY_LIBHEIF_PREFIX}|${DAILYBOY_X264_PREFIX}|${DAILYBOY_X265_PREFIX}|${DAILYBOY_FFMPEG_PREFIX}|${DAILYBOY_TBB_PREFIX}|${DAILYBOY_OCIO_PREFIX}"
)
if(DAILYBOY_LIBRAW_PREFIX)
    string(APPEND _dailyboy_oiio_prefix_path "|${DAILYBOY_LIBRAW_PREFIX}")
endif()

dailyboy_ep_cmake_args(_dailyboy_oiio_args "${DAILYBOY_OIIO_PREFIX}")
list(APPEND _dailyboy_oiio_args
    -DUSE_PYTHON=OFF
    -DOIIO_BUILD_TESTS=OFF
    -DOIIO_BUILD_TOOLS=OFF
    -DBUILD_DOCS=OFF
    -DINSTALL_DOCS=OFF
    -DBUILD_TESTING=OFF
    -DUSE_QT=OFF
    -DSTOP_ON_WARNING=OFF
    -DOpenImageIO_BUILD_MISSING_DEPS=all
    -DOpenEXR_BUILD_VERSION=${DAILYBOY_OPENEXR_BUILD_VERSION}
    -DImath_BUILD_VERSION=${DAILYBOY_IMATH_BUILD_VERSION}
    -DOpenColorIO_DIR=${DAILYBOY_OCIO_PREFIX}/lib/cmake/OpenColorIO
    -DTBB_DIR=${DAILYBOY_TBB_PREFIX}/lib/cmake/TBB
    -DZLIB_ROOT=${DAILYBOY_ZLIB_PREFIX}
    -DJPEG_ROOT=${DAILYBOY_JPEG_TURBO_PREFIX}
    -DPNG_ROOT=${DAILYBOY_LIBPNG_PREFIX}
    -DTIFF_ROOT=${DAILYBOY_LIBTIFF_PREFIX}
    -DLibheif_ROOT=${DAILYBOY_LIBHEIF_PREFIX}
    -DFFMPEG_ROOT=${DAILYBOY_FFMPEG_PREFIX}
    -DFFmpeg_ROOT=${DAILYBOY_FFMPEG_PREFIX}
    -DCMAKE_PREFIX_PATH=${_dailyboy_oiio_prefix_path}
    -DCMAKE_MODULE_PATH=${CMAKE_SOURCE_DIR}/cmake/modules
    -DENABLE_JPEG=ON
    -DENABLE_PNG=ON
    -DENABLE_TIFF=ON
    -DENABLE_FFmpeg=ON
    -DENABLE_Libheif=ON
    -DENABLE_OpenJPEG=OFF
    -DENABLE_OpenCV=OFF
    -DENABLE_Webp=OFF
    -DENABLE_JXL=OFF
    -DENABLE_OpenVDB=OFF
    -DENABLE_GIF=OFF
    -DENABLE_Freetype=ON
    -DENABLE_OpenGL=OFF
    -DENABLE_Ptex=OFF
)
if(DAILYBOY_LIBRAW_PREFIX)
    list(APPEND _dailyboy_oiio_args
        -DLibRaw_ROOT=${DAILYBOY_LIBRAW_PREFIX}
        -DENABLE_LibRaw=ON
    )
else()
    list(APPEND _dailyboy_oiio_args -DENABLE_LibRaw=OFF)
endif()
# Ultra HDR (libuhdr) exists on OIIO 3.x only.
if(DAILYBOY_OPENIMAGEIO_VERSION VERSION_GREATER_EQUAL "3.0")
    list(APPEND _dailyboy_oiio_args -DENABLE_libuhdr=OFF)
endif()

set(_dailyboy_oiio_depends
    dailyboy_zlib
    dailyboy_onetbb
    dailyboy_opencolorio
    dailyboy_jpeg_turbo
    dailyboy_libpng
    dailyboy_libtiff
    dailyboy_libde265
    dailyboy_libheif
    dailyboy_x264
    dailyboy_x265
    dailyboy_ffmpeg
)
if(TARGET dailyboy_libraw)
    list(APPEND _dailyboy_oiio_depends dailyboy_libraw)
endif()

ExternalProject_Add(
    dailyboy_openimageio
    DEPENDS ${_dailyboy_oiio_depends}
    GIT_REPOSITORY https://github.com/AcademySoftwareFoundation/OpenImageIO.git
    GIT_TAG "${DAILYBOY_OPENIMAGEIO_GIT_TAG}"
    GIT_SHALLOW FALSE
    UPDATE_DISCONNECTED TRUE
    LIST_SEPARATOR |
    CMAKE_ARGS ${_dailyboy_oiio_args}
    BUILD_COMMAND
        ${CMAKE_COMMAND} --build <BINARY_DIR> --parallel ${DAILYBOY_EP_JOBS}
    INSTALL_COMMAND ${CMAKE_COMMAND} --install <BINARY_DIR>
    BUILD_BYPRODUCTS ${_dailyboy_oiio_byproducts}
    USES_TERMINAL_BUILD TRUE
)

dailyboy_add_imported_shared(
    OpenImageIO::OpenImageIO_Util
    dailyboy_openimageio
    "${_dailyboy_oiio_util_lib}"
    "${DAILYBOY_OIIO_PREFIX}/include"
)
set_target_properties(
    OpenImageIO::OpenImageIO_Util PROPERTIES INTERFACE_COMPILE_FEATURES cxx_std_17
)

dailyboy_add_imported_shared(
    OpenImageIO::OpenImageIO
    dailyboy_openimageio
    "${_dailyboy_oiio_lib}"
    "${DAILYBOY_OIIO_PREFIX}/include"
)
set_target_properties(
    OpenImageIO::OpenImageIO
    PROPERTIES
        INTERFACE_COMPILE_FEATURES cxx_std_17
        INTERFACE_LINK_LIBRARIES OpenImageIO::OpenImageIO_Util
)

dailyboy_install_bundled_libs(
    "${DAILYBOY_OIIO_PREFIX}" "${CMAKE_SHARED_LIBRARY_PREFIX}OpenImageIO*"
)

if(DAILYBOY_LIBRAW_PREFIX)
    set(_dailyboy_oiio_formats "EXR, TIFF, OCIO, JPEG, PNG, RAW, FFmpeg, TBB, HEIF, FreeType")
else()
    set(_dailyboy_oiio_formats "EXR, TIFF, OCIO, JPEG, PNG, FFmpeg, TBB, HEIF, FreeType")
endif()
message(
    STATUS
    "deps: OpenImageIO ${DAILYBOY_OPENIMAGEIO_GIT_TAG} (${_dailyboy_oiio_formats})"
)
unset(_dailyboy_oiio_formats)
unset(_dailyboy_oiio_depends)

unset(_dailyboy_oiio_build_type)
unset(_dailyboy_oiio_lib_basename)
unset(_dailyboy_oiio_util_basename)
unset(_dailyboy_oiio_util_lib)
unset(_dailyboy_oiio_lib)
unset(_dailyboy_oiio_prefix_path)
unset(_dailyboy_oiio_args)
unset(_dailyboy_oiio_byproducts)
