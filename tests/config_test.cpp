#include "superior/core/config.h"
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
using namespace superior;
TEST(Config, DefaultsAreValidAndRoundTrip) {
    Settings s;EXPECT_TRUE(s.validate().empty());
    const auto restored=parse_settings(serialize_settings(s));
    EXPECT_EQ(restored.language,Language::English);EXPECT_TRUE(restored.click_through);EXPECT_TRUE(restored.overlay_enabled);
}
TEST(Config, ArabicAndHotkeysRoundTrip) {
    Settings s;s.language=Language::Arabic;s.click_through=false;s.hotkeys.show_ui="Ctrl+Shift+F9";
    const auto restored=parse_settings(serialize_settings(s));EXPECT_EQ(restored.language,Language::Arabic);
    EXPECT_FALSE(restored.click_through);EXPECT_EQ(restored.hotkeys.show_ui,"Ctrl+Shift+F9");
}
TEST(Config, RejectsMalformedAndWrongTypes) {
    for(const auto* input:{"{",R"({"click_through":"yes"})",R"({"schema_version":true})",R"({"language":1})",R"({"hotkeys":[]})", "[]"})
        EXPECT_THROW(parse_settings(input),std::exception) << input;
}
TEST(Config, RejectsUnknownSchemaLanguageAndDuplicateHotkeys) {
    EXPECT_THROW(parse_settings(R"({"schema_version":2})"),std::runtime_error);
    EXPECT_THROW(parse_settings(R"({"language":"fr"})"),std::runtime_error);
    Settings s;s.hotkeys.toggle_overlay=s.hotkeys.show_ui;EXPECT_FALSE(s.validate().empty());
    EXPECT_THROW(serialize_settings(s),std::runtime_error);
}
TEST(Config, RejectsOversizedDocument) {EXPECT_THROW(parse_settings(std::string(65537,' ')),std::runtime_error);}
TEST(Config, MissingFileUsesDefaultsWithoutNotice) {
    const auto dir=std::filesystem::temp_directory_path()/L"superior-config-missing-test";
    std::filesystem::remove_all(dir);const auto result=load_settings(dir/L"settings.json");
    EXPECT_TRUE(result.notice.empty());EXPECT_TRUE(result.settings.validate().empty());
}
TEST(Config, AtomicSaveBackupAndCorruptRecovery) {
    const auto dir=std::filesystem::temp_directory_path()/L"superior-config-roundtrip-test";
    std::filesystem::remove_all(dir);const auto path=dir/L"settings.json";
    Settings s;s.language=Language::Arabic;save_settings(path,s);
    s.language=Language::English;save_settings(path,s);
    {std::ofstream f(path);f<<"corrupt";}
    const auto loaded=load_settings(path);EXPECT_EQ(loaded.settings.language,Language::Arabic);EXPECT_FALSE(loaded.notice.empty());
    EXPECT_TRUE(std::filesystem::exists(path));EXPECT_FALSE(std::filesystem::exists(path.string()+L".tmp"));
    std::filesystem::remove_all(dir);
}
TEST(Config, BothInvalidCopiesUseDefaultsAndPreserveFiles) {
    const auto dir=std::filesystem::temp_directory_path()/L"superior-config-invalid-test";
    std::filesystem::create_directories(dir);const auto path=dir/L"settings.json";
    {std::ofstream f(path);f<<"bad";}{std::ofstream f(path.string()+L".bak");f<<"bad";}
    const auto loaded=load_settings(path);EXPECT_FALSE(loaded.notice.empty());EXPECT_TRUE(loaded.settings.validate().empty());
    std::ifstream f(path);std::string content;f>>content;EXPECT_EQ(content,"bad");std::filesystem::remove_all(dir);
}
