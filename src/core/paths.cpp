#include "superior/core/paths.h"
#include "superior/core/error.h"
#include <windows.h>
#include <shlobj.h>
namespace superior {
std::filesystem::path local_data_directory() {
    PWSTR raw = nullptr;
    check_hr(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &raw), "SHGetKnownFolderPath");
    const std::filesystem::path base(raw);
    CoTaskMemFree(raw);
    auto path = base / L"Superior";
    std::filesystem::create_directories(path);
    return path;
}
std::filesystem::path executable_directory() {
    std::wstring buffer(32768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (!length || length == buffer.size()) check_win(false, "GetModuleFileNameW");
    buffer.resize(length);
    return std::filesystem::path(buffer).parent_path();
}
} // namespace superior
