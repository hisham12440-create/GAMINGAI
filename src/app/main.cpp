#include "superior/app/application.h"
#include "superior/core/error.h"
#include "superior/core/strings.h"
#include <winrt/base.h>
#include <shellapi.h>
#include <objbase.h>
namespace {
LONG WINAPI unhandled(EXCEPTION_POINTERS* info) {
    superior::Logger::crash(info->ExceptionRecord->ExceptionCode,info->ExceptionRecord->ExceptionAddress);
    return EXCEPTION_EXECUTE_HANDLER;
}
}
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int) {
    SetUnhandledExceptionFilter(unhandled);
    winrt::init_apartment(winrt::apartment_type::multi_threaded);
    HANDLE instance=CreateMutexW(nullptr,FALSE,L"Local\\Superior.SingleInstance");
    if(!instance){superior::check_win(false,"Create single-instance mutex");return 1;}
    const bool already=GetLastError()==ERROR_ALREADY_EXISTS;
    if(already){CloseHandle(instance);MessageBoxW(nullptr,L"Superior is already running. Use its notification-area icon.",L"Superior",MB_OK);return 0;}
    int result=1;
    try { superior::Application app; result=app.run(); }
    catch(const std::exception& e) {
        const auto message=superior::utf8_to_wide(std::string("Superior could not start:\n")+e.what()+
            "\n\nSee %LOCALAPPDATA%\\Superior\\logs when available. Keep the shaders folder next to Superior.exe.");
        MessageBoxW(nullptr,message.c_str(),L"Superior",MB_OK|MB_ICONERROR);
    }
    if(instance)CloseHandle(instance);
    return result;
}
