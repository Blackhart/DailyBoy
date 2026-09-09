# Example tools (not installed)

dailyboy_bundled_runtime_lib_dirs(_dailyboy_example_lib_dirs)
string(JOIN ":" _dailyboy_example_rpath ${_dailyboy_example_lib_dirs})

add_executable(generate_bouncing_circle generate_bouncing_circle.cpp)
dailyboy_target_cxx_std(generate_bouncing_circle PRIVATE)
target_compile_definitions(
    generate_bouncing_circle
    PRIVATE
        "DAILYBOY_BOUNCE_OUT_DIR=\"${CMAKE_SOURCE_DIR}/examples/frames/1920x1080_bouncing\""
)
target_link_libraries(generate_bouncing_circle PRIVATE OpenImageIO::OpenImageIO)
# DT_RPATH (not RUNPATH) is searched for libheif's NEEDED (x265, libde265).
if(NOT APPLE)
    target_link_options(generate_bouncing_circle PRIVATE "-Wl,--disable-new-dtags")
endif()
set_target_properties(
    generate_bouncing_circle
    PROPERTIES BUILD_RPATH "${_dailyboy_example_rpath}"
)
if(TARGET dailyboy_openimageio)
    add_dependencies(generate_bouncing_circle dailyboy_openimageio)
endif()
