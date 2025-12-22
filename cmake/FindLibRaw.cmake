# SPDX-License-Identifier: MIT
# Local override for FindLibRaw used by bundled kimageformats.
#
# Goal: when this project builds a bundled LibRaw (target `raw` from LibRaw-cmake),
# make downstream `find_package(LibRaw)` resolve to that static library instead
# of the system libraw shared object.

# If the parent project already built LibRaw via FetchContent, prefer that.
if(TARGET raw)
    set(LibRaw_FOUND TRUE)

    # The LibRaw sources are fetched as `libraw_src` in the parent project.
    # The public headers live under <src>/libraw/.
    if(DEFINED libraw_src_SOURCE_DIR)
        set(LibRaw_INCLUDE_DIR "${libraw_src_SOURCE_DIR}")
    endif()

    # Best-effort: expose a version string if we can.
    if(DEFINED LibRaw_INCLUDE_DIR AND EXISTS "${LibRaw_INCLUDE_DIR}/libraw/libraw_version.h")
        file(READ "${LibRaw_INCLUDE_DIR}/libraw/libraw_version.h" _libraw_version_content)
        string(REGEX MATCH "#define LIBRAW_MAJOR_VERSION[ \t]*([0-9]*)\n" _major_match "${_libraw_version_content}")
        set(_major "${CMAKE_MATCH_1}")
        string(REGEX MATCH "#define LIBRAW_MINOR_VERSION[ \t]*([0-9]*)\n" _minor_match "${_libraw_version_content}")
        set(_minor "${CMAKE_MATCH_1}")
        string(REGEX MATCH "#define LIBRAW_PATCH_VERSION[ \t]*([0-9]*)\n" _patch_match "${_libraw_version_content}")
        set(_patch "${CMAKE_MATCH_1}")
        if(_major MATCHES "^[0-9]+$" AND _minor MATCHES "^[0-9]+$" AND _patch MATCHES "^[0-9]+$")
            set(LibRaw_VERSION "${_major}.${_minor}.${_patch}")
        endif()
        unset(_libraw_version_content)
        unset(_major)
        unset(_minor)
        unset(_patch)
    endif()

    # Provide the canonical CMake target used by kimageformats.
    if(NOT TARGET LibRaw::LibRaw)
        add_library(LibRaw::LibRaw INTERFACE IMPORTED)
        target_link_libraries(LibRaw::LibRaw INTERFACE raw)
        if(DEFINED LibRaw_INCLUDE_DIR)
            set_target_properties(LibRaw::LibRaw PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${LibRaw_INCLUDE_DIR}"
            )
        endif()
    endif()

    # Compatibility variables some find modules expect.
    set(LibRaw_LIBRARIES raw)
    return()
endif()

# Fallback: try the system libraw.
include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(PC_LIBRAW QUIET libraw)
    set(LibRaw_DEFINITIONS ${PC_LIBRAW_CFLAGS_OTHER})
endif()

find_path(LibRaw_INCLUDE_DIR libraw/libraw.h
    HINTS ${PC_LIBRAW_INCLUDEDIR} ${PC_LIBRAW_INCLUDE_DIRS}
)

find_library(LibRaw_LIBRARIES NAMES raw
    HINTS ${PC_LIBRAW_LIBDIR} ${PC_LIBRAW_LIBRARY_DIRS}
)

if(LibRaw_INCLUDE_DIR AND EXISTS "${LibRaw_INCLUDE_DIR}/libraw/libraw_version.h")
    file(READ "${LibRaw_INCLUDE_DIR}/libraw/libraw_version.h" _libraw_version_content)
    string(REGEX MATCH "#define LIBRAW_MAJOR_VERSION[ \t]*([0-9]*)\n" _major_match "${_libraw_version_content}")
    set(_major "${CMAKE_MATCH_1}")
    string(REGEX MATCH "#define LIBRAW_MINOR_VERSION[ \t]*([0-9]*)\n" _minor_match "${_libraw_version_content}")
    set(_minor "${CMAKE_MATCH_1}")
    string(REGEX MATCH "#define LIBRAW_PATCH_VERSION[ \t]*([0-9]*)\n" _patch_match "${_libraw_version_content}")
    set(_patch "${CMAKE_MATCH_1}")
    if(_major MATCHES "^[0-9]+$" AND _minor MATCHES "^[0-9]+$" AND _patch MATCHES "^[0-9]+$")
        set(LibRaw_VERSION "${_major}.${_minor}.${_patch}")
    endif()
endif()

find_package_handle_standard_args(LibRaw
    REQUIRED_VARS LibRaw_LIBRARIES LibRaw_INCLUDE_DIR
    VERSION_VAR LibRaw_VERSION
)

if(LibRaw_FOUND AND NOT TARGET LibRaw::LibRaw)
    add_library(LibRaw::LibRaw UNKNOWN IMPORTED)
    set_target_properties(LibRaw::LibRaw PROPERTIES
        IMPORTED_LOCATION "${LibRaw_LIBRARIES}"
        INTERFACE_COMPILE_OPTIONS "${LibRaw_DEFINITIONS}"
        INTERFACE_INCLUDE_DIRECTORIES "${LibRaw_INCLUDE_DIR}"
    )
endif()
