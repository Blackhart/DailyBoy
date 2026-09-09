# FFmpeg — video encode/decode + guide-track audio (always from source)
# Formats: MJPEG, DNxHD, H.264 (libx264), H.265 (libx265), MOV, MP4
# Audio: WAV/AAC demux → AAC-LC encode, swresample
# https://ffmpeg.org/documentation.html
# Windows: MSYS2 MinGW configure/make; MSVC links via import libs from DLLs.

if(TARGET FFmpeg::FFmpeg)
    return()
endif()

dailyboy_bundled_install_prefix(ffmpeg DAILYBOY_FFMPEG_PREFIX)
set(DAILYBOY_FFMPEG_PREFIX "${DAILYBOY_FFMPEG_PREFIX}" CACHE INTERNAL "bundled FFmpeg prefix")
set(_dailyboy_ffmpeg_libdir "${DAILYBOY_FFMPEG_PREFIX}/lib")
file(MAKE_DIRECTORY "${DAILYBOY_FFMPEG_PREFIX}/include")
file(MAKE_DIRECTORY "${_dailyboy_ffmpeg_libdir}")
if(WIN32)
    file(MAKE_DIRECTORY "${DAILYBOY_FFMPEG_PREFIX}/bin")
endif()

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

set(_dailyboy_ffmpeg_common_args
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
    --disable-network
    --disable-indevs
    --disable-outdevs
    --disable-everything
    --enable-protocol=file
    --enable-encoder=libx264,libx265,mjpeg,dnxhd,aac
    --enable-decoder=h264,hevc,mjpeg,dnxhd,aac,pcm_s16le,pcm_s24le,pcm_s32le,pcm_f32le,pcm_f64le
    --enable-muxer=mov,mp4
    --enable-demuxer=mov,mp4,wav,aac
    --enable-parser=h264,hevc,mjpeg,dnxhd,aac
    --enable-bsf=h264_mp4toannexb,hevc_mp4toannexb,aac_adtstoasc
)
string(JOIN " " _dailyboy_ffmpeg_common_args_str ${_dailyboy_ffmpeg_common_args})

if(WIN32)
    if(NOT DAILYBOY_MSYS2_BASH)
        message(
            FATAL_ERROR
            "Bundled FFmpeg on Windows requires MSYS2 bash "
            "(install https://www.msys2.org/ or set MSYS2_BASH)."
        )
    endif()
    # MinGW FFmpeg links against MSYS2 zlib/x265 packages + our mingw-built x264.
    # MSVC DailyBoy only imports av*.lib (DLL carries transitive deps).
    string(REPLACE "\\" "/" _dailyboy_ffmpeg_prefix_unix "${DAILYBOY_FFMPEG_PREFIX}")
    string(REPLACE "\\" "/" _dailyboy_x264_prefix_unix "${DAILYBOY_X264_PREFIX}")
    set(_dailyboy_ffmpeg_cflags "-I${_dailyboy_x264_prefix_unix}/include")
    set(_dailyboy_ffmpeg_ldflags
        "-L${_dailyboy_x264_prefix_unix}/lib -L${_dailyboy_x264_prefix_unix}/bin"
    )
    set(_dailyboy_ffmpeg_stems_sh "avcodec avformat avutil swscale swresample")
    set(_dailyboy_ffmpeg_win_sh
        "set -euo pipefail; \
export PATH=\"/mingw64/bin:\${PATH}\"; \
export PKG_CONFIG_PATH='${_dailyboy_x264_prefix_unix}/lib/pkgconfig'; \
cd '<SOURCE_DIR>'; \
./configure \
  --prefix='${_dailyboy_ffmpeg_prefix_unix}' \
  ${_dailyboy_ffmpeg_common_args_str} \
  --extra-cflags='${_dailyboy_ffmpeg_cflags}' \
  --extra-ldflags='${_dailyboy_ffmpeg_ldflags}'; \
make -j${DAILYBOY_EP_JOBS}; \
make install; \
_bin='${_dailyboy_ffmpeg_prefix_unix}/bin'; \
_lib='${_dailyboy_ffmpeg_prefix_unix}/lib'; \
mkdir -p \"\${_lib}\"; \
command -v gendef >/dev/null; \
_libexe=lib.exe; \
command -v lib.exe >/dev/null || _libexe=lib; \
for _stem in ${_dailyboy_ffmpeg_stems_sh}; do \
  _src=; \
  for _f in \"\${_bin}\"/\"\${_stem}\"-*.dll \"\${_bin}\"/\"\${_stem}\".dll; do \
    [[ -f \"\${_f}\" ]] || continue; \
    _src=\"\${_f}\"; \
    break; \
  done; \
  [[ -n \"\${_src}\" ]]; \
  cp -f \"\${_src}\" \"\${_bin}/\${_stem}.dll\"; \
  _implib=\"\${_lib}/\${_stem}.lib\"; \
  if [[ ! -f \"\${_implib}\" ]]; then \
    (cd \"\${_bin}\" && gendef \"./\${_stem}.dll\"); \
    \"\${_libexe}\" \"/def:\${_bin}/\${_stem}.def\" \"/out:\${_implib}\" /machine:x64; \
  fi; \
done"
    )
    ExternalProject_Add(
        dailyboy_ffmpeg
        DEPENDS dailyboy_x264
        GIT_REPOSITORY https://github.com/FFmpeg/FFmpeg.git
        GIT_TAG "${DAILYBOY_FFMPEG_GIT_TAG}"
        GIT_SHALLOW FALSE
        UPDATE_DISCONNECTED TRUE
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ${DAILYBOY_MSYS2_BASH} -lc "${_dailyboy_ffmpeg_win_sh}"
        INSTALL_COMMAND ""
        BUILD_IN_SOURCE 1
        BUILD_BYPRODUCTS ${_dailyboy_ffmpeg_byproducts}
        USES_TERMINAL_BUILD TRUE
    )
    unset(_dailyboy_ffmpeg_prefix_unix)
    unset(_dailyboy_x264_prefix_unix)
    unset(_dailyboy_ffmpeg_stems_sh)
    unset(_dailyboy_ffmpeg_win_sh)
else()
    if(NOT DAILYBOY_MAKE_EXECUTABLE)
        message(FATAL_ERROR "Bundled FFmpeg build requires 'make'.")
    endif()
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
            ${_dailyboy_ffmpeg_common_args}
            "--extra-cflags=-I${DAILYBOY_ZLIB_PREFIX}/include -I${DAILYBOY_X264_PREFIX}/include -I${DAILYBOY_X265_PREFIX}/include"
            "--extra-ldflags=-L${DAILYBOY_ZLIB_PREFIX}/lib -L${DAILYBOY_X264_PREFIX}/lib -L${DAILYBOY_X265_PREFIX}/lib"
        BUILD_COMMAND
            ${DAILYBOY_MAKE_EXECUTABLE} -C <SOURCE_DIR> -j${DAILYBOY_EP_JOBS}
        INSTALL_COMMAND ${DAILYBOY_MAKE_EXECUTABLE} -C <SOURCE_DIR> install
        BUILD_IN_SOURCE 1
        BUILD_BYPRODUCTS ${_dailyboy_ffmpeg_byproducts}
        USES_TERMINAL_BUILD TRUE
    )
endif()

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
if(WIN32)
    target_link_libraries(
        FFmpeg::FFmpeg
        INTERFACE
            FFmpeg::avformat
            FFmpeg::avcodec
            FFmpeg::avutil
            FFmpeg::swscale
            FFmpeg::swresample
    )
else()
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
endif()
target_include_directories(FFmpeg::FFmpeg INTERFACE "${DAILYBOY_FFMPEG_PREFIX}/include")

dailyboy_install_bundled_libs("${DAILYBOY_FFMPEG_PREFIX}" "*avcodec*")
dailyboy_install_bundled_libs("${DAILYBOY_FFMPEG_PREFIX}" "*avformat*")
dailyboy_install_bundled_libs("${DAILYBOY_FFMPEG_PREFIX}" "*avutil*")
dailyboy_install_bundled_libs("${DAILYBOY_FFMPEG_PREFIX}" "*swscale*")
dailyboy_install_bundled_libs("${DAILYBOY_FFMPEG_PREFIX}" "*swresample*")

message(
    STATUS
    "deps: FFmpeg ${DAILYBOY_FFMPEG_GIT_TAG} "
    "(mjpeg, dnxhd, h264/x264, h265/x265, aac, wav, mov, mp4)"
)

unset(_dailyboy_ffmpeg_libdir)
unset(_dailyboy_ffmpeg_libs)
unset(_dailyboy_ffmpeg_byproducts)
unset(_dailyboy_ffmpeg_asm_flag)
unset(_dailyboy_ffmpeg_pc)
unset(_dailyboy_ffmpeg_common_args)
unset(_dailyboy_ffmpeg_common_args_str)
unset(_dailyboy_ffmpeg_cflags)
unset(_dailyboy_ffmpeg_ldflags)
