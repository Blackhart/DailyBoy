# x265 — H.265 encoder for FFmpeg and libheif (CMake, GPL)
# Official repo (videolan/x265 on GitHub is a stale mirror without 4.x tags).
# CMakeLists lives in source/.

if(TARGET FFmpeg::x265)
    return()
endif()

dailyboy_bundled_install_prefix(x265 DAILYBOY_X265_PREFIX)
set(DAILYBOY_X265_PREFIX "${DAILYBOY_X265_PREFIX}" CACHE INTERNAL "bundled x265 prefix")
file(MAKE_DIRECTORY "${DAILYBOY_X265_PREFIX}/include")
file(MAKE_DIRECTORY "${DAILYBOY_X265_PREFIX}/lib")
if(WIN32)
    file(MAKE_DIRECTORY "${DAILYBOY_X265_PREFIX}/bin")
endif()
dailyboy_bundled_shared_lib_path("${DAILYBOY_X265_PREFIX}/lib" x265 _dailyboy_x265_lib)

dailyboy_ep_cmake_args(_dailyboy_x265_args "${DAILYBOY_X265_PREFIX}")
list(APPEND _dailyboy_x265_args
    -DENABLE_SHARED=ON
    -DENABLE_STATIC=OFF
    -DENABLE_CLI=OFF
    -DENABLE_PIC=ON
    -DHIGH_BIT_DEPTH=OFF
    -DMAIN12=OFF
    -DENABLE_HDR10_PLUS=OFF
)
if(DAILYBOY_NASM_EXECUTABLE)
    list(APPEND _dailyboy_x265_args -DENABLE_NASM=ON)
else()
    list(APPEND _dailyboy_x265_args -DENABLE_ASSEMBLY=OFF)
endif()

dailyboy_ep_imported_byproducts(_dailyboy_ep_byproducts "${_dailyboy_x265_lib}")

ExternalProject_Add(
    dailyboy_x265
    GIT_REPOSITORY https://github.com/Multicorewareinc/x265.git
    GIT_TAG "${DAILYBOY_X265_GIT_TAG}"
    GIT_SHALLOW FALSE
    UPDATE_DISCONNECTED TRUE
    SOURCE_SUBDIR source
    CMAKE_ARGS ${_dailyboy_x265_args}
    BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --parallel ${DAILYBOY_EP_JOBS}
    INSTALL_COMMAND ${CMAKE_COMMAND} --install <BINARY_DIR>
    BUILD_BYPRODUCTS ${_dailyboy_ep_byproducts}
    USES_TERMINAL_BUILD TRUE
)

dailyboy_add_imported_shared(
    FFmpeg::x265 dailyboy_x265 "${_dailyboy_x265_lib}" "${DAILYBOY_X265_PREFIX}/include"
)
dailyboy_install_bundled_libs("${DAILYBOY_X265_PREFIX}" "${CMAKE_SHARED_LIBRARY_PREFIX}x265*")

message(STATUS "deps: x265 ${DAILYBOY_X265_GIT_TAG} (8-bit)")
unset(_dailyboy_x265_lib)
unset(_dailyboy_x265_args)
unset(_dailyboy_ep_byproducts)
