#pragma once

#include "blockwater/wire.hpp"
#include <cstring>
#include <optional>
#include <type_traits>

namespace blockwater::rdr2 {

struct NativeVector {
    float x = 0;
    std::uint32_t pad_x = 0;
    float y = 0;
    std::uint32_t pad_y = 0;
    float z = 0;
    std::uint32_t pad_z = 0;
    Vec3 value() const noexcept { return {x, y, z}; }
};
static_assert(sizeof(NativeVector) == 24);
static_assert(offsetof(NativeVector, y) == 8 && offsetof(NativeVector, z) == 16);

template <typename T>
std::uint64_t native_argument(T value) noexcept {
    static_assert(std::is_arithmetic_v<T> || std::is_pointer_v<T>);
    static_assert(sizeof(T) <= sizeof(std::uint64_t));
    std::uint64_t slot = 0;
    if constexpr (std::is_same_v<T, bool>) {
        slot = value ? 1 : 0;
    } else {
        std::memcpy(&slot, &value, sizeof(value));
    }
    return slot;
}

template <typename T>
std::optional<T> native_result(const std::uint64_t* slots) noexcept {
    static_assert(std::is_arithmetic_v<T> && sizeof(T) <= sizeof(std::uint64_t));
    if (!slots) return std::nullopt;
    T result{};
    std::memcpy(&result, slots, sizeof(result));
    return result;
}

inline std::optional<Vec3> native_vector(const std::uint64_t* slots) noexcept {
    if (!slots) return std::nullopt;
    NativeVector result{};
    std::memcpy(&result.x, slots, sizeof(float));
    std::memcpy(&result.y, slots+1, sizeof(float));
    std::memcpy(&result.z, slots+2, sizeof(float));
    return result.value();
}

} // namespace blockwater::rdr2
