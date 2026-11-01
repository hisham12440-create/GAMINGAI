#pragma once
#include <filesystem>
#include <mutex>
#include <string>
#include <windows.h>
namespace superior {
class Logger {
public:
    Logger(const std::filesystem::path& directory, std::size_t max_bytes = 1048576, unsigned files = 3);
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    void write(const char* level, const std::string& message) noexcept;
    void install_crash_handler() noexcept;
    static void crash(unsigned code, void* address) noexcept;
private:
    void rotate() noexcept;
    std::filesystem::path path_;
    std::size_t max_bytes_;
    unsigned files_;
    std::size_t bytes_ = 0;
    HANDLE file_ = INVALID_HANDLE_VALUE;
    HANDLE crash_file_ = INVALID_HANDLE_VALUE;
    std::mutex mutex_;
    static Logger* active_;
};
} // namespace superior
