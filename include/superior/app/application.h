#pragma once
#include "superior/core/config.h"
#include "superior/core/logging.h"
#include "superior/core/timing.h"
#include "superior/present/device.h"
#include "superior/present/presenter.h"
#include "superior/capture/capture.h"
#include "superior/ui/interface.h"
#include "superior/app/tray.h"
#include <memory>
namespace superior {
class Application {
public:
    Application();
    ~Application();
    int run();
private:
    static LRESULT CALLBACK window_proc(HWND,UINT,WPARAM,LPARAM,void*) noexcept;
    void refresh_windows();
    void start_capture();
    void stop_capture() noexcept;
    void apply_actions(const UiActions& actions);
    bool register_hotkeys(const Hotkey& keys,std::string& error) noexcept;
    void unregister_hotkeys() noexcept;
    void save() noexcept;
    void toggle_ui() noexcept;
    void poll_target();
    void fail(const std::exception& error) noexcept;
    std::filesystem::path data_,settings_path_;
    Logger logger_;
    Settings settings_;
    std::string notice_,status_,error_;
    present::Device device_;
    capture::Capture capture_;
    present::Presenter presenter_;
    Interface ui_;
    std::unique_ptr<Tray> tray_;
    std::vector<capture::WindowInfo> windows_;
    Clock clock_;
    IntervalMeter capture_intervals_;
    std::int64_t last_ui_=0,last_poll_=0,last_capture_frame_=0;
    HWND desired_target_=nullptr;
    DWORD desired_process_=0;
    bool want_capture_=false,paused_=false,quitting_=false,fatal_=false,hotkeys_registered_=false;
    bool capture_supported_;
};
} // namespace superior
