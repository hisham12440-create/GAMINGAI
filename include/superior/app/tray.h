#pragma once
#include <windows.h>
namespace superior {
class Tray {
public:
    static constexpr UINT message = WM_APP + 1;
    Tray(HWND owner);
    ~Tray();
    Tray(const Tray&) = delete;
    Tray& operator=(const Tray&) = delete;
    void show_menu(bool arabic) noexcept;
private:
    HWND owner_;
    NOTIFYICONDATAW data_{};
    bool added_ = false;
};
} // namespace superior
