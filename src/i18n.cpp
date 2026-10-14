#include "blockwater/i18n.hpp"

#include <fstream>
#include <stdexcept>

namespace blockwater {
namespace {

std::string_view trim(std::string_view value) noexcept {
    constexpr std::string_view spaces = " \t\r";
    const auto first = value.find_first_not_of(spaces);
    if (first == std::string_view::npos) return {};
    const auto last = value.find_last_not_of(spaces);
    return value.substr(first, last - first + 1);
}

} // namespace

Catalog::Entries Catalog::read_catalog(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) throw std::runtime_error("cannot open language catalog");
    Entries entries;
    std::string line;
    while (std::getline(stream, line)) {
        const std::string_view value = trim(line);
        if (value.empty() || value.front() == '#') continue;
        const auto equal = value.find('=');
        if (equal == std::string_view::npos) throw std::runtime_error("invalid language catalog line");
        const auto key = trim(value.substr(0, equal));
        const auto message = trim(value.substr(equal + 1));
        if (key.empty() || message.empty() || !entries.emplace(std::string(key), std::string(message)).second)
            throw std::runtime_error("invalid or duplicate language catalog key");
    }
    if (!stream.eof() || entries.empty()) throw std::runtime_error("cannot read language catalog");
    return entries;
}

Catalog::Catalog(const std::filesystem::path& directory, std::string_view requested_locale)
    : locale_(requested_locale == "zh" || requested_locale == "zh-CN" ? "zh-CN" : "en-US"),
      fallback_(read_catalog(directory / "en-US.lang")) {
    if (locale_ != "en-US") selected_ = read_catalog(directory / (locale_ + ".lang"));
}

std::string_view Catalog::get(std::string_view key) const noexcept {
    if (const auto it = selected_.find(key); it != selected_.end()) return it->second;
    if (const auto it = fallback_.find(key); it != fallback_.end()) return it->second;
    return key;
}

} // namespace blockwater
