# pybind11 — Python bindings
# https://github.com/pybind/pybind11
# Python interpreter/dev is a toolchain requirement (not a bundled lib).

if(TARGET pybind11::pybind11)
    return()
endif()

set(PYBIND11_FINDPYTHON ON CACHE BOOL "Use CMake FindPython for pybind11" FORCE)

if(NOT Python3_FOUND)
    set(Python3_FIND_REGISTRY NEVER)
    set(Python3_FIND_IMPLEMENTATIONS CPython)
    # Prefer the VFX-year binary when the user did not pass -DPython3_EXECUTABLE=…
    if(NOT Python3_EXECUTABLE)
        set(_dailyboy_python_hints
            "/usr/bin/python${DAILYBOY_PYTHON_VERSION}"
            "/usr/local/bin/python${DAILYBOY_PYTHON_VERSION}"
            "/opt/homebrew/bin/python${DAILYBOY_PYTHON_VERSION}"
        )
        if(WIN32)
            list(
                APPEND _dailyboy_python_hints
                "C:/Python${DAILYBOY_PYTHON_VERSION}/python.exe"
                "C:/Program Files/Python${DAILYBOY_PYTHON_VERSION}/python.exe"
            )
            # py launcher often resolves the VFX-year install.
            find_program(_dailyboy_py_launcher NAMES py py.exe)
            if(_dailyboy_py_launcher)
                execute_process(
                    COMMAND ${_dailyboy_py_launcher} -${DAILYBOY_PYTHON_VERSION} -c
                            "import sys; print(sys.executable)"
                    OUTPUT_VARIABLE _dailyboy_py_exe
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET
                )
                if(_dailyboy_py_exe)
                    list(INSERT _dailyboy_python_hints 0 "${_dailyboy_py_exe}")
                endif()
            endif()
        endif()
        foreach(_hint IN LISTS _dailyboy_python_hints)
            if(EXISTS "${_hint}")
                set(Python3_EXECUTABLE "${_hint}")
                break()
            endif()
        endforeach()
        unset(_dailyboy_python_hints)
        unset(_hint)
    endif()
    find_package(
        Python3
        ${DAILYBOY_PYTHON_VERSION}
        COMPONENTS Interpreter Development
    )
    if(NOT Python3_FOUND)
        message(
            FATAL_ERROR
            "Python ${DAILYBOY_PYTHON_VERSION} (interpreter + development headers) "
            "is required for VFX ${DAILYBOY_VFX_PLATFORM_LABEL} when "
            "DAILYBOY_BUILD_PYTHON=ON.\n"
            "  Host (Ubuntu): sudo add-apt-repository ppa:deadsnakes/ppa && "
            "sudo apt install python${DAILYBOY_PYTHON_VERSION} "
            "python${DAILYBOY_PYTHON_VERSION}-dev python${DAILYBOY_PYTHON_VERSION}-venv\n"
            "  Host (macOS):  brew install python@${DAILYBOY_PYTHON_VERSION}\n"
            "  Host (Windows): winget install Python.Python.${DAILYBOY_PYTHON_VERSION} "
            "(include development headers) or -DPython3_EXECUTABLE=…\n"
            "  Or C++ only:    cmake --preset debug -DDAILYBOY_BUILD_PYTHON=OFF"
        )
    endif()
endif()

FetchContent_Declare(
    dailyboy_pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG "${DAILYBOY_PYBIND11_GIT_TAG}"
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(dailyboy_pybind11)

if(NOT TARGET pybind11::pybind11 AND TARGET pybind11::headers)
    add_library(pybind11::pybind11 ALIAS pybind11::headers)
endif()

if(Python3_VERSION VERSION_LESS "${DAILYBOY_PYTHON_VERSION}")
    message(
        WARNING
        "Python ${Python3_VERSION} — VFX ${DAILYBOY_VFX_PLATFORM_LABEL} recommends "
        "${DAILYBOY_PYTHON_VERSION}.x for production."
    )
endif()

message(
    STATUS
    "deps: pybind11 ${DAILYBOY_PYBIND11_GIT_TAG} (Python ${Python3_VERSION})"
)
