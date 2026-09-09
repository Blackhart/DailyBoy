# FFmpeg — video encode/decode + guide-track audio (always from source)
# Formats: MJPEG, DNxHD, ProRes (prores_ks), H.264 (libx264), H.265 (libx265), MOV, MP4
# Audio: WAV/AAC demux → AAC-LC encode, swresample
# https://ffmpeg.org/documentation.html

if(TARGET FFmpeg::FFmpeg)
    return()
endif()

if(WIN32)
    message(FATAL_ERROR "Bundled FFmpeg is not configured for Windows yet.")
endif()

if(NOT DAILYBOY_MAKE_EXECUTABLE)
    message(FATAL_ERROR "Bundled FFmpeg build requires 'make'.")
endif()

dailyboy_bundled_install_prefix(ffmpeg DAILYBOY_FFMPEG_PREFIX)
set(DAILYBOY_FFMPEG_PREFIX "${DAILYBOY_FFMPEG_PREFIX}" CACHE INTERNAL "bundled FFmpeg prefix")
set(_dailyboy_ffmpeg_libdir "${DAILYBOY_FFMPEG_PREFIX}/lib")
file(MAKE_DIRECTORY "${DAILYBOY_FFMPEG_PREFIX}/include")
file(MAKE_DIRECTORY "${_dailyboy_ffmpeg_libdir}")

set(_dailyboy_ffmpeg_libs avcodec avformat avutil swscale swresample)
set(_dailyboy_ffmpeg_byproducts "")
foreach(_lib IN LISTS _dailyboy_ffmpeg_libs)
    dailyboy_bundled_shared_lib_path("${_dailyboy_ffmpeg_libdir}" "${_lib}" _lib_path)
    list(APPEND _dailyboy_ffmpeg_byproducts "${_lib_path}")
endforeach()

set(_dailyboy_ffmpeg_asm_flag "--disable-asm")
if(DAILYBOY_NASM_EXECUTABLE)
    set(_dailyboy_ffmpeg_asm_flag "--enable-asm")
endif()

dailyboy_join_pkg_config_path(
    _dailyboy_ffmpeg_pc
    "${DAILYBOY_ZLIB_PREFIX}"
    "${DAILYBOY_X264_PREFIX}"
    "${DAILYBOY_X265_PREFIX}"
)

ExternalProject_Add(
    dailyboy_ffmpeg
    DEPENDS dailyboy_zlib dailyboy_x264 dailyboy_x265
    GIT_REPOSITORY https://github.com/FFmpeg/FFmpeg.git
    GIT_TAG "${DAILYBOY_FFMPEG_GIT_TAG}"
    GIT_SHALLOW FALSE
    UPDATE_DISCONNECTED TRUE
    CONFIGURE_COMMAND
        ${CMAKE_COMMAND} -E env
        PKG_CONFIG_PATH=${_dailyboy_ffmpeg_pc}
        <SOURCE_DIR>/configure
        --prefix=${DAILYBOY_FFMPEG_PREFIX}
        --enable-shared
        --disable-static
        --disable-programs
        --disable-doc
        --disable-debug
        --enable-gpl
        --enable-version3
        --enable-zlib
        --enable-libx264
        --enable-libx265
        --enable-swscale
        --enable-swresample
        ${_dailyboy_ffmpeg_asm_flag}
        "--extra-cflags=-I${DAILYBOY_ZLIB_PREFIX}/include -I${DAILYBOY_X264_PREFIX}/include -I${DAILYBOY_X265_PREFIX}/include"
        "--extra-ldflags=-L${DAILYBOY_ZLIB_PREFIX}/lib -L${DAILYBOY_X264_PREFIX}/lib -L${DAILYBOY_X265_PREFIX}/lib"
        --disable-network
        --disable-indevs
        --disable-outdevs
        --disable-everything
        --enable-protocol=file
        --enable-encoder=libx264,libx265,mjpeg,dnxhd,prores_ks,aac
        --enable-decoder=h264,hevc,mjpeg,dnxhd,prores,aac,pcm_s16le,pcm_s24le,pcm_s32le,pcm_f32le,pcm_f64le
        --enable-muxer=mov,mp4
        --enable-demuxer=mov,mp4,wav,aac
        --enable-parser=h264,hevc,mjpeg,dnxhd,prores,aac
        --enable-bsf=h264_mp4toannexb,hevc_mp4toannexb,aac_adtstoasc
    BUILD_COMMAND
        ${DAILYBOY_MAKE_EXECUTABLE} -C <SOURCE_DIR> -j${DAILYBOY_EP_JOBS}
    INSTALL_COMMAND ${DAILYBOY_MAKE_EXECUTABLE} -C <SOURCE_DIR> install
    BUILD_IN_SOURCE 1
    BUILD_BYPRODUCTS ${_dailyboy_ffmpeg_byproducts}
    USES_TERMINAL_BUILD TRUE
)

function(_dailyboy_add_ffmpeg_imported lib)
    dailyboy_bundled_shared_lib_path("${_dailyboy_ffmpeg_libdir}" "${lib}" _path)
    dailyboy_add_imported_shared(
        FFmpeg::${lib} dailyboy_ffmpeg "${_path}" "${DAILYBOY_FFMPEG_PREFIX}/include"
    )
endfunction()

foreach(_lib IN LISTS _dailyboy_ffmpeg_libs)
    _dailyboy_add_ffmpeg_imported(${_lib})
endforeach()

add_library(FFmpeg::FFmpeg INTERFACE IMPORTED GLOBAL)
target_link_libraries(
    FFmpeg::FFmpeg
    INTERFACE
        FFmpeg::avformat
        FFmpeg::avcodec
        FFmpeg::avutil
        FFmpeg::swscale
        FFmpeg::swresample
        FFmpeg::x264
        FFmpeg::x265
        ZLIB::ZLIB
)
target_include_directories(FFmpeg::FFmpeg INTERFACE "${DAILYBOY_FFMPEG_PREFIX}/include")

dailyboy_install_bundled_libs("${DAILYBOY_FFMPEG_PREFIX}" "${CMAKE_SHARED_LIBRARY_PREFIX}avcodec*")
dailyboy_install_bundled_libs("${DAILYBOY_FFMPEG_PREFIX}" "${CMAKE_SHARED_LIBRARY_PREFIX}avformat*")
dailyboy_install_bundled_libs("${DAILYBOY_FFMPEG_PREFIX}" "${CMAKE_SHARED_LIBRARY_PREFIX}avutil*")
dailyboy_install_bundled_libs("${DAILYBOY_FFMPEG_PREFIX}" "${CMAKE_SHARED_LIBRARY_PREFIX}swscale*")
dailyboy_install_bundled_libs("${DAILYBOY_FFMPEG_PREFIX}" "${CMAKE_SHARED_LIBRARY_PREFIX}swresample*")

message(
    STATUS
    "deps: FFmpeg ${DAILYBOY_FFMPEG_GIT_TAG} "
    "(mjpeg, dnxhd, prores_ks, h264/x264, h265/x265, aac, wav, mov, mp4)"
)

unset(_dailyboy_ffmpeg_libdir)
unset(_dailyboy_ffmpeg_libs)
unset(_dailyboy_ffmpeg_byproducts)
unset(_dailyboy_ffmpeg_asm_flag)
unset(_dailyboy_ffmpeg_pc)
