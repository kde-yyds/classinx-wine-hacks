#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>

// Structure to hold minimal window information
struct WindowInfo {
    HWND hwnd;
    std::string title;
    bool isFullscreen;
    bool isMinimized;
    bool isTargetWindowBottom;  // Fullscreen "ClassIn X" -> send to bottom
    bool isTargetWindowTop;     // Non-fullscreen "ClassIn X" -> send to top
    bool isTargetWindowHide;    // Minimized "ClassIn X" -> hide completely
};

// Global vector to store found windows
std::vector<WindowInfo> windows;
// Track all ClassIn X windows we've seen (including hidden ones)
std::map<HWND, bool> trackedWindows; // HWND -> isCurrentlyHidden

// Function to check if window title matches exactly "ClassIn X"
bool isExactClassInX(const std::string& title) {
    return title == "ClassIn X";
}

// Debug function to convert HWND to string
std::string hwndToString(HWND hwnd) {
    char buffer[32];
    sprintf(buffer, "0x%p", (void*)hwnd);
    return std::string(buffer);
}

// Function to check if window is in minimized position (bottom-left corner)
bool isInMinimizedPosition(const RECT& rect) {
    // Wine typically places minimized windows at very small coordinates
    // Usually around (0,0) to (160,30) or similar small rectangle in bottom-left
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    return (rect.left <= 10 && rect.top <= 10 && width <= 200 && height <= 50) ||
    (rect.left < 0 && rect.top < 0) ||  // Sometimes negative coordinates
    (width <= 160 && height <= 30 && rect.left <= 200 && rect.top <= 200);
}

// Function to check if a window still exists and get its state
bool checkWindowState(HWND hwnd, bool& isVisible, bool& isMinimized, bool& isInMinPos) {
    if (!IsWindow(hwnd)) {
        return false; // Window no longer exists
    }

    isVisible = IsWindowVisible(hwnd);

    LONG style = GetWindowLongA(hwnd, GWL_STYLE);
    isMinimized = (style & WS_MINIMIZE) != 0;

    RECT rect;
    isInMinPos = false;
    if (GetWindowRect(hwnd, &rect)) {
        isInMinPos = isInMinimizedPosition(rect);
    }

    return true;
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
    if (!isExactClassInX(info.title)) {
        return TRUE;
    }

    // Add to tracked windows if not already there
    if (trackedWindows.find(hwnd) == trackedWindows.end()) {
        trackedWindows[hwnd] = false; // Not hidden initially
    }

    // Get window rectangle
    RECT rect;
    if (!GetWindowRect(hwnd, &rect)) {
        return TRUE;
    }

    // Get window styles
    LONG style = GetWindowLongA(hwnd, GWL_STYLE);
    info.isMinimized = (style & WS_MINIMIZE) != 0;

    // Check if window is in minimized position (Wine-specific issue)
    bool inMinimizedPosition = isInMinimizedPosition(rect);

    // Check if window is fullscreen
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    info.isFullscreen = (rect.left <= 0 && rect.top <= 0 &&
    rect.right >= screenWidth &&
    rect.bottom >= screenHeight);

    // Set target flags based on window state
    if (info.isMinimized || inMinimizedPosition) {
        // Minimized or in minimized position -> hide completely
        info.isTargetWindowHide = true;
        info.isTargetWindowBottom = false;
        info.isTargetWindowTop = false;
    } else if (info.isFullscreen) {
        // Fullscreen -> send to bottom
        info.isTargetWindowBottom = true;
        info.isTargetWindowTop = false;
        info.isTargetWindowHide = false;
    } else {
        // Non-fullscreen, non-minimized -> send to top
        info.isTargetWindowTop = true;
        info.isTargetWindowBottom = false;
        info.isTargetWindowHide = false;
    }

    // Add all "ClassIn X" windows
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

// Function to hide window completely
bool hideWindow(HWND hwnd) {
    // Hide the window completely
    BOOL result = ShowWindow(hwnd, SW_HIDE);
    return result != 0;
}

// Function to show window again
bool showWindow(HWND hwnd) {
    // Show the window again
    BOOL result = ShowWindow(hwnd, SW_SHOW);
    return result != 0;
}

int main() {
    std::cout << "ClassIn X Window Manager - Wine Hacks Edition" << std::endl;
    std::cout << "=============================================" << std::endl;
    std::cout << "Fullscreen 'ClassIn X' -> Bottom" << std::endl;
    std::cout << "Non-fullscreen 'ClassIn X' -> TOPMOST" << std::endl;
    std::cout << "Minimized 'ClassIn X' -> HIDDEN" << std::endl;
    std::cout << "Monitoring every 0.5 seconds..." << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << "=============================================" << std::endl;

    // Check screen resolution
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    std::cout << "Screen: " << screenWidth << "x" << screenHeight << std::endl;
    std::cout << "Starting monitoring..." << std::endl;

    int cycles = 0;
    int totalProcessedBottom = 0;
    int totalProcessedTop = 0;
    int totalProcessedHidden = 0;
    int totalProcessedShown = 0;

    while (true) {
        cycles++;
        windows.clear();

        // First, scan for currently visible windows
        EnumWindows(EnumWindowsProc, 0);

        // Then, check all tracked windows (including hidden ones)
        auto it = trackedWindows.begin();
        while (it != trackedWindows.end()) {
            HWND hwnd = it->first;
            bool wasHidden = it->second;

            bool isVisible, isMinimized, isInMinPos;
            if (!checkWindowState(hwnd, isVisible, isMinimized, isInMinPos)) {
                // Window no longer exists, remove from tracking
                it = trackedWindows.erase(it);
                continue;
            }

            // Check if hidden window should be shown again
            if (wasHidden && isVisible && !isMinimized && !isInMinPos) {
                // Window was hidden but is now visible and not minimized
                // Don't need to do anything, it will be handled by EnumWindows
                trackedWindows[hwnd] = false; // Mark as not hidden
            }
            // Check if visible window should be hidden
            else if (!wasHidden && isVisible && (isMinimized || isInMinPos)) {
                // Window is visible but should be hidden
                if (hideWindow(hwnd)) {
                    trackedWindows[hwnd] = true; // Mark as hidden
                    totalProcessedHidden++;
                    std::cout << "[" << cycles << "] Hidden tracked window: "
                    << hwndToString(hwnd) << std::endl;
                }
            }

            ++it;
        }

        // Process currently visible windows
        if (!windows.empty()) {
            int processedBottom = 0;
            int processedTop = 0;
            int processedHidden = 0;

            std::cout << "[" << cycles << "] Found " << windows.size() << " 'ClassIn X' window(s): ";

            for (const auto& win : windows) {
                if (win.isTargetWindowHide) {
                    // Minimized -> hide completely
                    if (hideWindow(win.hwnd)) {
                        processedHidden++;
                        totalProcessedHidden++;
                        trackedWindows[win.hwnd] = true; // Mark as hidden
                    }
                    std::cout << "\"" << win.title << "\"(Min->Hide) ";
                } else if (win.isTargetWindowBottom) {
                    // Fullscreen -> send to bottom
                    if (setWindowToBottom(win.hwnd)) {
                        processedBottom++;
                        totalProcessedBottom++;
                    }
                    trackedWindows[win.hwnd] = false; // Mark as not hidden
                    std::cout << "\"" << win.title << "\"(FS->Bot) ";
                } else if (win.isTargetWindowTop) {
                    // Non-fullscreen -> send to topmost
                    if (setWindowToTop(win.hwnd)) {
                        processedTop++;
                        totalProcessedTop++;
                    }
                    trackedWindows[win.hwnd] = false; // Mark as not hidden
                    std::cout << "\"" << win.title << "\"(Win->TopMost) ";
                }
            }

            if (processedBottom > 0 || processedTop > 0 || processedHidden > 0) {
                std::cout << "-> " << processedBottom << " to bottom, "
                << processedTop << " to topmost, "
                << processedHidden << " hidden" << std::endl;
            } else {
                std::cout << "-> failed to process" << std::endl;
            }
        } else if (cycles % 20 == 0) {
            // Print status every 10 seconds (20 cycles * 0.5s) to show it's still running
            std::cout << "[" << cycles << "] Monitoring... (Tracked: " << trackedWindows.size()
            << ", Total: " << totalProcessedBottom << " to bottom, "
            << totalProcessedTop << " to topmost, "
            << totalProcessedHidden << " hidden)" << std::endl;
        }

        Sleep(500); // Wait 0.5 seconds
    }

    return 0;
}
