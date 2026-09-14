# DailyView (Qt Quick media viewer). Optional: DAILYBOY_BUILD_DAILYVIEW=ON.

set(CMAKE_AUTOMOC ON)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(dailyview
    main.cpp
)

qt_add_qml_module(dailyview
    URI DailyBoy.DailyView
    VERSION 1.0
    QML_FILES
        View/qml/Main.qml
        View/qml/TransportBar.qml
    SOURCES
        Model/load_frame_result.hpp
        Model/job_runner.hpp
        Model/job_runner.cpp
        Model/sequence_model.hpp
        Model/sequence_model.cpp
        View/viewport_item.hpp
        View/viewport_item.cpp
)

dailyboy_target_cxx_std(dailyview PRIVATE)

target_include_directories(
    dailyview
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/Model"
        "${CMAKE_CURRENT_SOURCE_DIR}/View"
        "${CMAKE_SOURCE_DIR}/dailyboy/src"
)

target_compile_definitions(
    dailyview
    PRIVATE
        "DAILYBOY_SOURCE_DIR=\"${CMAKE_SOURCE_DIR}\""
)

target_link_libraries(
    dailyview
    PRIVATE
        dailyboy
        TBB::tbb
        Qt6::Quick
        Qt6::QuickControls2
        OpenImageIO::OpenImageIO
)

dailyboy_bundled_runtime_lib_dirs(_dailyboy_dailyview_lib_dirs)
set(_dailyboy_dailyview_rpath_dirs ${_dailyboy_dailyview_lib_dirs})
if(DAILYBOY_QT_PREFIX)
    list(APPEND _dailyboy_dailyview_rpath_dirs "${DAILYBOY_QT_PREFIX}/lib")
endif()
string(JOIN ":" _dailyboy_dailyview_rpath ${_dailyboy_dailyview_rpath_dirs})

set_target_properties(
    dailyview
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        WIN32_EXECUTABLE TRUE
        MACOSX_BUNDLE TRUE
        BUILD_RPATH "${_dailyboy_dailyview_rpath}"
)

if(NOT APPLE)
    target_link_options(dailyview PRIVATE "-Wl,--disable-new-dtags")
endif()

if(TARGET dailyboy_openimageio)
    add_dependencies(dailyview dailyboy_openimageio)
endif()
