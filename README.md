# ClassInX Wine Hacks

A collection of utilities to make ClassIn X annotation tool work seamlessly on Linux with Wine. Currently includes an automatic window manager for Wine (Windows side, cross-compiled) and a shell script for X11/Linux side to ensure reliable window order.

## Problem Solved

ClassIn X on Wine suffers from window layering issues where the fullscreen annotation canvas covers the toolbox, making annotation controls inaccessible.  
To solve this, **both sides must be managed**:
- **Windows-side (Wine internal):** Run `window_manager.exe` inside Wine to rearrange ClassIn X windows from within the Wine environment.
- **Linux-side (X11):** Run `window_manager.sh` on your Linux desktop to reinforce Z-order, stacking, and focus, and to handle touch/mouse quirks that can't be solved from the Windows side alone.

This hybrid approach ensures:
- Annotation canvas stays in background
- Toolbox remains accessible on top
- Seamless annotation workflow and touch input

## Features

- **Automatic Window Management (Windows-side):** Continuously manages ClassIn X window layers inside Wine.
- **Hybrid Layer Control (Both sides):**
  - Windows-side: Sends fullscreen annotation windows to the bottom layer (canvas), keeps toolbox always on top inside Wine.
  - Linux-side: Reinforces toolbox stacking/focus and works around X11 quirks (e.g., touch input, stacking fights).
- **Wine Optimized:** Cross-compiled for Windows, performance-tuned for Wine.
- **Zero Configuration:** Starts monitoring immediately, no setup required.
- **Lightweight:** Minimal resource usage with efficient scanning.

## Prerequisites

### For Compilation (Linux)
- MinGW cross-compiler for Windows
- Ubuntu/Debian: `sudo apt-get install mingw-w64`
- CentOS/RHEL/Fedora: `sudo dnf install mingw64-gcc-c++`
- Arch Linux: `sudo pacman -S mingw-w64-gcc`

### For Running
- Wine installed on Linux
- ClassIn X application
- `wmctrl`, `xdotool`, and `xprop` (for `window_manager.sh` on Linux)

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

3. **Disable "Allow the window manager to control the windows" in winecfg** to make Wine windows always-on-top and override-redirect.

4. **Start the Wine-side window manager hack**:
   ```bash
   wine window_manager.exe
   ```

5. **Start the Linux/X11-side window manager script** (in another terminal, on your Linux desktop):
   ```bash
   bash window_manager.sh
   ```

6. **Launch ClassIn X** and enjoy seamless annotation!

## How It Works

1. **Detection**: Both hacks scan for windows matching "ClassIn X" or "Classroom*".
2. **Classification**: Determine if window is fullscreen (canvas) or windowed (toolbox).
3. **Layer Management**:
   - Fullscreen → `HWND_BOTTOM` (inside Wine) and background (on Linux)
   - Toolbox → `HWND_TOPMOST` (inside Wine) and raised/focused (on Linux)
   - Minimized → moved far offscreen (inside Wine) and/or hidden (on Linux)
4. **Continuous**: Monitors every 500ms for new windows or state changes on both sides.

## Project Structure

```
classinx-wine-hacks/
├── window_manager.cpp    # Main Wine-side window management hack (cross-compiled for Windows)
├── window_manager.exe    # Compiled Wine-side hack (run with Wine)
├── window_manager.sh     # Linux/X11-side stacking/focus/enhancement script
├── README.md             # This README
└── LICENSE               # Project license
```

## Troubleshooting

### Wine Configuration
- Use recent Wine version to avoid missing DLLs needed by ClassIn X.
- Make sure to disable "Allow the window manager to control the windows" in winecfg.
- Run **both** hacks simultaneously: `window_manager.exe` (in Wine) and `window_manager.sh` (on Linux).
- Ensure you have X11 tools installed: `wmctrl`, `xdotool`, and `xprop`.

## Compatibility
- **ClassIn X**: Older versions with old UI are recommended because new ones have issues with toolbox popups.

## License

This hack is licensed under GPL v3.  
ClassIn X is proprietary software. Use it at your own risk.
