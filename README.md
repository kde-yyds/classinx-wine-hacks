# ClassInX Wine Hacks

A collection of utilities to make ClassIn X annotation tool work seamlessly on Linux with Wine. Currently includes an automatic window manager to workaround window order issues.

## Problem Solved

ClassIn X on Wine suffers from window layering issues where the fullscreen annotation canvas covers the toolbox, making annotation controls inaccessible.  
To solve this, disable "Allow the window manager to control the windows" in winecfg to make all wine windows on top. Then run this program to solve window order issues inside wine.  
This hack automatically manages window Z-order to ensure:
- Annotation canvas stays in background
- Toolbox remains accessible on top
- Seamless annotation workflow

## Features

- **Automatic Window Management**: Continuously manages ClassIn X window layers
- **Dual Layer Control**:
  - Sends fullscreen annotation windows to the bottom layer (canvas)
  - Keeps toolbox windows always on top (controls)
- **Wine Optimized**: Cross-compiled for Windows, performance-tuned for Wine
- **Zero Configuration**: Starts monitoring immediately, no setup required
- **Lightweight**: Minimal resource usage with efficient scanning

## Prerequisites

### For Compilation (Linux)
- MinGW cross-compiler for Windows
- Ubuntu/Debian: `sudo apt-get install mingw-w64`
- CentOS/RHEL/Fedora: `sudo dnf install mingw64-gcc-c++`
- Arch Linux: `sudo pacman -S mingw-w64-gcc`

### For Running
- Wine installed on Linux
- ClassIn X application

## Usage

1. **Clone and compile**:
   ```bash
   git clone https://github.com/kde-yyds/classinx-wine-hacks.git
   cd classinx-wine-hacks
   x86_64-w64-mingw32-g++ -std=c++11 -Wall -Wextra -O2 -g -static -o window_manager.exe window_manager.cpp -luser32 -lgdi32 -lkernel32
   ```
2. **Create a new wineprefix**
   ```bash
   export WINEPREFIX=/path/to/new/wineprefix
   wine winecfg
   ```
3. **Disable "Allow the window manager to control the windows" in winecfg** to make wine windows on the top.

4. **Start the window manager hack**:
   ```bash
   wine window_manager.exe
   ```

5. **Launch ClassIn X** and enjoy annotation!


## How It Works

1. **Detection**: Scans for windows with exact title "ClassIn X"
2. **Classification**: Determines if window is fullscreen (canvas) or windowed (toolbox)
3. **Layer Management**:
   - Fullscreen → `HWND_BOTTOM` (background)
   - Windowed → `HWND_TOPMOST` (always on top)
4. **Continuous**: Monitors every 500ms for new windows or state changes

## Project Structure

```
classinx-wine-hacks/
├── window_manager.cpp    # Main window management hack
├── README.md            # README
└── LICENSE              # Project license
```

## Troubleshooting

### Wine Configuration
- Use recent Wine version to avoid missing dll needed by Classin X.
- Make sure to disable "Allow the window manager to control the windows" in winecfg.

## Compatibility
- **ClassIn X**: Older versions with old UI are recommended because new ones have issue to write around the toolbox popup.


## License

This hack is licensed under GPL v3.  
Classin X is proprietary software. Use it at your own risk.

