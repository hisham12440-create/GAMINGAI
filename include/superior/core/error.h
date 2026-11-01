#pragma once
#include <stdexcept>
#include <string>
#include <windows.h>
namespace superior {
class Error : public std::runtime_error {
public:
    explicit Error(const std::string& message, HRESULT code = E_FAIL)
        : std::runtime_error(message), code_(code) {}
    HRESULT code() const noexcept { return code_; }
private:
    HRESULT code_;
};
inline void check_hr(HRESULT hr, const char* operation) {
    if (FAILED(hr)) throw Error(std::string(operation) + " failed (HRESULT 0x" +
        [hr] { char b[16]; wsprintfA(b, "%08X", static_cast<unsigned>(hr)); return std::string(b); }() + ")", hr);
}
inline void check_win(BOOL result, const char* operation) {
    const auto error = result ? ERROR_SUCCESS : GetLastError();
    if (!result) throw Error(std::string(operation) + " failed (Win32 error " + std::to_string(error) + ")",
                             HRESULT_FROM_WIN32(error));
}
inline void* checked(void* p, const char* operation) {
    if (!p) check_win(false, operation);
    return p;
}
} // namespace superior
