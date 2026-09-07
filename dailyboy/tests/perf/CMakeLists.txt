# dailyboy — perf benchmarks (included only when BUILD_TESTING=ON)

set(_dailyboy_engine "${CMAKE_SOURCE_DIR}/dailyboy")
set(_dailyboy_tests_root "${CMAKE_SOURCE_DIR}/dailyboy/tests")

dailyboy_bundled_runtime_lib_dirs(_dailyboy_dep_lib_dirs)
string(JOIN ":" _dailyboy_test_rpath ${_dailyboy_dep_lib_dirs})

function(dailyboy_add_perf_benchmark name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "FILTER;TIMEOUT" "")
    add_executable(${name} ${ARG_UNPARSED_ARGUMENTS})
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
            benchmark::benchmark_main
    )
    dailyboy_target_cxx_std(${name} PRIVATE)
    target_compile_definitions(
        ${name}
        PRIVATE
            "DAILYBOY_TEST_BINARY_DIR=\"${CMAKE_CURRENT_BINARY_DIR}\""
            "DAILYBOY_PERF_DATA_DIR=\"${CMAKE_SOURCE_DIR}/dailyboy/tests/perf/data\""
    )
    set_target_properties(
        ${name}
        PROPERTIES BUILD_RPATH "${_dailyboy_test_rpath}"
    )
    set(_timeout 180)
    if(ARG_TIMEOUT)
        set(_timeout ${ARG_TIMEOUT})
    endif()
    set(_cmd
        ${name}
        --benchmark_min_time=0.05s
        --benchmark_repetitions=1
    )
    if(ARG_FILTER)
        list(APPEND _cmd "--benchmark_filter=${ARG_FILTER}")
    endif()
    add_test(NAME ${name} COMMAND ${_cmd})
    set_tests_properties(
        ${name}
        PROPERTIES
            LABELS "perf;dailyboy"
            ENVIRONMENT "LD_LIBRARY_PATH=${_dailyboy_test_rpath}"
            TIMEOUT ${_timeout}
    )
endfunction()

dailyboy_add_perf_benchmark(reformat_perf process/reformat_perf.cpp)
dailyboy_add_perf_benchmark(compose_perf process/compose_perf.cpp)
dailyboy_add_perf_benchmark(burn_ins_perf process/burn_ins_perf.cpp)
dailyboy_add_perf_benchmark(
    writers_perf
    video/writers_perf.cpp
    TIMEOUT
    600
)
dailyboy_add_perf_benchmark(
    run_perf
    process/run_perf.cpp
    FILTER
    "/10/"
)

add_test(
    NAME format_load
    COMMAND
        writers_perf
        --benchmark_min_time=0.05s
        --benchmark_repetitions=1
)
set_tests_properties(
    format_load
    PROPERTIES
        LABELS "load;dailyboy"
        ENVIRONMENT "LD_LIBRARY_PATH=${_dailyboy_test_rpath}"
        TIMEOUT 900
)

add_test(
    NAME run_load
    COMMAND
        run_perf
        --benchmark_min_time=0.05s
        --benchmark_repetitions=1
)
set_tests_properties(
    run_load
    PROPERTIES
        LABELS "load;dailyboy"
        ENVIRONMENT "LD_LIBRARY_PATH=${_dailyboy_test_rpath}"
        TIMEOUT 600
)

add_executable(generate_reformat_perf_data io/write_reformat_data.cpp)
dailyboy_target_cxx_std(generate_reformat_perf_data PRIVATE)
target_compile_definitions(
    generate_reformat_perf_data
    PRIVATE
        "DAILYBOY_PERF_DATA_DIR=\"${CMAKE_SOURCE_DIR}/dailyboy/tests/perf/data\""
)
target_link_libraries(
    generate_reformat_perf_data
    PRIVATE OpenImageIO::OpenImageIO
)
target_link_options(generate_reformat_perf_data PRIVATE "-Wl,--disable-new-dtags")
set_target_properties(
    generate_reformat_perf_data
    PROPERTIES
        BUILD_RPATH "${_dailyboy_test_rpath}"
        EXCLUDE_FROM_ALL TRUE
)
if(TARGET dailyboy_openimageio)
    add_dependencies(generate_reformat_perf_data dailyboy_openimageio)
endif()

add_custom_target(
    perf
    DEPENDS
        reformat_perf
        compose_perf
        burn_ins_perf
        writers_perf
        run_perf
)
add_custom_target(load DEPENDS writers_perf run_perf)
