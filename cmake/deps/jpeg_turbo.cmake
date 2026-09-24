# libjpeg-turbo — JPEG for OpenImageIO / libtiff / LibRaw / libheif
# https://github.com/libjpeg-turbo/libjpeg-turbo

if(TARGET JPEG::JPEG)
    return()
endif()

dailyboy_bundled_install_prefix(jpeg-turbo DAILYBOY_JPEG_TURBO_PREFIX)
set(DAILYBOY_JPEG_TURBO_PREFIX "${DAILYBOY_JPEG_TURBO_PREFIX}" CACHE INTERNAL "bundled jpeg-turbo prefix")
file(MAKE_DIRECTORY "${DAILYBOY_JPEG_TURBO_PREFIX}/include")
file(MAKE_DIRECTORY "${DAILYBOY_JPEG_TURBO_PREFIX}/lib")
if(WIN32)
    file(MAKE_DIRECTORY "${DAILYBOY_JPEG_TURBO_PREFIX}/bin")
endif()

# WITH_JPEG8=ON installs jpeg8[.d].dll; import lib stays jpeg[.d].lib.
set(_dailyboy_jpeg_implib "")
if(WIN32)
    dailyboy_win_shared_basename(jpeg8 _dailyboy_jpeg_dll_stem)
    dailyboy_win_shared_basename(jpeg _dailyboy_jpeg_implib_stem)
    dailyboy_bundled_implib_path(
        "${DAILYBOY_JPEG_TURBO_PREFIX}/lib" "${_dailyboy_jpeg_implib_stem}" _dailyboy_jpeg_implib
    )
else()
    set(_dailyboy_jpeg_dll_stem jpeg)
endif()
dailyboy_bundled_shared_lib_path(
    "${DAILYBOY_JPEG_TURBO_PREFIX}/lib" "${_dailyboy_jpeg_dll_stem}" _dailyboy_jpeg_lib
)

dailyboy_ep_cmake_args(_dailyboy_jpeg_args "${DAILYBOY_JPEG_TURBO_PREFIX}")
list(APPEND _dailyboy_jpeg_args
    -DENABLE_SHARED=ON
    -DENABLE_STATIC=OFF
    -DWITH_TURBOJPEG=ON
    -DWITH_JPEG8=ON
)
if(MSVC)
    list(APPEND _dailyboy_jpeg_args -DCMAKE_DEBUG_POSTFIX=d)
endif()

if(WIN32)
    dailyboy_ep_imported_byproducts(
        _dailyboy_ep_byproducts "${_dailyboy_jpeg_lib}" IMPLIB "${_dailyboy_jpeg_implib}"
    )
else()
    dailyboy_ep_imported_byproducts(_dailyboy_ep_byproducts "${_dailyboy_jpeg_lib}")
endif()

ExternalProject_Add(
    dailyboy_jpeg_turbo
    GIT_REPOSITORY https://github.com/libjpeg-turbo/libjpeg-turbo.git
    GIT_TAG "${DAILYBOY_JPEG_TURBO_GIT_TAG}"
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED TRUE
    CMAKE_ARGS ${_dailyboy_jpeg_args}
    BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --parallel ${DAILYBOY_EP_JOBS}
    INSTALL_COMMAND ${CMAKE_COMMAND} --install <BINARY_DIR>
    BUILD_BYPRODUCTS ${_dailyboy_ep_byproducts}
    USES_TERMINAL_BUILD TRUE
)

if(WIN32)
    dailyboy_add_imported_shared(
        JPEG::JPEG
        dailyboy_jpeg_turbo
        "${_dailyboy_jpeg_lib}"
        "${DAILYBOY_JPEG_TURBO_PREFIX}/include"
        IMPLIB "${_dailyboy_jpeg_implib}"
    )
else()
    dailyboy_add_imported_shared(
        JPEG::JPEG dailyboy_jpeg_turbo "${_dailyboy_jpeg_lib}" "${DAILYBOY_JPEG_TURBO_PREFIX}/include"
    )
endif()
dailyboy_install_bundled_libs("${DAILYBOY_JPEG_TURBO_PREFIX}" "*jpeg*")

message(STATUS "deps: libjpeg-turbo ${DAILYBOY_JPEG_TURBO_GIT_TAG}")
unset(_dailyboy_jpeg_lib)
unset(_dailyboy_jpeg_args)
unset(_dailyboy_jpeg_dll_stem)
unset(_dailyboy_jpeg_implib)
unset(_dailyboy_jpeg_implib_stem)
unset(_dailyboy_ep_byproducts)
