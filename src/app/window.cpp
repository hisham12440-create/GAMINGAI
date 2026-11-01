#include "superior/app/window.h"
#include "superior/core/error.h"
namespace superior {
Window::Window(const wchar_t* class_name,const wchar_t* title,DWORD style,DWORD ex_style,
               int width,int height,Handler handler,void* context) : handler_(handler),context_(context) {
    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc = route; wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = class_name; wc.hCursor = LoadCursorW(nullptr,IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);
    const ATOM atom = RegisterClassExW(&wc);
    if (!atom && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) check_win(false,"RegisterClassExW");
    hwnd_ = CreateWindowExW(ex_style,class_name,title,style,CW_USEDEFAULT,CW_USEDEFAULT,width,height,
                            nullptr,nullptr,wc.hInstance,this);
    if (!hwnd_) check_win(false,"CreateWindowExW");
}
Window::~Window() { if (hwnd_) DestroyWindow(hwnd_); }
LRESULT CALLBACK Window::route(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) noexcept {
    Window* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = static_cast<Window*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    } else self = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
    if (!self) return DefWindowProcW(hwnd,msg,wp,lp);
    try { return self->handler_(hwnd,msg,wp,lp,self->context_); }
    catch (const std::exception& e) {
        OutputDebugStringA(e.what());
        if (msg == WM_CREATE) return -1;
        return DefWindowProcW(hwnd,msg,wp,lp);
    } catch (...) { if (msg == WM_CREATE) return -1; return DefWindowProcW(hwnd,msg,wp,lp); }
}
void Window::show(int command) noexcept { ShowWindow(hwnd_,command); UpdateWindow(hwnd_); }
void Window::client_size(int& width,int& height) const {
    RECT r{}; check_win(GetClientRect(hwnd_,&r),"GetClientRect"); width=r.right-r.left; height=r.bottom-r.top;
}
} // namespace superior
