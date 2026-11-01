#include "superior/app/tray.h"
#include "superior/core/error.h"
namespace superior {
Tray::Tray(HWND owner) : owner_(owner) {
    data_.cbSize=sizeof(data_);data_.hWnd=owner;data_.uID=1;
    data_.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP;data_.uCallbackMessage=message;
    data_.hIcon=LoadIconW(nullptr,IDI_APPLICATION);
    if(!data_.hIcon) check_win(false,"Load tray icon");
    wcscpy_s(data_.szTip,L"Superior");
    if(!Shell_NotifyIconW(NIM_ADD,&data_)) throw Error("Cannot add notification-area icon");
    added_=true;data_.uVersion=NOTIFYICON_VERSION_4;
    if(!Shell_NotifyIconW(NIM_SETVERSION,&data_)) throw Error("Cannot set notification-area version");
}
Tray::~Tray() {if(added_) Shell_NotifyIconW(NIM_DELETE,&data_);}
void Tray::show_menu(bool arabic) noexcept {
    HMENU menu=CreatePopupMenu();if(!menu)return;
    AppendMenuW(menu,MF_STRING,1,arabic?L"إظهار Superior":L"Show Superior");
    AppendMenuW(menu,MF_STRING,2,arabic?L"إنهاء":L"Exit");
    POINT p{};if(!GetCursorPos(&p)){DestroyMenu(menu);return;}
    SetForegroundWindow(owner_);
    const UINT command=TrackPopupMenu(menu,TPM_RIGHTBUTTON|TPM_RETURNCMD,p.x,p.y,0,owner_,nullptr);
    if(command) PostMessageW(owner_,WM_COMMAND,command,0);
    DestroyMenu(menu);PostMessageW(owner_,WM_NULL,0,0);
}
} // namespace superior
