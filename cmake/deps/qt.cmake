# Qt 6 — optional DailyView UI (binary install via aqtinstall, not from source).
# Triggered only when DAILYBOY_BUILD_DAILYVIEW=ON.
# https://github.com/miurahr/aqtinstall

if(TARGET Qt6::Quick)
    return()
endif()

if(NOT DEFINED DAILYBOY_QT_VERSION)
    message(
        FATAL_ERROR
        "DAILYBOY_BUILD_DAILYVIEW requires DAILYBOY_QT_VERSION "
        "(DailyView needs CY2024–2026; set DAILYBOY_VFX_PLATFORM=2024|2025|2026)"
    )
endif()

set(_dailyboy_qt_output_dir "${CMAKE_SOURCE_DIR}/.deps/Qt")
set(_dailyboy_aqt_venv "${CMAKE_SOURCE_DIR}/.deps/aqt-venv")

# Host / arch names expected by aqtinstall (linux_gcc_64 from Qt 6.7+).
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(_dailyboy_aqt_host "linux")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
        if(DAILYBOY_QT_VERSION VERSION_GREATER_EQUAL "6.7.0")
            set(_dailyboy_aqt_arch "linux_gcc_arm64")
        else()
            set(_dailyboy_aqt_arch "gcc_arm64")
        endif()
    else()
        if(DAILYBOY_QT_VERSION VERSION_GREATER_EQUAL "6.7.0")
            set(_dailyboy_aqt_arch "linux_gcc_64")
        else()
            set(_dailyboy_aqt_arch "gcc_64")
        endif()
    endif()
elseif(APPLE)
    set(_dailyboy_aqt_host "mac")
    set(_dailyboy_aqt_arch "clang_64")
else()
    message(
        FATAL_ERROR
        "DAILYBOY_BUILD_DAILYVIEW: unsupported host '${CMAKE_SYSTEM_NAME}' "
        "(aqt desktop Qt supported on Linux and macOS)"
    )
endif()

# aqt arch arg ≠ install folder (e.g. clang_64 → macos, linux_gcc_64 → gcc_64).
function(dailyboy_qt_resolve_prefix output_dir version out_var)
    file(
        GLOB _dailyboy_qt_configs
        "${output_dir}/${version}/*/lib/cmake/Qt6/Qt6Config.cmake"
    )
    if(_dailyboy_qt_configs STREQUAL "")
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()
    list(SORT _dailyboy_qt_configs)
    list(GET _dailyboy_qt_configs 0 _dailyboy_qt_config)
    get_filename_component(_dailyboy_qt_cmake_qt6 "${_dailyboy_qt_config}" DIRECTORY)
    get_filename_component(_dailyboy_qt_cmake "${_dailyboy_qt_cmake_qt6}" DIRECTORY)
    get_filename_component(_dailyboy_qt_lib "${_dailyboy_qt_cmake}" DIRECTORY)
    get_filename_component(_dailyboy_qt_prefix "${_dailyboy_qt_lib}" DIRECTORY)
    set(${out_var} "${_dailyboy_qt_prefix}" PARENT_SCOPE)
endfunction()

dailyboy_qt_resolve_prefix(
    "${_dailyboy_qt_output_dir}"
    "${DAILYBOY_QT_VERSION}"
    _dailyboy_qt_prefix
)

if(_dailyboy_qt_prefix STREQUAL "")
    message(
        STATUS
        "deps: Qt ${DAILYBOY_QT_VERSION} missing under ${_dailyboy_qt_output_dir}; "
        "installing with aqtinstall (${_dailyboy_aqt_host}/${_dailyboy_aqt_arch})"
    )

    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    if(WIN32)
        set(_dailyboy_aqt_python "${_dailyboy_aqt_venv}/Scripts/python.exe")
    else()
        set(_dailyboy_aqt_python "${_dailyboy_aqt_venv}/bin/python")
    endif()

    if(NOT EXISTS "${_dailyboy_aqt_python}")
        execute_process(
            COMMAND "${Python3_EXECUTABLE}" -m venv "${_dailyboy_aqt_venv}"
            RESULT_VARIABLE _dailyboy_venv_rc
            ERROR_VARIABLE _dailyboy_venv_err
        )
        if(NOT _dailyboy_venv_rc EQUAL 0)
            message(
                FATAL_ERROR
                "Failed to create aqt venv at ${_dailyboy_aqt_venv}:\n"
                "${_dailyboy_venv_err}"
            )
        endif()
    endif()

    execute_process(
        COMMAND
            "${_dailyboy_aqt_python}" -m pip install --upgrade pip aqtinstall
        RESULT_VARIABLE _dailyboy_pip_rc
        ERROR_VARIABLE _dailyboy_pip_err
        OUTPUT_VARIABLE _dailyboy_pip_out
    )
    if(NOT _dailyboy_pip_rc EQUAL 0)
        message(
            FATAL_ERROR
            "Failed to install aqtinstall into ${_dailyboy_aqt_venv}:\n"
            "${_dailyboy_pip_out}\n${_dailyboy_pip_err}"
        )
    endif()

    file(MAKE_DIRECTORY "${_dailyboy_qt_output_dir}")
    execute_process(
        COMMAND
            "${_dailyboy_aqt_python}" -m aqt install-qt
            "${_dailyboy_aqt_host}"
            desktop
            "${DAILYBOY_QT_VERSION}"
            "${_dailyboy_aqt_arch}"
            --outputdir
            "${_dailyboy_qt_output_dir}"
            --modules
            qtshadertools
        RESULT_VARIABLE _dailyboy_aqt_rc
        ERROR_VARIABLE _dailyboy_aqt_err
        OUTPUT_VARIABLE _dailyboy_aqt_out
    )
    message(STATUS "aqt install-qt:\n${_dailyboy_aqt_out}${_dailyboy_aqt_err}")
    if(NOT _dailyboy_aqt_rc EQUAL 0)
        message(
            FATAL_ERROR
            "aqtinstall failed for Qt ${DAILYBOY_QT_VERSION} "
            "(${_dailyboy_aqt_host}/${_dailyboy_aqt_arch}):\n"
            "${_dailyboy_aqt_out}\n${_dailyboy_aqt_err}"
        )
    endif()

    dailyboy_qt_resolve_prefix(
        "${_dailyboy_qt_output_dir}"
        "${DAILYBOY_QT_VERSION}"
        _dailyboy_qt_prefix
    )
endif()

if(_dailyboy_qt_prefix STREQUAL ""
   OR NOT EXISTS "${_dailyboy_qt_prefix}/lib/cmake/Qt6/Qt6Config.cmake")
    file(
        GLOB _dailyboy_qt_tree
        RELATIVE "${_dailyboy_qt_output_dir}"
        "${_dailyboy_qt_output_dir}/*/*"
    )
    message(
        FATAL_ERROR
        "Qt ${DAILYBOY_QT_VERSION} not found after aqtinstall "
        "(expected under ${_dailyboy_qt_output_dir}). "
        "Entries: ${_dailyboy_qt_tree}"
    )
endif()

list(PREPEND CMAKE_PREFIX_PATH "${_dailyboy_qt_prefix}")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" CACHE STRING "" FORCE)
set(DAILYBOY_QT_PREFIX "${_dailyboy_qt_prefix}" CACHE PATH "aqt Qt install prefix" FORCE)

find_package(
    Qt6
    ${DAILYBOY_QT_VERSION}
    COMPONENTS Quick QuickControls2
)
if(NOT Qt6_FOUND)
    message(
        FATAL_ERROR
        "find_package(Qt6 Quick) failed at ${DAILYBOY_QT_PREFIX}. "
        "Qt Gui/Quick need system OpenGL and XKB (e.g. Ubuntu: "
        "libgl1-mesa-dev libopengl-dev libxkbcommon-dev; "
        "rebuild CI images after docker/*/Dockerfile.*.base updates)."
    )
endif()

message(STATUS "deps: Qt ${Qt6_VERSION} (aqt → ${DAILYBOY_QT_PREFIX})")
