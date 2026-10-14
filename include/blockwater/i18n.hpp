#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <string_view>

namespace blockwater {

// Catalogs are loaded during startup and never read from disk on the game frame loop.
// en-US is the fallback locale for unsupported languages and missing keys.
class Catalog {
public:
    Catalog(const std::filesystem::path& directory, std::string_view requested_locale);

    std::string_view locale() const noexcept { return locale_; }
    std::string_view get(std::string_view key) const noexcept;

private:
    using Entries = std::map<std::string, std::string, std::less<>>;
    static Entries read_catalog(const std::filesystem::path& path);

    std::string locale_;
    Entries fallback_;
    Entries selected_;
};

} // namespace blockwater
