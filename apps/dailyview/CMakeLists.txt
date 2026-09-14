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
        qml/Main.qml
)

dailyboy_target_cxx_std(dailyview PRIVATE)

target_link_libraries(
    dailyview
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
)

set_target_properties(
    dailyview
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        WIN32_EXECUTABLE TRUE
        MACOSX_BUNDLE TRUE
)

# Prefer aqt Qt libs at runtime in the build tree.
if(DAILYBOY_QT_PREFIX)
    set_target_properties(
        dailyview
        PROPERTIES BUILD_RPATH "${DAILYBOY_QT_PREFIX}/lib"
    )
endif()
