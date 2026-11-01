#pragma once
#include <windows.h>
#include <string>
namespace superior {
class Window {
public:
    using Handler = LRESULT (*)(HWND,UINT,WPARAM,LPARAM,void*);
    Window(const wchar_t* class_name, const wchar_t* title, DWORD style, DWORD ex_style,
           int width, int height, Handler handler, void* context);
    ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    HWND handle() const noexcept { return hwnd_; }
    void set_handler(Handler handler, void* context) noexcept { handler_ = handler; context_ = context; }
    void show(int command) noexcept;
    void client_size(int& width, int& height) const;
private:
    static LRESULT CALLBACK route(HWND,UINT,WPARAM,LPARAM) noexcept;
    HWND hwnd_ = nullptr;
    Handler handler_;
    void* context_;
};
} // namespace superior
