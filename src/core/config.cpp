#include "superior/core/config.h"
#include "superior/core/hotkey.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <set>
#include <windows.h>

namespace superior {
using nlohmann::json;
namespace {
template<class T> T value(const json& j, const char* key, T fallback) {
    if (!j.contains(key)) return fallback;
    if (!j.at(key).is_number_integer()) throw std::runtime_error(std::string("Integer required: ") + key);
    return j.at(key).get<T>();
}
std::string text(const json& j, const char* key, const std::string& fallback) {
    if (!j.contains(key)) return fallback;
    if (!j.at(key).is_string()) throw std::runtime_error(std::string("String required: ") + key);
    return j.at(key).get<std::string>();
}
bool boolean(const json& j, const char* key, bool fallback) {
    if (!j.contains(key)) return fallback;
    if (!j.at(key).is_boolean()) throw std::runtime_error(std::string("Boolean required: ") + key);
    return j.at(key).get<bool>();
}
std::string read_file(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open settings file");
    if (std::filesystem::file_size(path) > 65536) throw std::runtime_error("Settings exceed 64 KiB");
    std::ostringstream s; s << f.rdbuf();
    if (f.bad()) throw std::runtime_error("Cannot read settings");
    return s.str();
}
} // namespace
std::string Settings::validate() const {
    if (schema_version != 1) return "Unsupported settings schema_version (expected 1)";
    const std::pair<std::string, std::string> keys[] = {
        {"show_ui", hotkeys.show_ui}, {"toggle_overlay", hotkeys.toggle_overlay},
        {"click_through", hotkeys.click_through}, {"toggle_capture", hotkeys.toggle_capture}};
    std::set<std::string> unique;
    for (const auto& [name, chord] : keys) {
        try {
            const auto parsed = parse_hotkey(chord);
            if (!unique.insert(canonical_hotkey(parsed)).second) return "Hotkeys must be distinct: " + name;
        } catch (const std::exception& e) { return name + ": " + e.what(); }
    }
    return {};
}
std::string serialize_settings(const Settings& s) {
    const auto error = s.validate();
    if (!error.empty()) throw std::runtime_error(error);
    return json{{"schema_version", s.schema_version}, {"language", s.language == Language::Arabic ? "ar" : "en"},
        {"click_through", s.click_through}, {"overlay_enabled", s.overlay_enabled},
        {"show_ui_on_start", s.show_ui_on_start}, {"hotkeys", {
            {"show_ui", s.hotkeys.show_ui}, {"toggle_overlay", s.hotkeys.toggle_overlay},
            {"click_through", s.hotkeys.click_through}, {"toggle_capture", s.hotkeys.toggle_capture}}}}.dump(2) + "\n";
}
Settings parse_settings(const std::string& input) {
    if (input.size() > 65536) throw std::runtime_error("Settings exceed 64 KiB");
    const auto j = json::parse(input);
    if (!j.is_object()) throw std::runtime_error("Settings root must be an object");
    Settings s;
    s.schema_version = value<int>(j, "schema_version", 1);
    const auto language = text(j, "language", "en");
    if (language != "en" && language != "ar") throw std::runtime_error("language must be en or ar");
    s.language = language == "ar" ? Language::Arabic : Language::English;
    s.click_through = boolean(j, "click_through", true);
    s.overlay_enabled = boolean(j, "overlay_enabled", true);
    s.show_ui_on_start = boolean(j, "show_ui_on_start", true);
    if (j.contains("hotkeys")) {
        const auto& h = j.at("hotkeys");
        if (!h.is_object()) throw std::runtime_error("hotkeys must be an object");
        s.hotkeys.show_ui = text(h, "show_ui", s.hotkeys.show_ui);
        s.hotkeys.toggle_overlay = text(h, "toggle_overlay", s.hotkeys.toggle_overlay);
        s.hotkeys.click_through = text(h, "click_through", s.hotkeys.click_through);
        s.hotkeys.toggle_capture = text(h, "toggle_capture", s.hotkeys.toggle_capture);
    }
    const auto error = s.validate();
    if (!error.empty()) throw std::runtime_error(error);
    return s;
}
SettingsDocument load_settings(const std::filesystem::path& path) {
    SettingsDocument result;
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) return result;
    try { result.settings = parse_settings(read_file(path)); return result; }
    catch (const std::exception& e) { result.notice = std::string("Primary settings invalid: ") + e.what(); }
    try {
        result.settings = parse_settings(read_file(std::filesystem::path(path.string() + ".bak")));
        result.notice += "; restored the last valid backup";
    } catch (const std::exception&) {
        result.settings = {};
        result.notice += "; using defaults (invalid files preserved)";
    }
    return result;
}
void save_settings(const std::filesystem::path& path, const Settings& settings) {
    const auto data = serialize_settings(settings);
    std::filesystem::create_directories(path.parent_path());
    const auto temp = std::filesystem::path(path.string() + ".tmp");
    {
        std::ofstream f(temp, std::ios::binary | std::ios::trunc);
        if (!f) throw std::runtime_error("Cannot create settings temporary file");
        f.write(data.data(), static_cast<std::streamsize>(data.size()));
        f.flush();
        if (!f) throw std::runtime_error("Cannot write settings");
    }
    // std::ofstream cannot expose its HANDLE for FlushFileBuffers. Open the closed
    // temporary file and flush its OS cache before the atomic replacement.
    const auto wide = temp.wstring();
    HANDLE file = CreateFileW(wide.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot reopen settings for flush");
    const BOOL flushed = FlushFileBuffers(file);
    const DWORD error = flushed ? ERROR_SUCCESS : GetLastError();
    if (!CloseHandle(file) && flushed) throw std::runtime_error("Cannot close settings flush handle");
    if (!flushed) throw std::runtime_error("Cannot flush settings: " + std::to_string(error));
    std::error_code ec;
    if (std::filesystem::exists(path, ec)) {
        const auto backup = std::filesystem::path(path.string() + ".bak");
        if (!MoveFileExW(path.c_str(), backup.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Cannot back up settings");
    }
    if (!MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("Cannot replace settings atomically");
}
} // namespace superior
