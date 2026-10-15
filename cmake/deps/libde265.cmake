# libde265 — HEVC decoder for libheif
# https://github.com/strukturag/libde265

if(TARGET de265::de265)
    return()
endif()

dailyboy_bundled_install_prefix(libde265 DAILYBOY_LIBDE265_PREFIX)
set(DAILYBOY_LIBDE265_PREFIX "${DAILYBOY_LIBDE265_PREFIX}" CACHE INTERNAL "bundled libde265 prefix")
file(MAKE_DIRECTORY "${DAILYBOY_LIBDE265_PREFIX}/include")
file(MAKE_DIRECTORY "${DAILYBOY_LIBDE265_PREFIX}/lib")
if(WIN32)
    file(MAKE_DIRECTORY "${DAILYBOY_LIBDE265_PREFIX}/bin")
endif()

# Windows installs libde265.dll + de265.lib (DLL stem differs from import lib).
set(_dailyboy_de265_implib "")
if(WIN32)
    set(_dailyboy_de265_dll_stem libde265)
    dailyboy_bundled_implib_path(
        "${DAILYBOY_LIBDE265_PREFIX}/lib" de265 _dailyboy_de265_implib
    )
else()
    set(_dailyboy_de265_dll_stem de265)
endif()
dailyboy_bundled_shared_lib_path(
    "${DAILYBOY_LIBDE265_PREFIX}/lib" "${_dailyboy_de265_dll_stem}" _dailyboy_de265_lib
)

dailyboy_ep_cmake_args(_dailyboy_de265_args "${DAILYBOY_LIBDE265_PREFIX}")
list(APPEND _dailyboy_de265_args
    -DENABLE_SDL=OFF
    -DENABLE_DECODER=ON
)

if(WIN32)
    dailyboy_ep_imported_byproducts(
        _dailyboy_ep_byproducts "${_dailyboy_de265_lib}" IMPLIB "${_dailyboy_de265_implib}"
    )
else()
    dailyboy_ep_imported_byproducts(_dailyboy_ep_byproducts "${_dailyboy_de265_lib}")
endif()

ExternalProject_Add(
    dailyboy_libde265
    GIT_REPOSITORY https://github.com/strukturag/libde265.git
    GIT_TAG "${DAILYBOY_LIBDE265_GIT_TAG}"
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED TRUE
    CMAKE_ARGS ${_dailyboy_de265_args}
    BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --parallel ${DAILYBOY_EP_JOBS}
    INSTALL_COMMAND ${CMAKE_COMMAND} --install <BINARY_DIR>
    BUILD_BYPRODUCTS ${_dailyboy_ep_byproducts}
    USES_TERMINAL_BUILD TRUE
)

if(WIN32)
    dailyboy_add_imported_shared(
        de265::de265
        dailyboy_libde265
        "${_dailyboy_de265_lib}"
        "${DAILYBOY_LIBDE265_PREFIX}/include"
        IMPLIB "${_dailyboy_de265_implib}"
    )
    dailyboy_install_bundled_libs("${DAILYBOY_LIBDE265_PREFIX}" "*de265*")
else()
    dailyboy_add_imported_shared(
        de265::de265 dailyboy_libde265 "${_dailyboy_de265_lib}" "${DAILYBOY_LIBDE265_PREFIX}/include"
    )
    dailyboy_install_bundled_libs(
        "${DAILYBOY_LIBDE265_PREFIX}" "${CMAKE_SHARED_LIBRARY_PREFIX}de265*"
    )
endif()

message(STATUS "deps: libde265 ${DAILYBOY_LIBDE265_GIT_TAG}")
unset(_dailyboy_de265_lib)
unset(_dailyboy_de265_args)
unset(_dailyboy_de265_dll_stem)
unset(_dailyboy_de265_implib)
unset(_dailyboy_ep_byproducts)
