#!/bin/bash

# HDR Image Viewer Build Script
# Install build dependencies on Arch Linux:
# sudo pacman -S cmake base-devel qt6-base qt6-declarative qt6-wayland kirigami extra-cmake-modules kconfig kcoreaddons ki18n ccache

set -e

echo "Building HDR Image Viewer..."

# Configure and build
cmake -S . -B build && cmake --build build --parallel $(nproc)

echo ""
echo "Build completed successfully!"
echo "Executable: ./build/bin/hdr-image-viewer"
echo ""
echo "To run: ./build/bin/hdr-image-viewer <image-file>"