#include "superior/core/hotkey.h"
#include <gtest/gtest.h>
#include <windows.h>
using namespace superior;
TEST(Hotkey, ParsesAndNormalizesChord) {
    const auto a=parse_hotkey(" ctrl + alt + u ");EXPECT_EQ(a.modifiers,MOD_CONTROL|MOD_ALT);EXPECT_EQ(a.key,'U');
    EXPECT_EQ(canonical_hotkey(a),canonical_hotkey(parse_hotkey("Alt+Ctrl+U")));
}
TEST(Hotkey, FunctionAndNamedKeys) {
    EXPECT_EQ(parse_hotkey("F12").key,VK_F12);EXPECT_EQ(parse_hotkey("Ctrl+PageDown").key,VK_NEXT);
    EXPECT_EQ(parse_hotkey("Win+Shift+Space").key,VK_SPACE);
}
TEST(Hotkey, RejectsAmbiguousOrGameInputChords) {
    for(const auto* input:{"", "U", "Shift+U", "Ctrl", "Ctrl++U", "Ctrl+U+K", "Ctrl+Ctrl+U", "F0", "F25", "Ctrl+Unknown", "Ctrl+F99999999999999999999"})
        EXPECT_THROW(parse_hotkey(input),std::runtime_error) << input;
}
