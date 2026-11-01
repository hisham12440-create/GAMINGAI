#pragma once
#include <windows.h>
#include <string>
#include <vector>
namespace superior::capture {
struct WindowInfo { HWND handle = nullptr; DWORD process = 0; std::wstring title; };
std::vector<WindowInfo> enumerate_windows(DWORD own_process);
bool belongs_to(HWND window, DWORD process) noexcept;
bool covers_monitor(HWND window) noexcept;
bool exclusive_fullscreen_hint() noexcept;
} // namespace superior::capture
