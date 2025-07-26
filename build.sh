#!/bin/bash

# Install build dependencies on Arch Linux
# sudo pacman -S cmake base-devel qt6-base qt6-declarative qt6-wayland kirigami extra-cmake-modules kconfig kcoreaddons ki18n ccache

# Configure and build
cmake -S . -B build && cmake --build build --parallel $(nproc)