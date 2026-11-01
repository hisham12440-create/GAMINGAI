#include "superior/core/strings.h"
#include "superior/core/error.h"
#include <windows.h>
#include <limits>
namespace superior {
std::wstring utf8_to_wide(const std::string& text) {
    if (text.empty()) return {};
    if (text.size() > static_cast<size_t>((std::numeric_limits<int>::max)())) throw Error("UTF-8 text too long");
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (!size) check_win(false, "MultiByteToWideChar");
    std::wstring result(static_cast<size_t>(size), L'\0');
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), result.data(), size))
        check_win(false, "MultiByteToWideChar");
    return result;
}
std::string wide_to_utf8(const std::wstring& text) {
    if (text.empty()) return {};
    if (text.size() > static_cast<size_t>((std::numeric_limits<int>::max)())) throw Error("UTF-16 text too long");
    const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (!size) check_win(false, "WideCharToMultiByte");
    std::string result(static_cast<size_t>(size), '\0');
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), result.data(), size, nullptr, nullptr))
        check_win(false, "WideCharToMultiByte");
    return result;
}
} // namespace superior
