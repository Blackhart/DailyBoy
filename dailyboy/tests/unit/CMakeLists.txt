# dailyboy — unit tests (included only when BUILD_TESTING=ON)

set(_dailyboy_engine "${CMAKE_SOURCE_DIR}/dailyboy")
set(_dailyboy_tests_root "${CMAKE_SOURCE_DIR}/dailyboy/tests")

dailyboy_bundled_runtime_lib_dirs(_dailyboy_dep_lib_dirs)
string(JOIN ":" _dailyboy_test_rpath ${_dailyboy_dep_lib_dirs})

# --- YmlLoader (GoogleTest) ---

add_executable(
    yml_loader_test
    yaml/yml_loader_test.cpp
    ${_dailyboy_engine}/src/yaml/yml_loader.cpp
)
target_include_directories(
    yml_loader_test
    PRIVATE
        ${_dailyboy_engine}/include
        ${_dailyboy_engine}/src
)
target_compile_definitions(
    yml_loader_test
    PRIVATE
        "DAILYBOY_TEST_DATA_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/yaml/data\""
        "DAILYBOY_TEST_BINARY_DIR=\"${CMAKE_CURRENT_BINARY_DIR}\""
)
target_link_libraries(
    yml_loader_test
    PRIVATE GTest::gtest_main yaml-cpp::yaml-cpp
)
dailyboy_target_cxx_std(yml_loader_test PRIVATE)
dailyboy_enable_sanitizers(yml_loader_test)
dailyboy_gtest_discover(yml_loader_test)

# --- JobLoader (GoogleTest) ---

add_executable(job_loader_test job/job_loader_test.cpp)
target_include_directories(
    job_loader_test
    PRIVATE
        ${_dailyboy_engine}/include
        ${_dailyboy_engine}/src
)
target_compile_definitions(
    job_loader_test
    PRIVATE
        "DAILYBOY_TEST_DATA_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/job/data\""
        "DAILYBOY_TEST_BINARY_DIR=\"${CMAKE_CURRENT_BINARY_DIR}\""
)
target_link_libraries(job_loader_test PRIVATE dailyboy GTest::gtest_main)
target_link_directories(job_loader_test PRIVATE ${_dailyboy_dep_lib_dirs})
dailyboy_target_cxx_std(job_loader_test PRIVATE)
set_target_properties(
    job_loader_test
    PROPERTIES BUILD_RPATH "${_dailyboy_test_rpath}"
)
dailyboy_gtest_discover(job_loader_test)
dailyboy_enable_sanitizers(job_loader_test)

# --- Sequence / H.264 / render (GoogleTest) ---

function(dailyboy_add_engine_test name)
    add_executable(${name} ${ARGN})
    target_include_directories(
        ${name}
        PRIVATE
            ${_dailyboy_engine}/include
            ${_dailyboy_engine}/src
            ${_dailyboy_tests_root}
    )
    target_link_libraries(
        ${name}
        PRIVATE
            dailyboy
            OpenImageIO::OpenImageIO
            FFmpeg::FFmpeg
            GTest::gtest_main
    )
    dailyboy_target_cxx_std(${name} PRIVATE)
    target_compile_definitions(
        ${name}
        PRIVATE "DAILYBOY_TEST_BINARY_DIR=\"${CMAKE_CURRENT_BINARY_DIR}\""
    )
    set_target_properties(${name} PROPERTIES BUILD_RPATH "${_dailyboy_test_rpath}")
    dailyboy_enable_sanitizers(${name})
    dailyboy_gtest_discover(${name})
endfunction()

dailyboy_add_engine_test(sequence_test image/sequence_test.cpp)
dailyboy_add_engine_test(sequence_writer_test image/sequence_writer_test.cpp)
dailyboy_add_engine_test(buffer_test image/buffer_test.cpp)
dailyboy_add_engine_test(geom_test image/geom_test.cpp)
dailyboy_add_engine_test(text_test image/text_test.cpp)
dailyboy_add_engine_test(reformat_test process/reformat_test.cpp)
dailyboy_add_engine_test(compose_test process/compose_test.cpp)
dailyboy_add_engine_test(tokens_test process/tokens_test.cpp)
dailyboy_add_engine_test(path_tokens_test process/path_tokens_test.cpp)
dailyboy_add_engine_test(burn_ins_test process/burn_ins_test.cpp)
dailyboy_add_engine_test(slate_test process/slate_test.cpp)
dailyboy_add_engine_test(colorimetry_test process/colorimetry_test.cpp)
dailyboy_add_engine_test(output_test process/output_test.cpp)
dailyboy_add_engine_test(h264_writer_test video/h264_writer_test.cpp)
dailyboy_add_engine_test(mjpeg_writer_test video/mjpeg_writer_test.cpp)
dailyboy_add_engine_test(dnxhd_writer_test video/dnxhd_writer_test.cpp)
dailyboy_add_engine_test(audio_timeline_test video/audio_timeline_test.cpp)
target_link_libraries(audio_timeline_test PRIVATE yaml-cpp::yaml-cpp)
dailyboy_add_engine_test(run_test process/run_test.cpp)

dailyboy_add_engine_test(yaml_read_test job/yaml_read_test.cpp)
target_link_libraries(
    yaml_read_test
    PRIVATE yaml-cpp::yaml-cpp nlohmann_json::nlohmann_json fileseq::fileseq
)
dailyboy_add_engine_test(parse_test job/parse_test.cpp)
target_link_libraries(parse_test PRIVATE yaml-cpp::yaml-cpp)
