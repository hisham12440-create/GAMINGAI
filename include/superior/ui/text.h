#pragma once
#include <imgui.h>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
namespace superior {
class TextEngine {
public:
    TextEngine(ImFontAtlas& atlas, const std::filesystem::path& font, float pixel_size);
    ~TextEngine();
    TextEngine(const TextEngine&) = delete;
    TextEngine& operator=(const TextEngine&) = delete;
    void text(const std::string& utf8, bool rtl, ImU32 color = IM_COL32_WHITE);
    void text_at(const std::string& utf8, bool rtl, ImVec2 position, ImU32 color = IM_COL32_WHITE);
    ImVec2 measure(const std::string& utf8, bool rtl);
    float line_height() const;
    const std::filesystem::path& font_path() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace superior
