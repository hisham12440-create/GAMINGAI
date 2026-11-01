#include "superior/ui/interface.h"
#include "superior/core/error.h"
#include "superior/core/strings.h"
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <algorithm>
#include <cstring>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND,UINT,WPARAM,LPARAM);
namespace superior {
namespace {
const char* tr(bool ar,const char* en,const char* arabic) {return ar?arabic:en;}
void copy_field(char (&dest)[81],const std::string& value) {
    if(value.size()>80)throw Error("Hotkey field too long");
    std::strcpy(dest,value.c_str());
}
}
Interface::Interface(present::Device& device,const std::filesystem::path& font) : device_(device),font_(font) {
    window_=std::make_unique<Window>(L"Superior.Control",L"Superior",WS_OVERLAPPEDWINDOW,0,900,740,window_proc,this);
    DXGI_SWAP_CHAIN_DESC1 sd{};sd.Width=900;sd.Height=740;sd.Format=DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.SampleDesc.Count=1;sd.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;sd.BufferCount=2;
    sd.Scaling=DXGI_SCALING_STRETCH;sd.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;
    Microsoft::WRL::ComPtr<IDXGIDevice> dxgi;check_hr(device.get()->QueryInterface(IID_PPV_ARGS(&dxgi)),"Query UI DXGI device");
    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;check_hr(dxgi->GetAdapter(&adapter),"Get UI adapter");
    Microsoft::WRL::ComPtr<IDXGIFactory2> factory;check_hr(adapter->GetParent(IID_PPV_ARGS(&factory)),"Get UI factory");
    check_hr(factory->CreateSwapChainForHwnd(device.get(),handle(),&sd,nullptr,nullptr,&swap_),"Create UI swap chain");
    create_target();
    imgui_=ImGui::CreateContext();ImGui::SetCurrentContext(imgui_);
    auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;io.ConfigWindowsMoveFromTitleBarOnly=true;
    ImGui::StyleColorsDark();ImGui::GetStyle().WindowRounding=7;ImGui::GetStyle().FrameRounding=5;
    ImGui::GetStyle().Colors[ImGuiCol_WindowBg]=ImVec4(.055f,.065f,.085f,1);
    ImGui::GetStyle().Colors[ImGuiCol_Button]=ImVec4(.12f,.28f,.36f,1);
    if(!ImGui_ImplWin32_Init(handle()))throw Error("ImGui Win32 initialization failed");
    if(!ImGui_ImplDX11_Init(device.get(),device.context()))throw Error("ImGui DX11 initialization failed");
    rebuild_font(96);
    initialized_=true;
}
Interface::~Interface() {
    if(imgui_)ImGui::SetCurrentContext(imgui_);
    if(initialized_)ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    text_.reset();if(imgui_)ImGui::DestroyContext(imgui_);
}
HWND Interface::handle()const noexcept{return window_->handle();}
void Interface::set_window_handler(Window::Handler handler,void* context)noexcept{window_->set_handler(handler,context);}
bool Interface::visible()const noexcept{return IsWindowVisible(handle());}
void Interface::show()noexcept{window_->show(SW_SHOW);SetForegroundWindow(handle());}
void Interface::hide()noexcept{window_->show(SW_HIDE);}
void Interface::release_target()noexcept{device_.context()->OMSetRenderTargets(0,nullptr,nullptr);rtv_.Reset();}
void Interface::create_target(){Microsoft::WRL::ComPtr<ID3D11Texture2D> buffer;check_hr(swap_->GetBuffer(0,IID_PPV_ARGS(&buffer)),"Get UI buffer");check_hr(device_.get()->CreateRenderTargetView(buffer.Get(),nullptr,&rtv_),"Create UI RTV");}
void Interface::rebuild_font(float dpi){
    ImGui_ImplDX11_InvalidateDeviceObjects();text_.reset();ImGui::GetIO().Fonts->Clear();
    text_=std::make_unique<TextEngine>(*ImGui::GetIO().Fonts,font_,18.0f*dpi/96.0f);
    if(!ImGui_ImplDX11_CreateDeviceObjects())throw Error("Cannot upload Arabic/Latin font atlas");
}
LRESULT CALLBACK Interface::window_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp,void* context)noexcept{
    auto* self=static_cast<Interface*>(context);
    if(self&&self->imgui_){ImGui::SetCurrentContext(self->imgui_);if(ImGui_ImplWin32_WndProcHandler(hwnd,msg,wp,lp))return 1;}
    if(msg==WM_SIZE&&self){self->width_=LOWORD(lp);self->height_=HIWORD(lp);}
    if(msg==WM_DPICHANGED&&self){self->pending_dpi_=HIWORD(wp);auto* r=reinterpret_cast<RECT*>(lp);SetWindowPos(hwnd,nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER|SWP_NOACTIVATE);}
    if(msg==WM_CLOSE){ShowWindow(hwnd,SW_HIDE);return 0;}
    return DefWindowProcW(hwnd,msg,wp,lp);
}
void Interface::draw_button(const char* id,const std::string& label,float width,bool enabled,std::function<void()> action){
    const bool ar=ImGui::GetIO().UserData!=nullptr;
    const auto size=text_->measure(label,ar);const ImVec2 dim(width>0?width:size.x+28, text_->line_height()+12);
    ImGui::BeginDisabled(!enabled);const ImVec2 p=ImGui::GetCursorScreenPos();
    if(ImGui::Button(id,dim))action();
    const auto color=ImGui::GetColorU32(ImGuiCol_Text);
    text_->text_at(label,ar,ImVec2(p.x+(dim.x-size.x)*.5f,p.y+6),color);ImGui::EndDisabled();
}
void Interface::draw_checkbox(const char* id,const std::string& label,bool& value){
    const bool ar=ImGui::GetIO().UserData!=nullptr;const float h=text_->line_height()+8;
    const ImVec2 p=ImGui::GetCursorScreenPos();const float width=ImGui::GetContentRegionAvail().x;
    ImGui::PushID(id);if(ImGui::InvisibleButton("toggle",ImVec2(width,h)))value=!value;ImGui::PopID();
    const float box=18;const float bx=ar?p.x+width-box:p.x;
    auto* draw=ImGui::GetWindowDrawList();draw->AddRect(ImVec2(bx,p.y+3),ImVec2(bx+box,p.y+3+box),IM_COL32(110,150,170,255),4);
    if(value)draw->AddRectFilled(ImVec2(bx+4,p.y+7),ImVec2(bx+box-4,p.y+box-1),IM_COL32(70,200,185,255),2);
    const auto measure=text_->measure(label,ar);const float tx=ar?bx-measure.x-10:p.x+box+10;
    text_->text_at(label,ar,ImVec2(tx,p.y+3));ImGui::Dummy(ImVec2(width,0));
}
UiActions Interface::render(const UiModel& m){
    UiActions actions;actions.settings=m.settings;actions.selected=selected_;
    if(pending_dpi_&&pending_dpi_!=dpi_){dpi_=pending_dpi_;rebuild_font(static_cast<float>(dpi_));pending_dpi_=0;}
    if(!visible())return actions;
    if(width_<1||height_<1)return actions;
    DXGI_SWAP_CHAIN_DESC desc{};check_hr(swap_->GetDesc(&desc),"Get UI swap desc");
    if(desc.BufferDesc.Width!=static_cast<UINT>(width_)||desc.BufferDesc.Height!=static_cast<UINT>(height_)){
        release_target();check_hr(swap_->ResizeBuffers(0,width_,height_,DXGI_FORMAT_UNKNOWN,0),"Resize UI");create_target();
    }
    bool first=!fields_initialized_;
    if(first){copy_field(show_ui_,m.settings.hotkeys.show_ui);copy_field(toggle_overlay_,m.settings.hotkeys.toggle_overlay);copy_field(click_through_,m.settings.hotkeys.click_through);copy_field(toggle_capture_,m.settings.hotkeys.toggle_capture);first=false;fields_initialized_=true;}
    ImGui::SetCurrentContext(imgui_);auto& io=ImGui::GetIO();io.UserData=m.settings.language==Language::Arabic?this:nullptr;
    ImGui_ImplDX11_NewFrame();ImGui_ImplWin32_NewFrame();ImGui::NewFrame();
    const bool ar=m.settings.language==Language::Arabic;
    ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({static_cast<float>(width_),static_cast<float>(height_)});
    ImGui::Begin("Superior",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoSavedSettings);
    text_->text("SUPERIOR",false,IM_COL32(90,220,205,255));
    text_->text(tr(ar,"Real-time capture / display foundation","أساس الالتقاط والعرض في الزمن الحقيقي"),ar);
    text_->text(tr(ar,"Stage 1: spatial scaling only. No frame generation or neural enhancement yet.","المرحلة 1: تكبير مكاني فقط. لا يوجد توليد إطارات أو تحسين عصبي بعد."),ar,IM_COL32(170,185,200,255));
    ImGui::Separator();
    draw_button("language",ar?"English":"العربية",130,true,[&]{actions.settings.language=ar?Language::English:Language::Arabic;});
    ImGui::Separator();
    text_->text(tr(ar,"Game window","نافذة اللعبة"),ar);
    draw_button("refresh",tr(ar,"Refresh windows","تحديث النوافذ"),180,true,[&]{actions.refresh=true;});
    if(ImGui::BeginChild("windows",ImVec2(0,145),ImGuiChildFlags_Borders)){
        for(int i=0;i<static_cast<int>(m.windows.size());++i){
            const auto title=wide_to_utf8(m.windows[i].title);const ImVec2 p=ImGui::GetCursorScreenPos();
            ImGui::PushID(i);if(ImGui::Selectable("##window",selected_==i))selected_=i;ImGui::PopID();
            text_->text_at(title,ar,ImVec2(p.x+5,p.y+2));
        }
        ImGui::EndChild();
    }
    actions.selected=selected_;
    draw_button("start",tr(ar,"Start capture","بدء الالتقاط"),180,m.capture_supported&&!m.capturing&&selected_<static_cast<int>(m.windows.size()),[&]{actions.start=true;});
    ImGui::SameLine();draw_button("stop",tr(ar,"Stop capture","إيقاف الالتقاط"),180,m.capturing,[&]{actions.stop=true;});
    ImGui::Separator();
    draw_checkbox("overlay",tr(ar,"Overlay enabled","تفعيل الطبقة المعروضة"),actions.settings.overlay_enabled);
    draw_checkbox("click",tr(ar,"Click-through (game receives input)","النقر عبر الطبقة إلى اللعبة"),actions.settings.click_through);
    ImGui::Separator();
    text_->text(tr(ar,"Global hotkeys — Ctrl / Alt / Shift / Win + key","اختصارات عامة — Ctrl / Alt / Shift / Win مع مفتاح"),ar);
    ImGui::PushItemWidth(-1);
    text_->text(tr(ar,"Show / hide this panel","إظهار أو إخفاء هذه اللوحة"),ar);ImGui::InputText("##show",show_ui_,sizeof(show_ui_));
    text_->text(tr(ar,"Show / hide overlay","إظهار أو إخفاء العرض"),ar);ImGui::InputText("##overlay",toggle_overlay_,sizeof(toggle_overlay_));
    text_->text(tr(ar,"Toggle click-through","تبديل النقر عبر الطبقة"),ar);ImGui::InputText("##click",click_through_,sizeof(click_through_));
    text_->text(tr(ar,"Start / stop capture","بدء أو إيقاف الالتقاط"),ar);ImGui::InputText("##capture",toggle_capture_,sizeof(toggle_capture_));
    ImGui::PopItemWidth();
    draw_button("apply",tr(ar,"Apply hotkeys and save settings","تطبيق الاختصارات وحفظ الإعدادات"),300,true,[&]{
        actions.settings.hotkeys={show_ui_,toggle_overlay_,click_through_,toggle_capture_};actions.save_hotkeys=true;
    });
    ImGui::Separator();
    const auto gpu=wide_to_utf8(m.adapter.name);
    text_->text("GPU: "+gpu+" ("+m.adapter.vendor+")",false);
    text_->text(tr(ar,"Capture: ","الالتقاط: ")+std::string(m.capturing?(m.paused?"paused / minimized":"active"):"stopped"),ar);
    if(m.capture_width>0)text_->text(std::to_string(m.capture_width)+" x "+std::to_string(m.capture_height)+"  |  "+(m.capture_fps>0?std::to_string(m.capture_fps).substr(0,5)+" capture FPS":"waiting for frames"),false);
    text_->text(m.status,ar);
    if(!m.notice.empty())text_->text(m.notice,ar,IM_COL32(245,190,90,255));
    if(!m.error.empty())text_->text(m.error,ar,IM_COL32(255,130,120,255));
    text_->text(tr(ar,"Use borderless/windowed mode. Exclusive fullscreen is not supported.","استخدم وضع النافذة بلا حدود. وضع ملء الشاشة الحصري غير مدعوم."),ar);
    draw_button("exit",tr(ar,"Exit Superior","إنهاء Superior"),150,true,[&]{actions.exit=true;});
    ImGui::End();ImGui::Render();
    const float clear[]={.04f,.05f,.07f,1};device_.context()->OMSetRenderTargets(1,&rtv_,nullptr);
    device_.context()->ClearRenderTargetView(rtv_.Get(),clear);ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    check_hr(swap_->Present(0,0),"Present control panel");device_.check_lost();return actions;
}
} // namespace superior
