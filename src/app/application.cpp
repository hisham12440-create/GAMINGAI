#include "superior/app/application.h"
#include "superior/core/error.h"
#include "superior/core/hotkey.h"
#include "superior/core/paths.h"
#include "superior/core/strings.h"
#include <shellapi.h>
#include <thread>
namespace superior {
namespace {
constexpr WPARAM key_show=1,key_overlay=2,key_click=3,key_capture=4;
}
Application::Application()
    : data_(local_data_directory()),settings_path_(data_/L"settings.json"),logger_(data_/L"logs"),
      capture_(*device_.get()),presenter_(device_,executable_directory()/L"shaders"),
      ui_(device_,[] {
          wchar_t windows[MAX_PATH]{};
          if(!GetWindowsDirectoryW(windows,MAX_PATH))check_win(false,"GetWindowsDirectoryW");
          for(const auto* name:{L"tahoma.ttf",L"arial.ttf"}) {
              const auto path=std::filesystem::path(windows)/L"Fonts"/name;
              if(std::filesystem::exists(path))return path;
          }
          throw Error("No Arabic-capable system font found (Tahoma or Arial)");
      }()),capture_intervals_(clock_.frequency()) {
    ui_.set_window_handler(window_proc,this);
    logger_.install_crash_handler();
    auto loaded=load_settings(settings_path_);settings_=loaded.settings;notice_=loaded.notice;
    capture_supported_=capture::Capture::supported();
    status_=capture_supported_?"Ready. Select a game window and start capture.":
        "Capture unavailable: requires Windows 10 1903+ / Windows 11 and a WGC-capable system.";
    tray_=std::make_unique<Tray>(ui_.handle());refresh_windows();
    std::string hotkey_error;
    if(!register_hotkeys(settings_.hotkeys,hotkey_error))notice_+="; "+hotkey_error;
    if(settings_.show_ui_on_start)ui_.show();else ui_.hide();
    try{save();}catch(const std::exception& e){fail(e);}
    logger_.write("INFO","Superior 0.1.0 started; capture supported="+std::to_string(capture_supported_));
}
Application::~Application(){unregister_hotkeys();presenter_.hide();capture_.stop();}
void Application::refresh_windows(){
    windows_=capture::enumerate_windows(GetCurrentProcessId());
    if(desired_target_&&!capture::belongs_to(desired_target_,desired_process_))stop_capture();
}
void Application::save()noexcept{
    try{save_settings(settings_path_,settings_);}
    catch(const std::exception& e){error_=std::string("Settings not saved: ")+e.what();logger_.write("ERROR",error_);}
}
void Application::unregister_hotkeys()noexcept{
    for(int id=1;id<=4;++id)if(!UnregisterHotKey(ui_.handle(),id))logger_.write("WARN","UnregisterHotKey failed");
    hotkeys_registered_=false;
}
bool Application::register_hotkeys(const Hotkey& keys,std::string& error)noexcept{
    unregister_hotkeys();
    const std::pair<int,std::string> entries[]={{key_show,keys.show_ui},{key_overlay,keys.toggle_overlay},
        {key_click,keys.click_through},{key_capture,keys.toggle_capture}};
    for(const auto& [id,chord]:entries){
        try{
            const auto parsed=parse_hotkey(chord);
            if(!RegisterHotKey(ui_.handle(),id,parsed.modifiers|MOD_NOREPEAT,parsed.key))check_win(false,"RegisterHotKey");
        }catch(const std::exception& e){error="Hotkey conflict/invalid input: "+chord+" ("+e.what()+")";unregister_hotkeys();return false;}
    }
    hotkeys_registered_=true;error.clear();return true;
}
void Application::start_capture(){
    if(!capture_supported_)throw Error("Windows.Graphics.Capture is not available on this system");
    const int selected=ui_.selected_window();
    if(selected<0||selected>=static_cast<int>(windows_.size()))throw Error("Select a game window first");
    desired_target_=windows_[static_cast<size_t>(selected)].handle;desired_process_=windows_[static_cast<size_t>(selected)].process;
    capture_.start(desired_target_);want_capture_=true;paused_=false;error_.clear();
    capture_intervals_.reset();last_capture_frame_=clock_.now();status_="Waiting for the first captured frame.";
    SetForegroundWindow(desired_target_);logger_.write("INFO","Capture started");
}
void Application::stop_capture()noexcept{
    want_capture_=false;paused_=false;capture_.stop();presenter_.hide();desired_target_=nullptr;desired_process_=0;
    capture_intervals_.reset();status_="Capture stopped.";
}
void Application::toggle_ui()noexcept{if(ui_.visible())ui_.hide();else ui_.show();}
void Application::apply_actions(const UiActions& a){
    if(a.refresh)refresh_windows();
    if(a.stop)stop_capture();
    if(a.start)start_capture();
    if(a.settings.language!=settings_.language||a.settings.overlay_enabled!=settings_.overlay_enabled||a.settings.click_through!=settings_.click_through){
        settings_.language=a.settings.language;settings_.overlay_enabled=a.settings.overlay_enabled;settings_.click_through=a.settings.click_through;save();
    }
    if(!settings_.overlay_enabled)presenter_.hide();
    if(a.save_hotkeys){
        auto candidate=settings_;candidate.hotkeys=a.settings.hotkeys;
        const auto error=candidate.validate();
        if(!error.empty())throw Error(error);
        std::string registration_error;
        if(register_hotkeys(a.settings.hotkeys,registration_error)){settings_.hotkeys=a.settings.hotkeys;save();notice_="Hotkeys registered and settings saved.";}
        else{
            error_=registration_error;
            std::string restore_error;
            const bool restored=register_hotkeys(settings_.hotkeys,restore_error);
            notice_=restored?"Previous hotkeys restored; change the conflicting chord.":
                "Hotkeys unavailable due to a conflict. Use the control panel or tray; change the conflicting chord.";
            if(!restored)logger_.write("WARN",restore_error);
        }
    }
    if(a.exit)quitting_=true;
}
void Application::poll_target(){
    if(!want_capture_)return;
    if(!capture::belongs_to(desired_target_,desired_process_)){stop_capture();status_="Game window closed. Capture stopped safely.";return;}
    const bool minimize=IsIconic(desired_target_)!=FALSE;
    if(minimize&&!paused_){capture_.stop();presenter_.hide();paused_=true;capture_intervals_.reset();status_="Paused: game minimized.";return;}
    if(!minimize&&paused_){capture_.start(desired_target_);paused_=false;last_capture_frame_=clock_.now();status_="Capture resumed.";return;}
    if(GetForegroundWindow()!=desired_target_)presenter_.hide();
    if(capture::exclusive_fullscreen_hint()){
        presenter_.hide();error_="Exclusive fullscreen detected. Switch the game to Borderless Windowed / Windowed, then restart capture.";
    }
}
void Application::fail(const std::exception& e)noexcept{
    error_=e.what();logger_.write("ERROR",error_);
    capture_.stop();presenter_.hide();want_capture_=false;paused_=false;
    status_="Operation stopped safely. See the error in the control panel.";
}
LRESULT CALLBACK Application::window_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp,void* context)noexcept{
    if(msg==WM_NCCREATE){auto* cs=reinterpret_cast<CREATESTRUCTW*>(lp);context=cs->lpCreateParams;}
    auto* self=static_cast<Application*>(context);if(!self)return DefWindowProcW(hwnd,msg,wp,lp);
    try{
        if(msg==WM_HOTKEY){
            if(wp==key_show)self->toggle_ui();
            if(wp==key_overlay){self->settings_.overlay_enabled=!self->settings_.overlay_enabled;if(!self->settings_.overlay_enabled)self->presenter_.hide();self->save();}
            if(wp==key_click){self->settings_.click_through=!self->settings_.click_through;self->save();}
            if(wp==key_capture){if(self->want_capture_)self->stop_capture();else self->start_capture();}
            return 0;
        }
        if(msg==Tray::message){
            if(LOWORD(lp)==WM_LBUTTONUP)self->toggle_ui();
            if(LOWORD(lp)==WM_RBUTTONUP)self->tray_->show_menu(self->settings_.language==Language::Arabic);
            return 0;
        }
        if(msg==WM_COMMAND){if(wp==1)self->ui_.show();if(wp==2)self->quitting_=true;return 0;}
        if(msg==WM_QUERYENDSESSION||msg==WM_ENDSESSION){self->capture_.stop();self->presenter_.hide();return msg==WM_QUERYENDSESSION?TRUE:0;}
    }catch(const std::exception& e){self->fail(e);}
    return Interface::window_proc(hwnd,msg,wp,lp,self);
}
int Application::run(){
    while(!quitting_){
        MSG msg{};
        while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){
            if(msg.message==WM_QUIT){quitting_=true;break;}
            TranslateMessage(&msg);DispatchMessageW(&msg);
        }
        if(quitting_)break;
        const auto now=clock_.now();
        try{
            if(now-last_poll_>=250000){poll_target();last_poll_=now;}
            if(want_capture_&&!paused_&&capture_.acquire()){
                capture_intervals_.sample(clock_.now());last_capture_frame_=clock_.now();
                status_="Capturing. Overlay follows the game's size and monitor.";error_.clear();
            }
            if(want_capture_&&!paused_&&capture_.texture()&&settings_.overlay_enabled&&GetForegroundWindow()==desired_target_&&!capture::exclusive_fullscreen_hint())
                presenter_.render(desired_target_,capture_.texture(),capture_.size(),settings_.click_through);
            if(want_capture_&&!paused_&&clock_.milliseconds(last_capture_frame_,clock_.now())>5000)
                error_="No frames for 5 seconds. Try Borderless Windowed mode; exclusive fullscreen and protected content may not be capturable.";
            if(now-last_ui_>=333333){
                UiModel model{settings_,windows_,hardware::describe_adapter(device_.luid()),status_,error_,notice_,want_capture_,paused_,capture_supported_,capture_intervals_.fps(),capture_.size().cx,capture_.size().cy};
                apply_actions(ui_.render(model));last_ui_=now;
            }
        }catch(const std::exception& e){
            fail(e);
            if(!ui_.visible())ui_.show();
            if(const Error* typed=dynamic_cast<const Error*>(&e);typed&&(typed->code()==DXGI_ERROR_DEVICE_REMOVED||typed->code()==DXGI_ERROR_DEVICE_RESET))fatal_=true;
        }
        if(fatal_){MessageBoxW(ui_.handle(),L"Graphics device lost. Close the game if necessary, then restart Superior.",L"Superior",MB_OK|MB_ICONERROR);break;}
        HANDLE handles[]={capture_.event()};
        const DWORD timeout=want_capture_&&!paused_?1:50;
        const DWORD wait=MsgWaitForMultipleObjectsEx(1,handles,timeout,QS_ALLINPUT,MWMO_INPUTAVAILABLE);
        if(wait==WAIT_FAILED)check_win(false,"MsgWaitForMultipleObjectsEx");
    }
    logger_.write("INFO","Normal shutdown");return 0;
}
} // namespace superior
