# x264 — H.264 encoder for FFmpeg (autotools, GPL)
# https://code.videolan.org/videolan/x264

if(TARGET FFmpeg::x264)
    return()
endif()

if(NOT DAILYBOY_MAKE_EXECUTABLE)
    message(FATAL_ERROR "Bundled x264 requires 'make'.")
endif()

dailyboy_bundled_install_prefix(x264 DAILYBOY_X264_PREFIX)
set(DAILYBOY_X264_PREFIX "${DAILYBOY_X264_PREFIX}" CACHE INTERNAL "bundled x264 prefix")
file(MAKE_DIRECTORY "${DAILYBOY_X264_PREFIX}/include")
file(MAKE_DIRECTORY "${DAILYBOY_X264_PREFIX}/lib")
dailyboy_bundled_shared_lib_path("${DAILYBOY_X264_PREFIX}/lib" x264 _dailyboy_x264_lib)

set(_dailyboy_x264_asm_flag "--disable-asm")
if(DAILYBOY_NASM_EXECUTABLE)
    set(_dailyboy_x264_asm_flag "")
endif()

ExternalProject_Add(
    dailyboy_x264
    GIT_REPOSITORY https://code.videolan.org/videolan/x264.git
    GIT_TAG "${DAILYBOY_X264_GIT_TAG}"
    GIT_SHALLOW FALSE
    UPDATE_DISCONNECTED TRUE
    CONFIGURE_COMMAND
        <SOURCE_DIR>/configure
        --prefix=${DAILYBOY_X264_PREFIX}
        --enable-shared
        --disable-static
        --disable-cli
        --enable-pic
        ${_dailyboy_x264_asm_flag}
    BUILD_COMMAND
        ${DAILYBOY_MAKE_EXECUTABLE} -C <SOURCE_DIR> -j${DAILYBOY_EP_JOBS}
    INSTALL_COMMAND ${DAILYBOY_MAKE_EXECUTABLE} -C <SOURCE_DIR> install
    BUILD_IN_SOURCE 1
    BUILD_BYPRODUCTS "${_dailyboy_x264_lib}"
    USES_TERMINAL_BUILD TRUE
)

dailyboy_add_imported_shared(
    FFmpeg::x264 dailyboy_x264 "${_dailyboy_x264_lib}" "${DAILYBOY_X264_PREFIX}/include"
)
dailyboy_install_bundled_libs("${DAILYBOY_X264_PREFIX}" "${CMAKE_SHARED_LIBRARY_PREFIX}x264*")

message(STATUS "deps: x264 ${DAILYBOY_X264_GIT_TAG}")
unset(_dailyboy_x264_lib)
unset(_dailyboy_x264_asm_flag)
