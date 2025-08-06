# HDR Image Viewer

An image viewer for displaying High Dynamic Range (HDR) images on Linux systems.

## Description

HDR Image Viewer is an application for viewing HDR images with accurate color reproduction on Linux. It supports PNG and AVIF images in BT.2020 PQ/ST.2084 color encoding and integrates with the Wayland color-management-v1 protocol for proper HDR display. It has been developed and tested under KDE Plasma 6 in Wayland mode with HDR enabled on a HDR1000 certified display.

## Features

- Display of HDR images via the Wayland color-management-v1 protocol
- 100x zoom with cursor-centered scaling, WASD movement and persistent zoom/pan state for seamless image comparison
- High performance with animations developed for maximum smoothness on high refresh rate displays up to 240 Hertz
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

Launch the application with an image file:

```bash
./build/bin/hdr-image-viewer path/to/image.png
./build/bin/hdr-image-viewer path/to/image.avif
```

### Controls

#### Navigation
- **Space**, **Right Arrow** or **Page Down**: Navigate to next image
- **Shift**, **Left Arrow** or **Page Up**: Navigate to previous image

#### Zoom & View
- **Mouse wheel**: Zoom in/out at cursor position
- **E**, **+**, or **=**: Zoom in (continuous when held)
- **Q**, **-** or **_**: Zoom out (continuous when held)
- **0** or **Home**: Reset zoom to fit window

#### Image Movement (when zoomed in)
- **W**: Move image up (continuous when held)
- **A**: Move image left (continuous when held)  
- **S**: Move image down (continuous when held)
- **D**: Move image right (continuous when held)
- **Click + drag**: Move image with mouse

#### Window Control
- **F** or **F11**: Toggle fullscreen mode
- **Escape**: Exit fullscreen mode
- **Ctrl+Q**: Quit application
- **Double-click**: Toggle fullscreen mode
- **Click + drag** (when not zoomed): Move window

## License

MIT License - see [LICENSE.md](LICENSE.md)

## Acknowledgments

Based on [colortest](https://invent.kde.org/zamundaaa/colortest) by Xaver Hugl for the Wayland color management implementation.