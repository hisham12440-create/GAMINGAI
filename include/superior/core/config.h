#pragma once
#include <filesystem>
#include <string>
namespace superior {
enum class Language { English, Arabic };
struct Hotkey {
    std::string show_ui = "Ctrl+Alt+U";
    std::string toggle_overlay = "Ctrl+Alt+O";
    std::string click_through = "Ctrl+Alt+T";
    std::string toggle_capture = "Ctrl+Alt+S";
};
struct Settings {
    int schema_version = 1;
    Language language = Language::English;
    bool click_through = true;
    bool overlay_enabled = true;
    bool show_ui_on_start = true;
    Hotkey hotkeys;
    std::string validate() const; // empty means valid
};
struct SettingsDocument {
    Settings settings;
    std::string notice;
};
std::string serialize_settings(const Settings& settings);
Settings parse_settings(const std::string& json);
SettingsDocument load_settings(const std::filesystem::path& path);
void save_settings(const std::filesystem::path& path, const Settings& settings);
} // namespace superior
