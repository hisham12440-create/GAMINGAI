#pragma once
#include <cstdint>
#include <string>
namespace superior {
struct ParsedHotkey { unsigned modifiers = 0; unsigned key = 0; };
ParsedHotkey parse_hotkey(const std::string& text);
std::string canonical_hotkey(const ParsedHotkey& key);
} // namespace superior
