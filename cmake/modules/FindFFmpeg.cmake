# FindFFmpeg.cmake — bundled prefix only (no distro / pkg-config system search).
# Used by the OpenImageIO ExternalProject via CMAKE_MODULE_PATH.

include(FindPackageHandleStandardArgs)

set(_dailyboy_ffmpeg_hint "")
if(DAILYBOY_FFMPEG_PREFIX)
    set(_dailyboy_ffmpeg_hint "${DAILYBOY_FFMPEG_PREFIX}")
elseif(FFmpeg_ROOT)
    set(_dailyboy_ffmpeg_hint "${FFmpeg_ROOT}")
elseif(FFMPEG_ROOT)
    set(_dailyboy_ffmpeg_hint "${FFMPEG_ROOT}")
endif()

set(_dailyboy_ffmpeg_libs avcodec avformat avutil swscale)

find_path(
    FFmpeg_INCLUDE_DIR
    NAMES libavcodec/avcodec.h
    HINTS ${_dailyboy_ffmpeg_hint}
    PATH_SUFFIXES include
    NO_DEFAULT_PATH
)

foreach(_lib IN LISTS _dailyboy_ffmpeg_libs)
    string(TOUPPER "${_lib}" _lib_upper)
    find_library(
        FFmpeg_${_lib_upper}_LIBRARY
        NAMES ${_lib}
        HINTS ${_dailyboy_ffmpeg_hint}
        PATH_SUFFIXES lib lib64
        NO_DEFAULT_PATH
    )
endforeach()

if(FFmpeg_INCLUDE_DIR AND EXISTS "${FFmpeg_INCLUDE_DIR}/libavutil/ffversion.h")
    file(STRINGS "${FFmpeg_INCLUDE_DIR}/libavutil/ffversion.h" _ffver_line REGEX "^#define FFMPEG_VERSION ")
    if(_ffver_line)
        string(REGEX REPLACE "^#define FFMPEG_VERSION \"(.*)\".*" "\\1" FFmpeg_VERSION "${_ffver_line}")
    endif()
endif()

if(NOT FFmpeg_VERSION)
    set(FFmpeg_VERSION "unknown")
endif()

find_package_handle_standard_args(
    FFmpeg
    REQUIRED_VARS
        FFmpeg_INCLUDE_DIR
        FFmpeg_AVCODEC_LIBRARY
        FFmpeg_AVFORMAT_LIBRARY
        FFmpeg_AVUTIL_LIBRARY
        FFmpeg_SWSCALE_LIBRARY
    VERSION_VAR FFmpeg_VERSION
)

if(FFmpeg_FOUND AND NOT TARGET FFmpeg::FFmpeg)
    foreach(_lib IN LISTS _dailyboy_ffmpeg_libs)
        string(TOUPPER "${_lib}" _lib_upper)
        if(NOT TARGET FFmpeg::${_lib})
            add_library(FFmpeg::${_lib} UNKNOWN IMPORTED)
            set_target_properties(
                FFmpeg::${_lib}
                PROPERTIES
                    IMPORTED_LOCATION "${FFmpeg_${_lib_upper}_LIBRARY}"
                    INTERFACE_INCLUDE_DIRECTORIES "${FFmpeg_INCLUDE_DIR}"
            )
        endif()
    endforeach()
    add_library(FFmpeg::FFmpeg INTERFACE IMPORTED)
    target_link_libraries(
        FFmpeg::FFmpeg
        INTERFACE FFmpeg::avformat FFmpeg::avcodec FFmpeg::avutil FFmpeg::swscale
    )
    target_include_directories(FFmpeg::FFmpeg INTERFACE "${FFmpeg_INCLUDE_DIR}")
endif()

mark_as_advanced(
    FFmpeg_INCLUDE_DIR
    FFmpeg_AVCODEC_LIBRARY
    FFmpeg_AVFORMAT_LIBRARY
    FFmpeg_AVUTIL_LIBRARY
    FFmpeg_SWSCALE_LIBRARY
)
