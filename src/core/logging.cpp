#include "superior/core/logging.h"
#include "superior/core/error.h"
#include <chrono>
#include <vector>
namespace superior {
Logger* Logger::active_ = nullptr;
Logger::Logger(const std::filesystem::path& directory, std::size_t max_bytes, unsigned files)
    : path_(directory / L"superior.log"), max_bytes_(max_bytes), files_(files) {
    if (max_bytes < 4096 || files < 1 || files > 10) throw Error("Invalid log rotation policy");
    std::filesystem::create_directories(directory);
    file_ = CreateFileW(path_.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr);
    if (file_ == INVALID_HANDLE_VALUE) check_win(false, "Create log");
    LARGE_INTEGER size{};
    check_win(GetFileSizeEx(file_, &size), "Get log size");
    bytes_ = static_cast<std::size_t>(size.QuadPart);
    const auto crash_path = directory / L"crash.log";
    crash_file_ = CreateFileW(crash_path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                              OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr);
    if (crash_file_ == INVALID_HANDLE_VALUE) check_win(false, "Create crash log");
    check_win(SetFilePointer(crash_file_, 0, nullptr, FILE_END) != INVALID_SET_FILE_POINTER, "Seek crash log");
    active_ = this;
}
Logger::~Logger() {
    if (active_ == this) active_ = nullptr;
    if (file_ != INVALID_HANDLE_VALUE) CloseHandle(file_);
    if (crash_file_ != INVALID_HANDLE_VALUE) CloseHandle(crash_file_);
}
void Logger::rotate() noexcept {
    if (bytes_ < max_bytes_) return;
    if (file_ != INVALID_HANDLE_VALUE) { CloseHandle(file_); file_ = INVALID_HANDLE_VALUE; }
    std::error_code ec;
    for (unsigned i = files_ - 1; i > 0; --i) {
        auto from = path_.wstring(); if (i > 1) from += L"." + std::to_wstring(i - 1);
        const auto to = path_.wstring() + L"." + std::to_wstring(i);
        MoveFileExW(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING);
    }
    file_ = CreateFileW(path_.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                        CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr);
    bytes_ = 0;
}
void Logger::write(const char* level, const std::string& message) noexcept {
    try {
        std::lock_guard lock(mutex_);
        rotate();
        if (file_ == INVALID_HANDLE_VALUE) return;
        const auto now = std::chrono::system_clock::now().time_since_epoch();
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        std::string line = std::to_string(ms) + " [" + level + "] " + message.substr(0, 8192) + "\n";
        DWORD written = 0;
        if (WriteFile(file_, line.data(), static_cast<DWORD>(line.size()), &written, nullptr) && written == line.size())
            bytes_ += written;
    } catch (...) { OutputDebugStringA("Superior: logging failed\n"); }
}
void Logger::crash(unsigned code, void* address) noexcept {
    Logger* logger = active_;
    if (!logger || logger->crash_file_ == INVALID_HANDLE_VALUE) return;
    char buffer[256];
    const int n = wsprintfA(buffer, "CRASH code=0x%08X address=0x%p\r\n", code, address);
    DWORD written = 0;
    WriteFile(logger->crash_file_, buffer, static_cast<DWORD>(n), &written, nullptr);
    FlushFileBuffers(logger->crash_file_);
}
void Logger::install_crash_handler() noexcept {
    SetUnhandledExceptionFilter([](EXCEPTION_POINTERS* ep) noexcept -> LONG {
        crash(ep->ExceptionRecord->ExceptionCode, ep->ExceptionRecord->ExceptionAddress);
        return EXCEPTION_EXECUTE_HANDLER;
    });
}
} // namespace superior
