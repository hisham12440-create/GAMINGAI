#include "superior/capture/windows.h"
#include "superior/core/error.h"
#include <dwmapi.h>
#include <shellapi.h>
#include <algorithm>
namespace superior::capture {
namespace {
BOOL CALLBACK collect(HWND hwnd, LPARAM param) {
    auto* output = reinterpret_cast<std::vector<WindowInfo>*>(param);
    try {
        if (!IsWindowVisible(hwnd)) return TRUE;
        SetLastError(ERROR_SUCCESS);
        const auto style = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
        if (!style && GetLastError() != ERROR_SUCCESS) return TRUE;
        if (style & WS_EX_TOOLWINDOW) return TRUE;
        DWORD process = 0;
        if (!GetWindowThreadProcessId(hwnd, &process) || process == own_process) return TRUE;
        int length = GetWindowTextLengthW(hwnd);
        if (length <= 0 || length > 2048) return TRUE;
        std::wstring title(static_cast<size_t>(length) + 1, L'\0');
        length = GetWindowTextW(hwnd, title.data(), length + 1);
        if (!length) return TRUE;
        title.resize(static_cast<size_t>(length));
        DWORD cloaked = 0;
        if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && cloaked) return TRUE;
        RECT rect{};
        if (!GetWindowRect(hwnd, &rect) || rect.right-rect.left < 160 || rect.bottom-rect.top < 90) return TRUE;
        output->push_back({hwnd, process, std::move(title)});
    } catch (...) { /* A transient window may disappear during enumeration; keep the rest. */ }
    return TRUE;
}
}
std::vector<WindowInfo> enumerate_windows(DWORD own_process) {
    std::vector<WindowInfo> result;
    check_win(EnumWindows(collect, reinterpret_cast<LPARAM>(&result)), "EnumWindows");
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.title < b.title; });
    return result;
}
bool belongs_to(HWND hwnd, DWORD process) noexcept {
    DWORD current = 0;
    return hwnd && IsWindow(hwnd) && GetWindowThreadProcessId(hwnd,&current) && current == process;
}
bool covers_monitor(HWND hwnd) noexcept {
    RECT r{};
    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL);
    MONITORINFO mi{sizeof(mi)};
    return monitor && GetWindowRect(hwnd,&r) && GetMonitorInfoW(monitor,&mi) &&
        r.left <= mi.rcMonitor.left && r.top <= mi.rcMonitor.top &&
        r.right >= mi.rcMonitor.right && r.bottom >= mi.rcMonitor.bottom;
}
bool exclusive_fullscreen_hint() noexcept {
    QUERY_USER_NOTIFICATION_STATE state{};
    return SUCCEEDED(SHQueryUserNotificationState(&state)) && state == QUNS_RUNNING_D3D_FULL_SCREEN;
}
} // namespace superior::capture
