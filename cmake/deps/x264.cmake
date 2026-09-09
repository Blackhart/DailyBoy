# x264 — H.264 encoder for FFmpeg (autotools, GPL)
# https://code.videolan.org/videolan/x264
# Windows: MSYS2 bash + configure (MinGW), then MSVC import lib from the DLL.

if(TARGET FFmpeg::x264)
    return()
endif()

dailyboy_bundled_install_prefix(x264 DAILYBOY_X264_PREFIX)
set(DAILYBOY_X264_PREFIX "${DAILYBOY_X264_PREFIX}" CACHE INTERNAL "bundled x264 prefix")
file(MAKE_DIRECTORY "${DAILYBOY_X264_PREFIX}/include")
file(MAKE_DIRECTORY "${DAILYBOY_X264_PREFIX}/lib")
if(WIN32)
    file(MAKE_DIRECTORY "${DAILYBOY_X264_PREFIX}/bin")
endif()
dailyboy_bundled_shared_lib_path("${DAILYBOY_X264_PREFIX}/lib" x264 _dailyboy_x264_lib)

set(_dailyboy_x264_asm_flag "--disable-asm")
if(DAILYBOY_NASM_EXECUTABLE)
    set(_dailyboy_x264_asm_flag "")
endif()

if(WIN32)
    if(NOT DAILYBOY_MSYS2_BASH)
        message(
            FATAL_ERROR
            "Bundled x264 on Windows requires MSYS2 bash "
            "(install https://www.msys2.org/ or set MSYS2_BASH)."
        )
    endif()
    string(REPLACE "\\" "/" _dailyboy_x264_prefix_unix "${DAILYBOY_X264_PREFIX}")
    # configure + make install under MSYS2; unversion DLL and build MSVC .lib.
    set(_dailyboy_x264_win_sh
        "set -euo pipefail; \
export PATH=\"/mingw64/bin:\${PATH}\"; \
cd '<SOURCE_DIR>'; \
./configure \
  --prefix='${_dailyboy_x264_prefix_unix}' \
  --enable-shared --disable-static --disable-cli --enable-pic \
  ${_dailyboy_x264_asm_flag}; \
make -j${DAILYBOY_EP_JOBS}; \
make install; \
_bin='${_dailyboy_x264_prefix_unix}/bin'; \
_lib='${_dailyboy_x264_prefix_unix}/lib'; \
mkdir -p \"\${_lib}\"; \
_src=; \
for _f in \"\${_bin}\"/x264-*.dll \"\${_bin}\"/x264.dll; do \
  [[ -f \"\${_f}\" ]] || continue; \
  _src=\"\${_f}\"; \
  break; \
done; \
[[ -n \"\${_src}\" ]]; \
cp -f \"\${_src}\" \"\${_bin}/x264.dll\"; \
_implib=\"\${_lib}/x264.lib\"; \
if [[ ! -f \"\${_implib}\" ]]; then \
  command -v gendef >/dev/null; \
  command -v lib.exe >/dev/null || command -v lib >/dev/null; \
  (cd \"\${_bin}\" && gendef ./x264.dll); \
  _libexe=lib.exe; \
  command -v lib.exe >/dev/null || _libexe=lib; \
  \"\${_libexe}\" \"/def:\${_bin}/x264.def\" \"/out:\${_implib}\" /machine:x64; \
fi"
    )
    ExternalProject_Add(
        dailyboy_x264
        GIT_REPOSITORY https://code.videolan.org/videolan/x264.git
        GIT_TAG "${DAILYBOY_X264_GIT_TAG}"
        GIT_SHALLOW FALSE
        UPDATE_DISCONNECTED TRUE
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ${DAILYBOY_MSYS2_BASH} -lc "${_dailyboy_x264_win_sh}"
        INSTALL_COMMAND ""
        BUILD_IN_SOURCE 1
        BUILD_BYPRODUCTS "${_dailyboy_x264_lib}"
        USES_TERMINAL_BUILD TRUE
    )
    unset(_dailyboy_x264_prefix_unix)
    unset(_dailyboy_x264_win_sh)
else()
    if(NOT DAILYBOY_MAKE_EXECUTABLE)
        message(FATAL_ERROR "Bundled x264 requires 'make'.")
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
endif()

dailyboy_add_imported_shared(
    FFmpeg::x264 dailyboy_x264 "${_dailyboy_x264_lib}" "${DAILYBOY_X264_PREFIX}/include"
)
dailyboy_install_bundled_libs("${DAILYBOY_X264_PREFIX}" "*x264*")

message(STATUS "deps: x264 ${DAILYBOY_X264_GIT_TAG}")
unset(_dailyboy_x264_lib)
unset(_dailyboy_x264_asm_flag)
