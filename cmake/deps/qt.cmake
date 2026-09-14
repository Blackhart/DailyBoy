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

# Resolve installed prefix (aqt may use gcc_64 even when arch is linux_gcc_64).
set(_dailyboy_qt_prefix "")
foreach(_dailyboy_qt_arch_dir IN ITEMS
    "${_dailyboy_aqt_arch}"
    "gcc_64"
    "linux_gcc_64"
    "linux_gcc_arm64"
    "clang_64"
)
    set(
        _dailyboy_qt_candidate
        "${_dailyboy_qt_output_dir}/${DAILYBOY_QT_VERSION}/${_dailyboy_qt_arch_dir}"
    )
    if(EXISTS "${_dailyboy_qt_candidate}/lib/cmake/Qt6/Qt6Config.cmake")
        set(_dailyboy_qt_prefix "${_dailyboy_qt_candidate}")
        break()
    endif()
endforeach()

if(_dailyboy_qt_prefix STREQUAL "")
    message(
        STATUS
        "deps: Qt ${DAILYBOY_QT_VERSION} missing under ${_dailyboy_qt_output_dir}; "
        "installing with aqtinstall"
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
    )
    if(NOT _dailyboy_pip_rc EQUAL 0)
        message(
            FATAL_ERROR
            "Failed to install aqtinstall into ${_dailyboy_aqt_venv}:\n"
            "${_dailyboy_pip_err}"
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
    if(NOT _dailyboy_aqt_rc EQUAL 0)
        message(
            FATAL_ERROR
            "aqtinstall failed for Qt ${DAILYBOY_QT_VERSION} "
            "(${_dailyboy_aqt_host}/${_dailyboy_aqt_arch}):\n"
            "${_dailyboy_aqt_out}\n${_dailyboy_aqt_err}"
        )
    endif()

    foreach(_dailyboy_qt_arch_dir IN ITEMS
        "${_dailyboy_aqt_arch}"
        "gcc_64"
        "linux_gcc_64"
        "linux_gcc_arm64"
        "clang_64"
    )
        set(
            _dailyboy_qt_candidate
            "${_dailyboy_qt_output_dir}/${DAILYBOY_QT_VERSION}/${_dailyboy_qt_arch_dir}"
        )
        if(EXISTS "${_dailyboy_qt_candidate}/lib/cmake/Qt6/Qt6Config.cmake")
            set(_dailyboy_qt_prefix "${_dailyboy_qt_candidate}")
            break()
        endif()
    endforeach()
endif()

if(_dailyboy_qt_prefix STREQUAL ""
   OR NOT EXISTS "${_dailyboy_qt_prefix}/lib/cmake/Qt6/Qt6Config.cmake")
    message(
        FATAL_ERROR
        "Qt ${DAILYBOY_QT_VERSION} not found after aqtinstall "
        "(expected under ${_dailyboy_qt_output_dir})"
    )
endif()

list(PREPEND CMAKE_PREFIX_PATH "${_dailyboy_qt_prefix}")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" CACHE STRING "" FORCE)
set(DAILYBOY_QT_PREFIX "${_dailyboy_qt_prefix}" CACHE PATH "aqt Qt install prefix" FORCE)

find_package(
    Qt6
    ${DAILYBOY_QT_VERSION}
    COMPONENTS Quick QuickControls2
    REQUIRED
)

message(STATUS "deps: Qt ${Qt6_VERSION} (aqt → ${DAILYBOY_QT_PREFIX})")
