#include "blockwater/i18n.hpp"
#include "test.hpp"
#include <filesystem>
#include <string_view>

#ifndef BLOCKWATER_LOCALES_DIR
#error BLOCKWATER_LOCALES_DIR must be defined for tests
#endif

using namespace blockwater;

void test_i18n() {
    const auto dir = std::filesystem::path(BLOCKWATER_LOCALES_DIR);
    const Catalog english(dir, "en-US");
    const Catalog chinese(dir, "zh-CN");
    const Catalog chinese_alias(dir, "zh");
    const Catalog unsupported(dir, "de-DE");
    REQUIRE(english.locale() == "en-US");
    REQUIRE(chinese.locale() == "zh-CN");
    REQUIRE(chinese_alias.locale() == "zh-CN");
    REQUIRE(unsupported.locale() == "en-US");
    REQUIRE(english.get("bridge.connected") == "Minecraft connected");
    REQUIRE(chinese.get("bridge.connected") == "已连接 Minecraft");
    REQUIRE(chinese_alias.get("bridge.multiplayer_refused") == "仅支持《荒野大镖客 2》故事模式");
    REQUIRE(unsupported.get("bridge.connected") == english.get("bridge.connected"));
    REQUIRE(chinese.get("not.registered") == "not.registered");
    bool missing = false;
    try { (void)Catalog(dir / "missing", "en-US"); }
    catch (const std::runtime_error&) { missing = true; }
    REQUIRE(missing);
}
