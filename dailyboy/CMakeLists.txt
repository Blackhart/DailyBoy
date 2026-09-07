add_library(dailyboy SHARED
    src/makeDaily.cpp
    src/c_api.cpp
    src/log.cpp
    src/yaml/yml_loader.cpp
    src/job/loader.cpp
    src/job/yaml_read.cpp
    src/job/schema.cpp
    src/job/parse_metadata.cpp
    src/job/parse_colorimetry.cpp
    src/job/parse_layout.cpp
    src/job/parse_output.cpp
    src/job/parse_format_options.cpp
    src/job/parse_plans.cpp
    src/image/buffer.cpp
    src/image/sequence.cpp
    src/image/sequence_writer.cpp
    src/image/text.cpp
    src/process/path_tokens.cpp
    src/process/reformat.cpp
    src/process/compositing.cpp
    src/process/tokens.cpp
    src/process/burn_ins.cpp
    src/process/slate.cpp
    src/process/colorimetry.cpp
    src/process/output.cpp
    src/process/run.cpp
    src/video/ffmpeg.cpp
    src/video/video_writer.cpp
    src/video/audio_timeline.cpp
    src/video/mov_audio_track.cpp
    src/video/h264_writer.cpp
    src/video/mjpeg_writer.cpp
    src/video/dnxhd_writer.cpp
)

dailyboy_target_cxx_std(dailyboy PUBLIC)
target_compile_definitions(dailyboy PRIVATE DAILYBOY_BUILD_SHARED)
target_compile_definitions(
    dailyboy
    PRIVATE "DAILYBOY_JOB_SCHEMA_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/schemas/job.schema.json\""
)

target_include_directories(dailyboy
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(dailyboy
    PRIVATE
        yaml-cpp::yaml-cpp
        spdlog::spdlog_header_only
        fileseq::fileseq
        nlohmann_json::nlohmann_json
        nlohmann_json_schema_validator::validator
        OpenColorIO::OpenColorIO
        OpenImageIO::OpenImageIO
        TBB::tbb
        FFmpeg::FFmpeg
)
if(BUILD_TESTING)
    dailyboy_enable_coverage(dailyboy)
endif()
dailyboy_enable_sanitizers(dailyboy)

if(TARGET dailyboy_opencolorio)
    add_dependencies(dailyboy dailyboy_opencolorio)
endif()
if(TARGET dailyboy_openimageio)
    add_dependencies(dailyboy dailyboy_openimageio)
endif()
if(TARGET dailyboy_onetbb)
    add_dependencies(dailyboy dailyboy_onetbb)
endif()
if(TARGET dailyboy_ffmpeg)
    add_dependencies(dailyboy dailyboy_ffmpeg)
endif()

set_target_properties(dailyboy PROPERTIES
    OUTPUT_NAME dailyboy
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

dailyboy_bundled_runtime_lib_dirs(_dailyboy_runtime_lib_dirs)
if(_dailyboy_runtime_lib_dirs)
    string(JOIN ":" _dailyboy_build_rpath ${_dailyboy_runtime_lib_dirs})
    set_target_properties(dailyboy PROPERTIES BUILD_RPATH "${_dailyboy_build_rpath}")
    # Consumers (makeDaily, Python) must find NEEDED of libdailyboy (avcodec →
    # x264/x265, heif → libde265). BUILD_RPATH on this shared lib is not enough
    # for GNU ld at the executable link step.
    target_link_directories(dailyboy INTERFACE ${_dailyboy_runtime_lib_dirs})
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
