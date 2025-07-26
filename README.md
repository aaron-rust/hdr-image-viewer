# HDR Image Viewer

An image viewer for displaying High Dynamic Range (HDR) images on Linux systems.

## Description

HDR Image Viewer is an application for viewing HDR images with accurate color reproduction on Linux. It supports PNG and AVIF images in BT.2020 PQ/ST.2084 color encoding and integrates with the Wayland color-management-v1 protocol for proper HDR display. It has been developed and tested under KDE Plasma 6 in Wayland mode with HDR enabled on a HDR1000 certified display.

## Features

- Display of HDR images via the Wayland color-management-v1 protocol
- 100x zoom with cursor-centered scaling
- Persistent zoom/pan state for image comparison
- Fullscreen mode (F11 or double-click)
- Keyboard navigation
- PNG and AVIF format support
- Large file handling (up to 1 GiB)

## Requirements

- Linux system
- Wayland compositor with HDR support (f.e. KDE Plasma)
- HDR display

### Supported Formats
- HDR PNG (.png) with BT.2020 PQ (ST.2084)
- HDR AVIF (.avif) with BT.2020 PQ (ST.2084)

## Installation

### From Source

1. **Install dependencies:**
   
   1.1 **Ubuntu/Debian:**
   ```bash
   sudo apt install qt6-base-dev qt6-declarative-dev qt6-wayland-dev \
                    libkf6kirigami2-dev libkf6coreaddons-dev libkf6config-dev \
                    libkf6i18n-dev extra-cmake-modules cmake build-essential
   ```

   1.2 **Arch Linux:**
   ```bash
   sudo pacman -S cmake base-devel qt6-base qt6-declarative qt6-wayland kirigami extra-cmake-modules kconfig kcoreaddons ki18n ccache
   ```

2. **Clone and build:**
   ```bash
   git clone https://github.com/aaron-rust/hdr-image-viewer.git
   cd hdr-image-viewer
   ./build.sh
   ```

## Usage

```bash
./hdr-image-viewer path/to/image.png
./hdr-image-viewer path/to/image.avif
```

### Controls
- **Right Arrow**, **D**, **Page Down**, or **Space**: Navigate to next image
- **Left Arrow**, **A**, **Page Up**, or **Backspace**: Navigate to previous image
- **Mouse wheel**: Zoom in/out
- **Click + drag**: Pan (when zoomed) or move window
- **F** or **F11**: Toggle fullscreen
- **+/-**: Zoom in/out
- **0**: Reset zoom
- **Q**: Quit

## License

MIT License - see [LICENSE.md](LICENSE.md)

## Acknowledgments

Based on [colortest](https://invent.kde.org/zamundaaa/colortest) by Xaver Hugl for the Wayland color management implementation.