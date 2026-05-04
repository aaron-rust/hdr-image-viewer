cmake_minimum_required(VERSION 3.28)

project(hdr-image-viewer VERSION 1.0.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(QT6_MIN_VERSION 6.8.0)
set(KF6_MIN_VERSION 6.8.0)

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

# Project-local find modules (e.g. FindLibRaw override for bundled kimageformats)
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(KDEInstallDirs)
include(KDECMakeSettings)
include(ECMSetupVersion)

find_package(Qt6WaylandClient REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimize compilation speed
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Use ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
endif()

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX HDR_IMAGE_VIEWER
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/src/version-hdr-image-viewer.h"
)

find_package(Qt6 ${QT6_MIN_VERSION} REQUIRED COMPONENTS Core Gui Qml QuickControls2 Svg Widgets)
find_package(Qt6GuiPrivate ${QT6_MIN_VERSION} REQUIRED NO_MODULE)
find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS Kirigami CoreAddons I18n)

# Find libjxl for JPEG-XL HDR detection
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBJXL REQUIRED IMPORTED_TARGET libjxl)

include(FetchContent)

option(HDR_IMAGE_VIEWER_USE_BUNDLED_LIBRAW "Build LibRaw from upstream git and link it statically" ON)
set(HDR_IMAGE_VIEWER_LIBRAW_GIT_TAG "master" CACHE STRING "LibRaw git ref (branch/tag/commit) to build (upstream default branch is typically 'master')")
option(HDR_IMAGE_VIEWER_BUNDLE_KIMAGEFORMATS_RAW_PLUGIN "Build and bundle kimg_raw Qt imageformat plugin (from kimageformats) linked against bundled LibRaw" ON)

# kimageformats 'master' can require newer ECM than available on stable distros.
# Default to a release tag matching the detected ECM version to keep builds reproducible.
if(NOT DEFINED HDR_IMAGE_VIEWER_KIMAGEFORMATS_GIT_TAG)
    if(DEFINED ECM_VERSION AND ECM_VERSION VERSION_LESS "6.26.0")
        set(_hdr_image_viewer_default_kimageformats_tag "v${ECM_VERSION}")
    else()
        set(_hdr_image_viewer_default_kimageformats_tag "master")
    endif()
    set(HDR_IMAGE_VIEWER_KIMAGEFORMATS_GIT_TAG "${_hdr_image_viewer_default_kimageformats_tag}" CACHE STRING "kimageformats git ref (branch/tag/commit) to build")
endif()

# Migrate older build directories that cached 'master' before this compatibility logic.
if(DEFINED ECM_VERSION AND ECM_VERSION VERSION_LESS "6.26.0" AND HDR_IMAGE_VIEWER_KIMAGEFORMATS_GIT_TAG STREQUAL "master")
    set(HDR_IMAGE_VIEWER_KIMAGEFORMATS_GIT_TAG "v${ECM_VERSION}" CACHE STRING "kimageformats git ref (branch/tag/commit) to build" FORCE)
    message(STATUS "Adjusted HDR_IMAGE_VIEWER_KIMAGEFORMATS_GIT_TAG to ${HDR_IMAGE_VIEWER_KIMAGEFORMATS_GIT_TAG} for ECM ${ECM_VERSION}")
endif()

if(HDR_IMAGE_VIEWER_USE_BUNDLED_LIBRAW)
    # We want to link LibRaw into a shared Qt plugin (kimg_raw.so). Ensure LibRaw static libs are PIC.
    set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "Build with -fPIC" FORCE)

    # Build LibRaw as a static library so it can be compiled into our binaries.
    # Use CACHE variables to affect the LibRaw sub-build without changing consumers.
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
    set(ENABLE_EXAMPLES OFF CACHE BOOL "Build LibRaw examples" FORCE)
    set(ENABLE_OPENMP ON CACHE BOOL "Build LibRaw with OpenMP support" FORCE)
    set(ENABLE_LCMS ON CACHE BOOL "Build LibRaw with LCMS support" FORCE)
    set(ENABLE_JASPER ON CACHE BOOL "Build LibRaw with jasper support" FORCE)

    # Upstream LibRaw does not ship CMake build scripts; they live in LibRaw/LibRaw-cmake.
    FetchContent_Declare(libraw_src
        GIT_REPOSITORY https://github.com/LibRaw/LibRaw.git
        GIT_TAG ${HDR_IMAGE_VIEWER_LIBRAW_GIT_TAG}
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(libraw_src)
    FetchContent_GetProperties(libraw_src)
    if(NOT libraw_src_POPULATED)
        message(FATAL_ERROR "Failed to fetch LibRaw sources")
    endif()

    FetchContent_Declare(libraw_cmake
        GIT_REPOSITORY https://github.com/LibRaw/LibRaw-cmake.git
        GIT_TAG master
        GIT_SHALLOW TRUE
    )

    # Tell LibRaw-cmake where the actual LibRaw sources are.
    set(LIBRAW_PATH "${libraw_src_SOURCE_DIR}" CACHE STRING "Path to LibRaw sources" FORCE)
    FetchContent_MakeAvailable(libraw_cmake)
endif()

# Single link point for LibRaw: either bundled static LibRaw or the system libraw.
add_library(hdr_image_viewer_libraw INTERFACE)
if(HDR_IMAGE_VIEWER_USE_BUNDLED_LIBRAW)
    if(TARGET raw)
        target_link_libraries(hdr_image_viewer_libraw INTERFACE raw)
    elseif(TARGET LibRaw::raw)
        target_link_libraries(hdr_image_viewer_libraw INTERFACE LibRaw::raw)
    elseif(TARGET libraw::libraw)
        target_link_libraries(hdr_image_viewer_libraw INTERFACE libraw::libraw)
    elseif(TARGET libraw)
        target_link_libraries(hdr_image_viewer_libraw INTERFACE libraw)
    else()
        message(FATAL_ERROR "Bundled LibRaw was enabled, but no known LibRaw CMake target was found (tried: raw, LibRaw::raw, libraw::libraw, libraw).")
    endif()
else()
    # Fall back to the system library name. This typically resolves to -lraw.
    target_link_libraries(hdr_image_viewer_libraw INTERFACE raw)
endif()

# Provide common LibRaw alias targets for downstream projects (e.g. kimageformats)
# so they can link against our bundled build without finding the system libraw.
if(TARGET raw AND NOT TARGET LibRaw::raw)
    add_library(LibRaw::raw ALIAS raw)
endif()

# kimageformats links the RAW plugin against LibRaw::LibRaw. Define that target
# up-front so its FindLibRaw.cmake doesn't create an imported target pointing to
# the system libraw shared object.
if(HDR_IMAGE_VIEWER_USE_BUNDLED_LIBRAW AND TARGET raw AND NOT TARGET LibRaw::LibRaw)
    add_library(LibRaw::LibRaw INTERFACE IMPORTED GLOBAL)
    target_link_libraries(LibRaw::LibRaw INTERFACE raw)
    if(DEFINED libraw_src_SOURCE_DIR)
        set_target_properties(LibRaw::LibRaw PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${libraw_src_SOURCE_DIR}"
        )
    endif()
endif()

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

if(HDR_IMAGE_VIEWER_BUNDLE_KIMAGEFORMATS_RAW_PLUGIN)
    # Build KDE's Qt imageformat plugins (including kimg_raw) from source and bundle them
    # next to the application so Qt loads them before system plugins.
    FetchContent_Declare(kimageformats
        GIT_REPOSITORY https://invent.kde.org/frameworks/kimageformats.git
        GIT_TAG ${HDR_IMAGE_VIEWER_KIMAGEFORMATS_GIT_TAG}
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(kimageformats)

    if(NOT TARGET kimg_raw)
        message(FATAL_ERROR "kimageformats was enabled, but target 'kimg_raw' was not found. The kimageformats build may have changed.")
    endif()

    # Ensure the plugin uses the bundled LibRaw (static) instead of the system libraw.
    # We do this by explicitly linking our interface target.
    target_link_libraries(kimg_raw PRIVATE hdr_image_viewer_libraw)

    set(_hdr_image_viewer_plugin_root "${CMAKE_BINARY_DIR}/bin/plugins")
    set(_hdr_image_viewer_imageformats_dir "${_hdr_image_viewer_plugin_root}/imageformats")

    add_custom_command(
        OUTPUT "${_hdr_image_viewer_imageformats_dir}/kimg_raw.so"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_hdr_image_viewer_imageformats_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:kimg_raw>" "${_hdr_image_viewer_imageformats_dir}/kimg_raw.so"
        DEPENDS kimg_raw
        VERBATIM
    )
    add_custom_target(hdr_image_viewer_bundle_imageformats ALL
        DEPENDS "${_hdr_image_viewer_imageformats_dir}/kimg_raw.so"
    )
endif()

# Generate .qmlls.ini for QML Language Server support
set(QT_QML_GENERATE_QMLLS_INI ON)

# Target: static library
qt_add_qml_module(hdr_image_viewer_static
    STATIC
    URI de.aaronrust.hdrimageviewer
    VERSION 1.0
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/src/de/aaronrust/hdrimageviewer
    QML_FILES
        src/qml/Main.qml
        src/qml/ImageViewer.qml
)

qt6_generate_wayland_protocol_client_sources(hdr_image_viewer_static
    FILES "${PROJECT_SOURCE_DIR}/src/color-management-v1.xml"
)

target_sources(hdr_image_viewer_static PUBLIC
    src/app.cpp
    src/color_management.cpp
    src/file_detector.cpp
    resources/app.qrc
)

target_link_libraries(hdr_image_viewer_static PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Svg
    Qt6::Widgets
    KF6::I18n
    KF6::CoreAddons
    Qt6::WaylandClient
    PkgConfig::LIBJXL
    hdr_image_viewer_libraw
)

# Only link Qt::GuiPrivate if available
if(TARGET Qt6::GuiPrivate)
    target_link_libraries(hdr_image_viewer_static PUBLIC Qt6::GuiPrivate)
endif()
target_include_directories(hdr_image_viewer_static PUBLIC ${CMAKE_BINARY_DIR} src)

# Target: main executable
add_executable(hdr-image-viewer src/main.cpp)
target_include_directories(hdr-image-viewer PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/src src)
target_link_libraries(hdr-image-viewer PUBLIC hdr_image_viewer_static hdr_image_viewer_staticplugin)
if(TARGET hdr_image_viewer_bundle_imageformats)
    add_dependencies(hdr-image-viewer hdr_image_viewer_bundle_imageformats)
endif()
install(TARGETS hdr-image-viewer ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
