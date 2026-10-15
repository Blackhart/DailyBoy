# OpenColorIO — VFX CY2026 (2.5.x)
# https://github.com/AcademySoftwareFoundation/OpenColorIO
# Nested packages compiled by OCIO when missing (OCIO_INSTALL_EXT_PACKAGES=MISSING).

if(TARGET OpenColorIO::OpenColorIO)
    return()
endif()

dailyboy_bundled_install_prefix(ocio DAILYBOY_OCIO_PREFIX)
set(DAILYBOY_OCIO_PREFIX "${DAILYBOY_OCIO_PREFIX}" CACHE INTERNAL "bundled OpenColorIO prefix")
file(MAKE_DIRECTORY "${DAILYBOY_OCIO_PREFIX}/include")
file(MAKE_DIRECTORY "${DAILYBOY_OCIO_PREFIX}/lib")
if(WIN32)
    file(MAKE_DIRECTORY "${DAILYBOY_OCIO_PREFIX}/bin")
endif()

# Windows shared OCIO uses CMAKE_SHARED_LIBRARY_SUFFIX=_M_N.dll; import lib stays OpenColorIO.lib.
set(_dailyboy_ocio_implib "")
if(WIN32)
    string(REGEX REPLACE "^([0-9]+)\\.([0-9]+).*" "\\1" _dailyboy_ocio_major
        "${DAILYBOY_OPENCOLORIO_VERSION}"
    )
    string(REGEX REPLACE "^([0-9]+)\\.([0-9]+).*" "\\2" _dailyboy_ocio_minor
        "${DAILYBOY_OPENCOLORIO_VERSION}"
    )
    set(
        _dailyboy_ocio_dll_stem
        "OpenColorIO_${_dailyboy_ocio_major}_${_dailyboy_ocio_minor}"
    )
    dailyboy_bundled_implib_path(
        "${DAILYBOY_OCIO_PREFIX}/lib" OpenColorIO _dailyboy_ocio_implib
    )
else()
    set(_dailyboy_ocio_dll_stem OpenColorIO)
endif()
dailyboy_bundled_shared_lib_path(
    "${DAILYBOY_OCIO_PREFIX}/lib" "${_dailyboy_ocio_dll_stem}" _dailyboy_ocio_lib
)
if(WIN32)
    dailyboy_ep_imported_byproducts(
        _dailyboy_ocio_byproducts "${_dailyboy_ocio_lib}" IMPLIB "${_dailyboy_ocio_implib}"
    )
else()
    set(_dailyboy_ocio_byproducts "${_dailyboy_ocio_lib}")
endif()

dailyboy_ep_cmake_args(_dailyboy_ocio_args "${DAILYBOY_OCIO_PREFIX}")
list(APPEND _dailyboy_ocio_args
    -DOCIO_BUILD_PYTHON=OFF
    -DOCIO_BUILD_APPS=OFF
    -DOCIO_BUILD_TESTS=OFF
    -DOCIO_BUILD_GPU_TESTS=OFF
    -DOCIO_BUILD_DOCS=OFF
    -DOCIO_INSTALL_EXT_PACKAGES=MISSING
)

ExternalProject_Add(
    dailyboy_opencolorio
    GIT_REPOSITORY https://github.com/AcademySoftwareFoundation/OpenColorIO.git
    GIT_TAG "${DAILYBOY_OPENCOLORIO_GIT_TAG}"
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED TRUE
    CMAKE_ARGS ${_dailyboy_ocio_args}
    BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --parallel ${DAILYBOY_EP_JOBS}
    INSTALL_COMMAND ${CMAKE_COMMAND} --install <BINARY_DIR>
    BUILD_BYPRODUCTS ${_dailyboy_ocio_byproducts}
    USES_TERMINAL_BUILD TRUE
)

if(WIN32)
    dailyboy_add_imported_shared(
        OpenColorIO::OpenColorIO
        dailyboy_opencolorio
        "${_dailyboy_ocio_lib}"
        "${DAILYBOY_OCIO_PREFIX}/include"
        IMPLIB "${_dailyboy_ocio_implib}"
    )
else()
    dailyboy_add_imported_shared(
        OpenColorIO::OpenColorIO
        dailyboy_opencolorio
        "${_dailyboy_ocio_lib}"
        "${DAILYBOY_OCIO_PREFIX}/include"
    )
endif()
set_target_properties(OpenColorIO::OpenColorIO PROPERTIES INTERFACE_COMPILE_FEATURES cxx_std_17)

dailyboy_install_bundled_libs(
    "${DAILYBOY_OCIO_PREFIX}" "${CMAKE_SHARED_LIBRARY_PREFIX}OpenColorIO*"
)

message(STATUS "deps: OpenColorIO ${DAILYBOY_OPENCOLORIO_GIT_TAG}")
unset(_dailyboy_ocio_lib)
unset(_dailyboy_ocio_args)
unset(_dailyboy_ocio_dll_stem)
unset(_dailyboy_ocio_implib)
unset(_dailyboy_ocio_byproducts)
unset(_dailyboy_ocio_major)
unset(_dailyboy_ocio_minor)
