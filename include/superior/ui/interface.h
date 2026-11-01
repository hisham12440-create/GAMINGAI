#pragma once
#include "superior/app/window.h"
#include "superior/present/device.h"
#include "superior/core/config.h"
#include "superior/capture/windows.h"
#include "superior/hardware/adapter.h"
#include "superior/ui/text.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>
namespace superior {
struct UiModel {
    Settings settings;
    std::vector<capture::WindowInfo> windows;
    hardware::AdapterInfo adapter;
    std::string status, error, notice;
    bool capturing=false, paused=false, capture_supported=false;
    double capture_fps=0;
    int capture_width=0,capture_height=0;
};
struct UiActions {
    bool refresh=false,start=false,stop=false,exit=false,save_hotkeys=false,toggle_capture=false;
    int selected=-1;
    Settings settings;
};
class Interface {
public:
    Interface(present::Device& device,const std::filesystem::path& font);
    ~Interface();
    Interface(const Interface&) = delete;
    Interface& operator=(const Interface&) = delete;
    HWND handle() const noexcept;
    void set_window_handler(Window::Handler handler, void* context) noexcept;
    int selected_window() const noexcept { return selected_; }
    bool visible() const noexcept;
    void show() noexcept;
    void hide() noexcept;
    UiActions render(const UiModel& model);
    static LRESULT CALLBACK window_proc(HWND,UINT,WPARAM,LPARAM,void*) noexcept;
private:
    void create_target();
    void release_target() noexcept;
    void rebuild_font(float dpi);
    void draw_button(const char* id,const std::string& label,float width,bool enabled,std::function<void()> action);
    void draw_checkbox(const char* id,const std::string& label,bool& value);
    present::Device& device_;
    std::unique_ptr<Window> window_;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> swap_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv_;
    ImGuiContext* imgui_=nullptr;
    std::unique_ptr<TextEngine> text_;
    std::filesystem::path font_;
    int selected_=0,width_=900,height_=740;
    unsigned dpi_=96,pending_dpi_=0;
    bool initialized_=false,fields_initialized_=false;
    char show_ui_[81]{},toggle_overlay_[81]{},click_through_[81]{},toggle_capture_[81]{};
};
} // namespace superior
