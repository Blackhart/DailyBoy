# libaom — AV1 codec for libheif AVIF encode/decode
# https://aomedia.googlesource.com/aom

if(TARGET aom::aom)
    return()
endif()

dailyboy_bundled_install_prefix(aom DAILYBOY_AOM_PREFIX)
set(DAILYBOY_AOM_PREFIX "${DAILYBOY_AOM_PREFIX}" CACHE INTERNAL "bundled libaom prefix")
file(MAKE_DIRECTORY "${DAILYBOY_AOM_PREFIX}/include")
file(MAKE_DIRECTORY "${DAILYBOY_AOM_PREFIX}/lib")
if(WIN32)
    file(MAKE_DIRECTORY "${DAILYBOY_AOM_PREFIX}/bin")
endif()
dailyboy_bundled_shared_lib_path("${DAILYBOY_AOM_PREFIX}/lib" aom _dailyboy_aom_lib)

dailyboy_ep_cmake_args(_dailyboy_aom_args "${DAILYBOY_AOM_PREFIX}")
list(APPEND _dailyboy_aom_args
    -DENABLE_DOCS=OFF
    -DENABLE_EXAMPLES=OFF
    -DENABLE_TESTDATA=OFF
    -DENABLE_TESTS=OFF
    -DENABLE_TOOLS=OFF
)

dailyboy_ep_imported_byproducts(_dailyboy_ep_byproducts "${_dailyboy_aom_lib}")

ExternalProject_Add(
    dailyboy_aom
    GIT_REPOSITORY https://aomedia.googlesource.com/aom
    GIT_TAG "${DAILYBOY_AOM_GIT_TAG}"
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED TRUE
    CMAKE_ARGS ${_dailyboy_aom_args}
    BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --parallel ${DAILYBOY_EP_JOBS}
    INSTALL_COMMAND ${CMAKE_COMMAND} --install <BINARY_DIR>
    BUILD_BYPRODUCTS ${_dailyboy_ep_byproducts}
    USES_TERMINAL_BUILD TRUE
)

dailyboy_add_imported_shared(
    aom::aom dailyboy_aom "${_dailyboy_aom_lib}" "${DAILYBOY_AOM_PREFIX}/include"
)
dailyboy_install_bundled_libs("${DAILYBOY_AOM_PREFIX}" "${CMAKE_SHARED_LIBRARY_PREFIX}aom*")

message(STATUS "deps: libaom ${DAILYBOY_AOM_GIT_TAG}")
unset(_dailyboy_aom_lib)
unset(_dailyboy_aom_args)
unset(_dailyboy_ep_byproducts)
