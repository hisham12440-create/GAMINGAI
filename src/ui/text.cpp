#include "superior/ui/text.h"
#include "superior/core/error.h"
#include "superior/core/strings.h"
#include <hb.h>
#include <hb-ot.h>
#include <windows.h>
#include <usp10.h>
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include <imstb_truetype.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <unordered_map>

namespace superior {
namespace {
constexpr ImWchar glyph_codepoint(unsigned glyph) { return static_cast<ImWchar>(0xE000 + glyph); }
}
struct TextEngine::Impl {
    ImFontAtlas& atlas;
    ImFont* font = nullptr;
    std::filesystem::path path;
    std::vector<unsigned char> bytes;
    stbtt_fontinfo stb{};
    hb_blob_t* blob = nullptr;
    hb_face_t* face = nullptr;
    hb_font_t* hb_font = nullptr;
    float size, scale, baseline;
    struct Glyph { unsigned gid; float x, y, advance; };
    struct Shape { std::vector<Glyph> glyphs; float width = 0; };
    std::unordered_map<std::string, Shape> cache;

    Impl(ImFontAtlas& a, const std::filesystem::path& p, float pixels) : atlas(a), path(p), size(pixels) {
        std::ifstream f(p, std::ios::binary | std::ios::ate);
        if (!f) throw Error("Cannot open UI font: " + p.string());
        const auto length = f.tellg();
        if (length <= 0 || length > 32 * 1024 * 1024) throw Error("UI font must be between 1 byte and 32 MiB");
        bytes.resize(static_cast<size_t>(length)); f.seekg(0);
        f.read(reinterpret_cast<char*>(bytes.data()), length);
        if (!f) throw Error("Cannot read UI font");
        if (!stbtt_InitFont(&stb, bytes.data(), stbtt_GetFontOffsetForIndex(bytes.data(), 0)))
            throw Error("Invalid TrueType UI font");
        blob = hb_blob_create(reinterpret_cast<const char*>(bytes.data()), static_cast<unsigned>(bytes.size()),
                              HB_MEMORY_MODE_READONLY, nullptr, nullptr);
        face = hb_face_create(blob, 0);
        hb_font = hb_font_create(face);
        const unsigned count = hb_face_get_glyph_count(face);
        if (!count || count > 0x1900) throw Error("Font has too many glyphs for the atlas mapping");
        unsigned probe = 0;
        if (!hb_font_get_nominal_glyph(hb_font, 0x0627, &probe) || !probe)
            throw Error("UI font does not contain Arabic letters");
        scale = stbtt_ScaleForPixelHeight(&stb, size);
        int ascent = 0, descent = 0, gap = 0;
        stbtt_GetFontVMetrics(&stb, &ascent, &descent, &gap);
        baseline = std::ceil(ascent * scale);
        const int upem = static_cast<int>(hb_face_get_upem(face));
        hb_font_set_scale(hb_font, static_cast<int>(std::lround(scale * upem * 64)),
                          static_cast<int>(std::lround(scale * upem * 64)));
        static const ImWchar latin[] = {0x20, 0x024F, 0};
        font = atlas.AddFontFromFileTTF(p.string().c_str(), size, nullptr, latin);
        if (!font) throw Error("ImGui could not load UI font");
        for (unsigned gid = 0; gid < count; ++gid) {
            int w = 0, h = 0, xo = 0, yo = 0;
            unsigned char* bitmap = stbtt_GetGlyphBitmap(&stb, scale, scale, static_cast<int>(gid), &w, &h, &xo, &yo);
            const int rect = atlas.AddCustomRectFontGlyph(font, glyph_codepoint(gid),
                w > 0 ? w : 1, h > 0 ? h : 1, 0, ImVec2(static_cast<float>(xo), baseline + yo));
            if (bitmap) {
                // Pack immediately into a private staging buffer; atlas pixels do not exist until Build().
                bitmaps.emplace_back(bitmap, w, h, rect);
            }
        }
        atlas.Build();
        unsigned char* pixels = nullptr; int width = 0, height = 0;
        atlas.GetTexDataAsAlpha8(&pixels, &width, &height);
        if (!pixels) throw Error("Font atlas build failed");
        for (const auto& b : bitmaps) {
            const auto* r = atlas.GetCustomRectByIndex(b.rect);
            if (!r->IsPacked()) throw Error("Font atlas overflow");
            for (int y = 0; y < b.height; ++y)
                std::copy_n(b.data.get() + y * b.width, b.width,
                    pixels + (r->Y + y) * width + r->X);
            stbtt_FreeBitmap(b.data.get(), nullptr);
        }
        bitmaps.clear();
        // Keep alpha pixels alive: the DX11 backend uploads them after this constructor.
    }
    struct Bitmap {
        Bitmap(unsigned char* p, int w, int h, int r) : data(p, stbtt_FreeBitmap), width(w), height(h), rect(r) {}
        std::unique_ptr<unsigned char, decltype(&stbtt_FreeBitmap)> data;
        int width, height, rect;
    };
    std::vector<Bitmap> bitmaps;
    ~Impl() {
        if (hb_font) hb_font_destroy(hb_font);
        if (face) hb_face_destroy(face);
        if (blob) hb_blob_destroy(blob);
    }
    const Shape& shape(const std::string& text, bool rtl) {
        const std::string key = (rtl ? "1:" : "0:") + text;
        if (auto it = cache.find(key); it != cache.end()) return it->second;
        if (text.size() > 4096) throw Error("UI label exceeds 4096 bytes");
        const auto wide = utf8_to_wide(text);
        Shape result;
        if (!wide.empty()) {
            std::vector<SCRIPT_ITEM> items(wide.size() + 1);
            SCRIPT_CONTROL control{}; control.uDefaultDirection = rtl ? 1 : 0;
            SCRIPT_STATE state{}; state.uBidiLevel = rtl ? 1 : 0;
            int count = 0;
            check_hr(ScriptItemize(wide.c_str(), static_cast<int>(wide.size()), static_cast<int>(items.size()) - 1,
                                   &control, &state, items.data(), &count), "ScriptItemize");
            std::vector<unsigned char> levels(static_cast<size_t>(count));
            std::vector<int> visual(static_cast<size_t>(count));
            for (int i = 0; i < count; ++i) levels[static_cast<size_t>(i)] = items[i].a.s.uBidiLevel;
            check_hr(ScriptLayout(count, levels.data(), visual.data(), nullptr), "ScriptLayout");
            for (int vi = 0; vi < count; ++vi) {
                const int i = visual[vi];
                const int begin = items[i].iCharPos;
                const int length = items[i + 1].iCharPos - begin;
                if (length <= 0) continue;
                hb_buffer_t* buffer = hb_buffer_create();
                if (!hb_buffer_allocation_successful(buffer)) { hb_buffer_destroy(buffer); throw Error("HarfBuzz allocation failed"); }
                hb_buffer_add_utf16(buffer, reinterpret_cast<const uint16_t*>(wide.data() + begin), length, 0, length);
                hb_buffer_set_direction(buffer, (levels[i] & 1) ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
                hb_buffer_guess_segment_properties(buffer);
                hb_shape(hb_font, buffer, nullptr, 0);
                unsigned glyphs = 0;
                auto* infos = hb_buffer_get_glyph_infos(buffer, &glyphs);
                auto* positions = hb_buffer_get_glyph_positions(buffer, &glyphs);
                for (unsigned g = 0; g < glyphs; ++g) {
                    result.glyphs.push_back({infos[g].codepoint,
                        result.width + positions[g].x_offset / 64.0f, -positions[g].y_offset / 64.0f,
                        positions[g].x_advance / 64.0f});
                    result.width += positions[g].x_advance / 64.0f;
                }
                hb_buffer_destroy(buffer);
            }
        }
        if (cache.size() >= 512) cache.clear();
        return cache.emplace(key, std::move(result)).first->second;
    }
    void draw(const Shape& shaped, ImVec2 pos, ImU32 color) {
        auto* list = ImGui::GetWindowDrawList();
        for (const auto& g : shaped.glyphs) {
            const auto* glyph = font->FindGlyphNoFallback(glyph_codepoint(g.gid));
            if (!glyph || glyph->X1 <= glyph->X0 || glyph->Y1 <= glyph->Y0) continue;
            const ImVec2 p(pos.x + g.x + glyph->X0, pos.y + g.y + glyph->Y0);
            list->AddImageQuad(atlas.TexID, p, ImVec2(p.x + glyph->X1 - glyph->X0, p.y),
                ImVec2(p.x + glyph->X1 - glyph->X0, p.y + glyph->Y1 - glyph->Y0),
                ImVec2(p.x, p.y + glyph->Y1 - glyph->Y0),
                ImVec2(glyph->U0, glyph->V0), ImVec2(glyph->U1, glyph->V0),
                ImVec2(glyph->U1, glyph->V1), ImVec2(glyph->U0, glyph->V1), color);
        }
    }
};
TextEngine::TextEngine(ImFontAtlas& atlas, const std::filesystem::path& font, float size)
    : impl_(std::make_unique<Impl>(atlas, font, size)) {}
TextEngine::~TextEngine() = default;
void TextEngine::text(const std::string& utf8, bool rtl, ImU32 color) {
    const auto& s = impl_->shape(utf8, rtl);
    auto p = ImGui::GetCursorScreenPos();
    if (rtl) p.x += (std::max)(0.0f, ImGui::GetContentRegionAvail().x - s.width);
    impl_->draw(s, p, color);
    ImGui::Dummy(ImVec2(s.width, line_height()));
}
void TextEngine::text_at(const std::string& utf8, bool rtl, ImVec2 p, ImU32 color) { impl_->draw(impl_->shape(utf8,rtl),p,color); }
ImVec2 TextEngine::measure(const std::string& utf8, bool rtl) { return {impl_->shape(utf8,rtl).width,line_height()}; }
float TextEngine::line_height() const { return impl_->font->FontSize; }
const std::filesystem::path& TextEngine::font_path() const { return impl_->path; }
} // namespace superior
