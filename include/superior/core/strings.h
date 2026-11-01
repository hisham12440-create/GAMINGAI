#pragma once
#include <string>
namespace superior {
std::wstring utf8_to_wide(const std::string& text);
std::string wide_to_utf8(const std::wstring& text);
} // namespace superior
