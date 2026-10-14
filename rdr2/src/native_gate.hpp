#pragma once

#include "blockwater/host_session.hpp"
#include "scripthook.hpp"
#include <cstdint>
#include <cstring>
#include <optional>
#include <initializer_list>

namespace blockwater::rdr2 {

// Read-only natives, recorded with signatures and provenance in docs/rdr2-host.md.
template <typename... Args>
std::optional<std::int32_t> query(std::uint64_t hash, Args... args) noexcept {
    nativeInit(hash);
    (nativePush64(static_cast<std::uint32_t>(args)), ...);
    const auto result = nativeCall();
    if (!result) return std::nullopt;
    std::int32_t value = 0;
    std::memcpy(&value, result, sizeof(value));
    return value;
}

inline HostContext read_context() noexcept {
    bool network = false;
    bool unavailable = false;
    for (const auto hash : {0xCA97246103B63917ULL, 0x9DE624D2FC4B603FULL,
                            0x10FAB35428CCC9D7ULL}) {
        const auto value = query(hash);
        network = network || (value && *value != 0);
        unavailable = unavailable || !value;
    }
    if (network) return HostContext::Multiplayer;
    if (unavailable) return HostContext::Unavailable;
    const auto paused = query(0x535384D6067BA42EULL);
    if (!paused) return HostContext::Unavailable;
    if (*paused) return HostContext::Paused;
    const auto ped = query(0x096275889B8E0EE0ULL);
    if (!ped || *ped == 0) return HostContext::Unavailable;
    const auto exists = query(0xD42BD6EB2E0F1677ULL, *ped);
    if (!exists || !*exists) return HostContext::Unavailable;
    const auto dead = query(0x7D5B1F88E7504BBAULL, *ped);
    if (!dead || *dead) return HostContext::Unavailable;
    const auto player = query(0x217E9DC48139933DULL);
    if (!player) return HostContext::Unavailable;
    const auto controlled = query(0x7964097FCE4C244BULL, *player);
    return controlled && *controlled ? HostContext::StoryMode : HostContext::Unavailable;
}

} // namespace blockwater::rdr2
