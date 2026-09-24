# libfileseq (gofileseq C++) — VFX frame sequence paths
# https://github.com/justinfx/gofileseq/tree/master/cpp
#
# ExternalProject (not FetchContent): gofileseq's CMakeLists uses CMAKE_SOURCE_DIR,
# enable_testing(), and find_package(GTest), which would pollute the DailyBoy tree.
#
# Also used as script mode (cmake -P):
#   - PATCH: -DFILESEQ_SOURCE_DIR=... -DFILESEQ_PATCH_DIR=... (WIN32 *.patch + dirent shim)
#   - INSTALL: -DINSTALL_PREFIX=... -DBINARY_DIR=... -DSOURCE_DIR=...

if(CMAKE_SCRIPT_MODE_FILE)
    cmake_minimum_required(VERSION 3.28)

    # WIN32: apply numbered *.patch under patches/fileseq/windows/cyXXXX/, then copy dirent shim.
    if(FILESEQ_SOURCE_DIR AND FILESEQ_PATCH_DIR)
        if(NOT IS_DIRECTORY "${FILESEQ_SOURCE_DIR}")
            message(FATAL_ERROR "Missing FILESEQ_SOURCE_DIR=${FILESEQ_SOURCE_DIR}")
        endif()
        if(NOT IS_DIRECTORY "${FILESEQ_PATCH_DIR}")
            message(FATAL_ERROR "Missing FILESEQ_PATCH_DIR=${FILESEQ_PATCH_DIR}")
        endif()

        file(GLOB _dailyboy_fileseq_patches "${FILESEQ_PATCH_DIR}/*.patch")
        list(SORT _dailyboy_fileseq_patches)
        if(_dailyboy_fileseq_patches STREQUAL "")
            message(FATAL_ERROR "No .patch files in ${FILESEQ_PATCH_DIR}")
        endif()

        foreach(_patch IN LISTS _dailyboy_fileseq_patches)
            execute_process(
                COMMAND git apply --verbose --whitespace=nowarn "${_patch}"
                WORKING_DIRECTORY "${FILESEQ_SOURCE_DIR}"
                RESULT_VARIABLE _dailyboy_fileseq_patch_rc
                ERROR_VARIABLE _dailyboy_fileseq_patch_err
                OUTPUT_VARIABLE _dailyboy_fileseq_patch_out
            )
            if(NOT _dailyboy_fileseq_patch_rc EQUAL 0)
                message(
                    FATAL_ERROR
                    "fileseq: git apply failed for ${_patch}:\n"
                    "${_dailyboy_fileseq_patch_out}${_dailyboy_fileseq_patch_err}"
                )
            endif()
        endforeach()

        set(_dailyboy_fileseq_dirent_shim "${FILESEQ_PATCH_DIR}/fileseq_dirent_win.h")
        if(NOT EXISTS "${_dailyboy_fileseq_dirent_shim}")
            message(FATAL_ERROR "Missing ${_dailyboy_fileseq_dirent_shim}")
        endif()
        file(COPY "${_dailyboy_fileseq_dirent_shim}" DESTINATION "${FILESEQ_SOURCE_DIR}/cpp")
        return()
    endif()

    if(NOT INSTALL_PREFIX OR NOT BINARY_DIR OR NOT SOURCE_DIR)
        message(FATAL_ERROR "fileseq install requires INSTALL_PREFIX, BINARY_DIR, SOURCE_DIR")
    endif()

    if(EXISTS "${SOURCE_DIR}/fileseq.h")
        set(_header_dir "${SOURCE_DIR}")
    elseif(EXISTS "${SOURCE_DIR}/cpp/fileseq.h")
        set(_header_dir "${SOURCE_DIR}/cpp")
    else()
        message(FATAL_ERROR "libfileseq headers not found under ${SOURCE_DIR}")
    endif()

    file(MAKE_DIRECTORY "${INSTALL_PREFIX}/lib" "${INSTALL_PREFIX}/include/fileseq")

    set(_lib_candidates
        "${BINARY_DIR}/libfileseq.a"
        "${BINARY_DIR}/lib/libfileseq.a"
        "${BINARY_DIR}/fileseq.lib"
        "${BINARY_DIR}/lib/fileseq.lib"
        "${BINARY_DIR}/fileseq_static.lib"
        "${BINARY_DIR}/lib/fileseq_static.lib"
        "${BINARY_DIR}/Debug/fileseq.lib"
        "${BINARY_DIR}/Release/fileseq.lib"
        "${BINARY_DIR}/Debug/fileseq_static.lib"
        "${BINARY_DIR}/Release/fileseq_static.lib"
    )
    set(_lib "")
    foreach(_cand IN LISTS _lib_candidates)
        if(EXISTS "${_cand}")
            set(_lib "${_cand}")
            break()
        endif()
    endforeach()
    if(_lib STREQUAL "")
        message(FATAL_ERROR "libfileseq archive not found under ${BINARY_DIR}")
    endif()

    get_filename_component(_lib_name "${_lib}" NAME)
    file(COPY "${_lib}" DESTINATION "${INSTALL_PREFIX}/lib")
    # Normalize import/static name for DailyBoy consumers.
    if(NOT _lib_name STREQUAL "fileseq.lib" AND NOT _lib_name STREQUAL "libfileseq.a")
        if(_lib_name MATCHES "\\.lib$")
            file(RENAME "${INSTALL_PREFIX}/lib/${_lib_name}" "${INSTALL_PREFIX}/lib/fileseq.lib")
        endif()
    endif()

    # WIN32: gofileseq does not merge antlr4 into the static archive (Unix/Apple only).
    if(WIN32)
        set(_antlr_candidates
            "${BINARY_DIR}/dist/antlr4-runtime-static.lib"
            "${SOURCE_DIR}/cpp/dist/antlr4-runtime-static.lib"
            "${SOURCE_DIR}/dist/antlr4-runtime-static.lib"
            "${BINARY_DIR}/ext/antlr4/runtime/antlr4-runtime-static.lib"
            "${BINARY_DIR}/ext/antlr4/runtime/dist/antlr4-runtime-static.lib"
        )
        set(_antlr_lib "")
        foreach(_cand IN LISTS _antlr_candidates)
            if(EXISTS "${_cand}")
                set(_antlr_lib "${_cand}")
                break()
            endif()
        endforeach()
        if(_antlr_lib STREQUAL "")
            message(
                FATAL_ERROR
                "antlr4-runtime-static.lib not found under ${BINARY_DIR} or ${SOURCE_DIR}"
            )
        endif()
        file(COPY "${_antlr_lib}" DESTINATION "${INSTALL_PREFIX}/lib")
    endif()

    set(_headers fileseq.h sequence.h frameset.h pad.h error.h)
    foreach(_hdr IN LISTS _headers)
        file(COPY "${_header_dir}/${_hdr}" DESTINATION "${INSTALL_PREFIX}/include/fileseq")
    endforeach()

    file(COPY "${_header_dir}/ranges" DESTINATION "${INSTALL_PREFIX}/include/fileseq")
    return()
endif()

if(TARGET fileseq::fileseq)
    return()
endif()

dailyboy_bundled_install_prefix(fileseq _dailyboy_fileseq_install)
file(MAKE_DIRECTORY "${_dailyboy_fileseq_install}/include")
file(MAKE_DIRECTORY "${_dailyboy_fileseq_install}/lib")
dailyboy_bundled_static_lib_path("${_dailyboy_fileseq_install}/lib" fileseq _dailyboy_fileseq_lib)

dailyboy_ep_cmake_args(_dailyboy_fileseq_ep_cmake_args "${_dailyboy_fileseq_install}")
# DailyBoy only consumes the static archive; shared is unused.
list(APPEND _dailyboy_fileseq_ep_cmake_args -DBUILD_SHARED_LIBS=OFF)

set(_dailyboy_fileseq_byproducts "${_dailyboy_fileseq_lib}")
if(WIN32)
    # antlr4 defaults WITH_STATIC_CRT=ON (/MT); DailyBoy uses /MD via MSVC_RUNTIME_LIBRARY.
    list(APPEND _dailyboy_fileseq_ep_cmake_args -DWITH_STATIC_CRT=OFF)
    set(_dailyboy_fileseq_antlr_lib
        "${_dailyboy_fileseq_install}/lib/antlr4-runtime-static.lib"
    )
    list(APPEND _dailyboy_fileseq_byproducts "${_dailyboy_fileseq_antlr_lib}")
    set(_dailyboy_fileseq_patch_dir
        "${CMAKE_CURRENT_LIST_DIR}/patches/fileseq/windows/cy${DAILYBOY_VFX_PLATFORM}"
    )
    set(_dailyboy_fileseq_patch
        PATCH_COMMAND
            ${CMAKE_COMMAND}
            -DFILESEQ_SOURCE_DIR=<SOURCE_DIR>
            -DFILESEQ_PATCH_DIR=${_dailyboy_fileseq_patch_dir}
            -P ${CMAKE_CURRENT_LIST_FILE}
    )
else()
    set(_dailyboy_fileseq_antlr_lib "")
    set(_dailyboy_fileseq_patch "")
endif()

ExternalProject_Add(
    dailyboy_fileseq
    GIT_REPOSITORY https://github.com/justinfx/gofileseq.git
    GIT_TAG "${DAILYBOY_FILESEQ_GIT_TAG}"
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED TRUE
    SOURCE_SUBDIR cpp
    ${_dailyboy_fileseq_patch}
    CMAKE_ARGS ${_dailyboy_fileseq_ep_cmake_args}
    BUILD_COMMAND
        ${CMAKE_COMMAND} --build <BINARY_DIR> --target fileseq_static --parallel ${DAILYBOY_EP_JOBS}
    INSTALL_COMMAND
        ${CMAKE_COMMAND}
        -DINSTALL_PREFIX=${_dailyboy_fileseq_install}
        -DBINARY_DIR=<BINARY_DIR>
        -DSOURCE_DIR=<SOURCE_DIR>
        -P ${CMAKE_CURRENT_LIST_FILE}
    BUILD_BYPRODUCTS ${_dailyboy_fileseq_byproducts}
    USES_TERMINAL_BUILD TRUE
)

add_library(fileseq::fileseq STATIC IMPORTED GLOBAL)
add_dependencies(fileseq::fileseq dailyboy_fileseq)
set_target_properties(
    fileseq::fileseq
    PROPERTIES
        IMPORTED_LOCATION "${_dailyboy_fileseq_lib}"
        INTERFACE_INCLUDE_DIRECTORIES "${_dailyboy_fileseq_install}/include"
        INTERFACE_COMPILE_FEATURES cxx_std_14
)
if(WIN32)
    set_property(
        TARGET fileseq::fileseq
        APPEND
        PROPERTY INTERFACE_LINK_LIBRARIES "${_dailyboy_fileseq_antlr_lib}"
    )
endif()

message(STATUS "deps: libfileseq ${DAILYBOY_FILESEQ_GIT_TAG} (ExternalProject)")

unset(_dailyboy_fileseq_install)
unset(_dailyboy_fileseq_lib)
unset(_dailyboy_fileseq_antlr_lib)
unset(_dailyboy_fileseq_byproducts)
unset(_dailyboy_fileseq_patch_dir)
unset(_dailyboy_fileseq_ep_cmake_args)
unset(_dailyboy_fileseq_patch)
