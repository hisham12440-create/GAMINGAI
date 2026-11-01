#include "superior/core/hotkey.h"
#include <windows.h>
#include <algorithm>
#include <sstream>
#include <vector>
namespace superior {
ParsedHotkey parse_hotkey(const std::string& input) {
    if (input.empty() || input.size() > 80) throw std::runtime_error("Hotkey must contain 1..80 characters");
    ParsedHotkey result;
    std::stringstream stream(input); std::string part;
    bool has_key = false;
    while (std::getline(stream, part, '+')) {
        part.erase(part.begin(), std::find_if(part.begin(), part.end(), [](unsigned char c) { return !std::isspace(c); }));
        part.erase(std::find_if(part.rbegin(), part.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), part.end());
        std::transform(part.begin(), part.end(), part.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        if (part.empty()) throw std::runtime_error("Empty hotkey component");
        unsigned mod = 0;
        if (part == "CTRL" || part == "CONTROL") mod = MOD_CONTROL;
        else if (part == "ALT") mod = MOD_ALT;
        else if (part == "SHIFT") mod = MOD_SHIFT;
        else if (part == "WIN") mod = MOD_WIN;
        if (mod) {
            if (result.modifiers & mod) throw std::runtime_error("Duplicate modifier");
            result.modifiers |= mod; continue;
        }
        if (has_key) throw std::runtime_error("Only one non-modifier key is allowed");
        has_key = true;
        if (part.size() == 1 && ((part[0] >= 'A' && part[0] <= 'Z') || (part[0] >= '0' && part[0] <= '9')))
            result.key = static_cast<unsigned char>(part[0]);
        else if (part.size() >= 2 && part[0] == 'F' && part.find_first_not_of("0123456789", 1) == std::string::npos) {
            const auto digits=part.substr(1);
            if(digits.size()>2)throw std::runtime_error("Function key must be F1..F24");
            const auto n=std::stoul(digits);
            if (n < 1 || n > 24) throw std::runtime_error("Function key must be F1..F24");
            result.key = VK_F1 + n - 1;
        } else {
            const std::pair<const char*, unsigned> named[] = {{"SPACE",VK_SPACE},{"TAB",VK_TAB},
                {"INSERT",VK_INSERT},{"DELETE",VK_DELETE},{"HOME",VK_HOME},{"END",VK_END},
                {"PAGEUP",VK_PRIOR},{"PAGEDOWN",VK_NEXT},{"UP",VK_UP},{"DOWN",VK_DOWN},{"LEFT",VK_LEFT},{"RIGHT",VK_RIGHT}};
            for (const auto& [name, key] : named) if (part == name) result.key = key;
            if (!result.key) throw std::runtime_error("Unsupported key: " + part);
        }
    }
    if (!has_key) throw std::runtime_error("Hotkey needs a key");
    if (!(result.modifiers & (MOD_CONTROL | MOD_ALT | MOD_WIN)) && !(result.key >= VK_F1 && result.key <= VK_F24))
        throw std::runtime_error("Use Ctrl, Alt or Win, or a function key, to avoid stealing game input");
    return result;
}
std::string canonical_hotkey(const ParsedHotkey& k) {
    return std::to_string(k.modifiers) + ":" + std::to_string(k.key);
}
} // namespace superior
