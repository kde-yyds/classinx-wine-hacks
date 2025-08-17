#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <set>

// Structure to hold minimal window information
struct WindowInfo {
    HWND hwnd;
    std::string title;
    bool isFullscreen;
    bool isMinimized;
    bool isSmallSize;  // 160x24 or similar small size
    RECT rect;
    bool isTargetWindowBottom;  // Fullscreen "ClassIn X" or "Classroom*" -> send to bottom
    bool isTargetWindowTop;     // Non-fullscreen "ClassIn X" or "Classroom*" -> send to top
    bool isTargetWindowHide;    // Minimized small "ClassIn X" or "Classroom*" -> hide (every loop)
    bool isTargetWindowRestore; // Was previously hidden by move, but now should be restored
};

// Global vector to store found windows
std::vector<WindowInfo> windows;

// Track windows that we've moved away, so we can restore them
std::set<HWND> movedWindows;

// Function to check if window title matches "ClassIn X" or starts with "Classroom"
bool isExactClassInXOrClassroom(const std::string& title) {
    // Match "ClassIn X" or any title starting with "Classroom"
    return title == "ClassIn X" || title.rfind("Classroom", 0) == 0;
}

// Debug function to convert HWND to string
std::string hwndToString(HWND hwnd) {
    char buffer[32];
    sprintf(buffer, "0x%p", (void*)hwnd);
    return std::string(buffer);
}

// Function to check if window has small minimized size (like 160x24)
bool isSmallMinimizedSize(const RECT& rect) {
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    // Wine typically makes minimized windows around 160x24 or similar small sizes
    return (width <= 200 && height <= 50);
}

// Callback function for EnumWindows
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    // Skip invisible windows immediately
    if (!IsWindowVisible(hwnd)) {
        return TRUE;
    }

    WindowInfo info;
    info.hwnd = hwnd;

    // Get window title
    char title[256];
    GetWindowTextA(hwnd, title, sizeof(title));
    info.title = title;

    // Skip if title doesn't match
    if (!isExactClassInXOrClassroom(info.title)) {
        return TRUE;
    }

    // Get window rectangle
    if (!GetWindowRect(hwnd, &info.rect)) {
        return TRUE;
    }

    // Get window styles
    LONG style = GetWindowLongA(hwnd, GWL_STYLE);
    info.isMinimized = (style & WS_MINIMIZE) != 0;

    // Check if window has small size (160x24 etc.)
    info.isSmallSize = isSmallMinimizedSize(info.rect);

    // Check if window is fullscreen
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    info.isFullscreen = (info.rect.left <= 0 && info.rect.top <= 0 &&
    info.rect.right >= screenWidth &&
    info.rect.bottom >= screenHeight);

    // Set target flags based on window state
    if ((info.isMinimized || info.isSmallSize)) {
        // Minimized or small size -> always hide (move out), every loop
        info.isTargetWindowHide = true;
        info.isTargetWindowBottom = false;
        info.isTargetWindowTop = false;
        info.isTargetWindowRestore = false;
    } else if (!info.isMinimized && !info.isSmallSize && movedWindows.find(hwnd) != movedWindows.end()) {
        // Previously hidden (by moving), now unminimized and moved? Restore
        info.isTargetWindowRestore = true;
        info.isTargetWindowHide = false;
        info.isTargetWindowBottom = false;
        info.isTargetWindowTop = false;
    } else if (info.isFullscreen && !info.isMinimized && !info.isSmallSize) {
        // Fullscreen and not minimized/small -> send to bottom
        info.isTargetWindowBottom = true;
        info.isTargetWindowTop = false;
        info.isTargetWindowHide = false;
        info.isTargetWindowRestore = false;
    } else if (!info.isFullscreen && !info.isMinimized && !info.isSmallSize) {
        // Non-fullscreen, non-minimized, not small -> send to top
        info.isTargetWindowTop = true;
        info.isTargetWindowBottom = false;
        info.isTargetWindowHide = false;
        info.isTargetWindowRestore = false;
    } else {
        // Other state
        info.isTargetWindowTop = false;
        info.isTargetWindowBottom = false;
        info.isTargetWindowHide = false;
        info.isTargetWindowRestore = false;
    }

    // Add all matching windows
    windows.push_back(info);

    return TRUE;
}

// Function to set window to bottom
bool setWindowToBottom(HWND hwnd) {
    // Remove topmost flag if it exists
    LONG exStyle = GetWindowLongA(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOPMOST) {
        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }

    // Set window to bottom
    BOOL result = SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0,
                               SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    return result != 0;
}

// Function to set window to top with TOPMOST flag
bool setWindowToTop(HWND hwnd) {
    // First set to topmost to ensure it stays above everything
    BOOL result1 = SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    // Also try to bring it to foreground (but don't activate to avoid focus stealing)
    BOOL result2 = SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0,
                                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    return result1 || result2;
}

// Function to "hide" window by moving it far out (9999,9999)
bool moveWindowFar(HWND hwnd) {
    // Move window to a far away location (9999,9999)
    BOOL result = SetWindowPos(hwnd, HWND_TOP, 9999, 9999, 200, 50,
                               SWP_NOACTIVATE | SWP_NOZORDER);
    if (result) {
        movedWindows.insert(hwnd);
    }
    return result != 0;
}

// Function to restore window to visible (move to 0,0, size unchanged)
bool restoreWindow(HWND hwnd, const RECT& origRect) {
    int width = origRect.right - origRect.left;
    int height = origRect.bottom - origRect.top;
    BOOL result = SetWindowPos(hwnd, HWND_TOP, 0, 0, width, height,
                               SWP_NOACTIVATE | SWP_NOZORDER);
    if (result) {
        movedWindows.erase(hwnd);
    }
    return result != 0;
}

// Function to print detailed window info for debugging
void printWindowDebugInfo(const WindowInfo& win, const std::string& action) {
    int width = win.rect.right - win.rect.left;
    int height = win.rect.bottom - win.rect.top;

    std::cout << std::endl;
    std::cout << "  DEBUG - " << action << ":" << std::endl;
    std::cout << "    HWND: " << hwndToString(win.hwnd) << std::endl;
    std::cout << "    Title: \"" << win.title << "\"" << std::endl;
    std::cout << "    Position: (" << win.rect.left << ", " << win.rect.top << ")" << std::endl;
    std::cout << "    Size: " << width << "x" << height << std::endl;
    std::cout << "    Minimized: " << (win.isMinimized ? "YES" : "NO") << std::endl;
    std::cout << "    SmallSize: " << (win.isSmallSize ? "YES" : "NO") << std::endl;
    std::cout << "    Fullscreen: " << (win.isFullscreen ? "YES" : "NO") << std::endl;
    std::cout << "    Marked as moved: " << (movedWindows.find(win.hwnd) != movedWindows.end() ? "YES" : "NO") << std::endl;
}

int main() {
    std::cout << "ClassIn X/Room Wine Hacks" << std::endl;
    std::cout << "==========================================================" << std::endl;
    std::cout << "Fullscreen 'ClassIn X'/'Classroom*' -> Bottom" << std::endl;
    std::cout << "Non-fullscreen 'ClassIn X'/'Classroom*' -> TOPMOST" << std::endl;
    std::cout << "Minimized/Small 'ClassIn X'/'Classroom*' -> MOVE FAR (9999,9999) always" << std::endl;
    std::cout << "Restore unmoved when not minimized/small" << std::endl;
    std::cout << "Monitoring every 0.5 seconds..." << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << "==========================================================" << std::endl;

    // Check screen resolution
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    std::cout << "Screen: " << screenWidth << "x" << screenHeight << std::endl;
    std::cout << "Starting monitoring..." << std::endl;

    int cycles = 0;
    int totalProcessedBottom = 0;
    int totalProcessedTop = 0;
    int totalProcessedMoved = 0;
    int totalProcessedRestored = 0;

    while (true) {
        cycles++;
        windows.clear();

        // Scan for currently visible windows
        EnumWindows(EnumWindowsProc, 0);

        // Process currently visible windows
        if (!windows.empty()) {
            int processedBottom = 0;
            int processedTop = 0;
            int processedMoved = 0;
            int processedRestored = 0;
            int ignored = 0;

            std::cout << "[" << cycles << "] Found " << windows.size() << " 'ClassIn X'/'Classroom*' window(s): ";

            for (const auto& win : windows) {
                if (win.isTargetWindowHide) {
                    // Minimized/small -> move far (every time)
                    if (moveWindowFar(win.hwnd)) {
                        processedMoved++;
                        totalProcessedMoved++;
                    }
                    std::cout << "\"" << win.title << "\"(MovedFar) ";
                    printWindowDebugInfo(win, "MOVING FAR: Minimized/small");
                } else if (win.isTargetWindowRestore) {
                    // Previously moved out, but now normal -> restore to (0,0)
                    if (restoreWindow(win.hwnd, win.rect)) {
                        processedRestored++;
                        totalProcessedRestored++;
                    }
                    std::cout << "\"" << win.title << "\"(Restored) ";
                    printWindowDebugInfo(win, "RESTORING: Unminimized/normal");
                } else if (win.isTargetWindowBottom) {
                    // Fullscreen -> send to bottom
                    if (setWindowToBottom(win.hwnd)) {
                        processedBottom++;
                        totalProcessedBottom++;
                    }
                    std::cout << "\"" << win.title << "\"(FS->Bot) ";
                } else if (win.isTargetWindowTop) {
                    // Non-fullscreen -> send to topmost
                    if (setWindowToTop(win.hwnd)) {
                        processedTop++;
                        totalProcessedTop++;
                    }
                    std::cout << "\"" << win.title << "\"(Win->TopMost) ";
                } else {
                    // Ignored (other state)
                    ignored++;
                    std::cout << "\"" << win.title << "\"(OtherState) ";
                    printWindowDebugInfo(win, "IGNORING: Other state");
                }
            }

            if (processedBottom > 0 || processedTop > 0 || processedMoved > 0 || processedRestored > 0) {
                std::cout << "-> " << processedBottom << " to bottom, "
                << processedTop << " to topmost, "
                << processedMoved << " moved far, "
                << processedRestored << " restored";
                if (ignored > 0) {
                    std::cout << ", " << ignored << " ignored";
                }
                std::cout << std::endl;
            } else {
                std::cout << "-> failed to process" << std::endl;
            }
        } else if (cycles % 20 == 0) {
            // Print status every 10 seconds (20 cycles * 0.5s) to show it's still running
            std::cout << "[" << cycles << "] Monitoring... (Total: "
            << totalProcessedBottom << " to bottom, "
            << totalProcessedTop << " to topmost, "
            << totalProcessedMoved << " moved far, "
            << totalProcessedRestored << " restored, "
            << movedWindows.size() << " in moved set)" << std::endl;
        }

        Sleep(500); // Wait 0.5 seconds
    }

    return 0;
}
