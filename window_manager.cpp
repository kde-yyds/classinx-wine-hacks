#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

// Structure to hold minimal window information
struct WindowInfo {
    HWND hwnd;
    std::string title;
    bool isFullscreen;
    bool isMinimized;
    bool isTargetWindowBottom;  // Fullscreen "ClassIn X" -> send to bottom
    bool isTargetWindowTop;     // Non-fullscreen "ClassIn X" -> send to top
};

// Global vector to store found windows
std::vector<WindowInfo> windows;

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

// Callback function for EnumWindows (minimal version)
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

    // Get window rectangle
    RECT rect;
    if (!GetWindowRect(hwnd, &rect)) {
        return TRUE;
    }

    // Get window styles
    LONG style = GetWindowLongA(hwnd, GWL_STYLE);
    info.isMinimized = (style & WS_MINIMIZE) != 0;

    // Skip if minimized
    if (info.isMinimized) {
        return TRUE;
    }

    // Check if window is fullscreen
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    info.isFullscreen = (rect.left <= 0 && rect.top <= 0 &&
    rect.right >= screenWidth &&
    rect.bottom >= screenHeight);

    // Set target flags based on fullscreen status
    info.isTargetWindowBottom = info.isFullscreen;      // Fullscreen -> bottom
    info.isTargetWindowTop = !info.isFullscreen;        // Non-fullscreen -> top

    // Add all "ClassIn X" windows (both fullscreen and non-fullscreen)
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

int main() {
    std::cout << "ClassIn X Window Manager - Aggressive Mode" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "Fullscreen 'ClassIn X' -> Bottom" << std::endl;
    std::cout << "Non-fullscreen 'ClassIn X' -> TOPMOST" << std::endl;
    std::cout << "Monitoring every 0.5 seconds..." << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << "==========================================" << std::endl;

    // Check screen resolution
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    std::cout << "Screen: " << screenWidth << "x" << screenHeight << std::endl;
    std::cout << "Starting monitoring..." << std::endl;

    int cycles = 0;
    int totalProcessedBottom = 0;
    int totalProcessedTop = 0;

    while (true) {
        cycles++;
        windows.clear();

        // Scan for target windows
        EnumWindows(EnumWindowsProc, 0);

        if (!windows.empty()) {
            int processedBottom = 0;
            int processedTop = 0;

            std::cout << "[" << cycles << "] Found " << windows.size() << " 'ClassIn X' window(s): ";

            for (const auto& win : windows) {
                if (win.isTargetWindowBottom) {
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
                }
            }

            if (processedBottom > 0 || processedTop > 0) {
                std::cout << "-> " << processedBottom << " to bottom, " << processedTop << " to topmost" << std::endl;
            } else {
                std::cout << "-> failed to process" << std::endl;
            }
        } else if (cycles % 20 == 0) {
            // Print status every 10 seconds (20 cycles * 0.5s) to show it's still running
            std::cout << "[" << cycles << "] Monitoring... (Total: "
            << totalProcessedBottom << " to bottom, "
            << totalProcessedTop << " to topmost)" << std::endl;
        }

        Sleep(500); // Wait 0.5 seconds
    }

    return 0;
}
