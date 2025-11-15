cmake_minimum_required(VERSION 3.28)

project(hdr-image-viewer VERSION 1.0.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(QT6_MIN_VERSION 6.8.0)
set(KF6_MIN_VERSION 6.8.0)

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
find_package(Qt6WaylandClient REQUIRED)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(ECMSetupVersion)

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

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

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
install(TARGETS hdr-image-viewer ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
