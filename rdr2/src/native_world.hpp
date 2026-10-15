#pragma once

#include "scripthook.hpp"
#include "native_values.hpp"
#include "blockwater/link.hpp"
#include "blockwater/world_stream.hpp"
#include <limits>

namespace blockwater::rdr2 {

template <typename... Args>
const std::uint64_t* invoke_slots(std::uint64_t hash, Args... args) noexcept {
    nativeInit(hash);
    (nativePush64(native_argument(args)), ...);
    return nativeCall();
}

template <typename T, typename... Args>
std::optional<T> invoke_value(std::uint64_t hash, Args... args) noexcept {
    return native_result<T>(invoke_slots(hash, args...));
}

template <typename... Args>
std::optional<Vec3> invoke_vector(std::uint64_t hash, Args... args) noexcept {
    return native_vector(invoke_slots(hash, args...));
}

// Read-only native signatures are recorded in docs/rdr2-world.md.
class NativeWorld final : public WorldSource {
public:
    std::uint64_t clock_us() const noexcept override { return now_us(); }
    std::optional<WorldView> view() noexcept override {
        const auto ped = invoke_value<std::int32_t>(0x096275889B8E0EE0ULL);
        const auto camera = invoke_vector(0x5352E025EC2B416FULL);
        const auto rotation = invoke_vector(0x602685BD85DD26CAULL, 2);
        const auto fov = invoke_value<float>(0x04AF77971E508F6AULL);
        if (!ped || *ped == 0 || !camera || !rotation || !fov) return std::nullopt;
        const auto model = invoke_value<std::uint32_t>(0xDA76A9F39210D365ULL, *ped);
        if (!model || *model == 0) return std::nullopt;
        const auto nan = std::numeric_limits<float>::quiet_NaN();
        NativeVector minimum{nan, 0, nan, 0, nan, 0}, maximum = minimum;
        // Void natives may return no result pointer: validate their output parameters instead.
        invoke_slots(0xDCB8DDD5D054A7E7ULL, *model, &minimum, &maximum);
        if (!finite(minimum.value()) || !finite(maximum.value()) || minimum.x > maximum.x ||
            minimum.y > maximum.y || minimum.z >= maximum.z || maximum.z-minimum.z > 20) return std::nullopt;
        const auto feet = invoke_vector(0x1899F328B0E12848ULL, *ped, 0.0f, 0.0f, minimum.z);
        if (!feet) return std::nullopt;
        WorldView result{};
        result.player = *ped;
        result.feet = *feet;
        result.camera = {*camera, static_cast<float>(rotation->z), static_cast<float>(rotation->x),
                         static_cast<float>(rotation->y), *fov};
        return result;
    }
    std::int32_t begin(const GroundProbe& p) noexcept override {
        // World geometry only; exclude the player's own entity and dynamic proxies.
        return invoke_value<std::int32_t>(0x7EE9F5D83DD4F90EULL,
            static_cast<float>(p.from.x), static_cast<float>(p.from.y), static_cast<float>(p.from.z),
            static_cast<float>(p.to.x), static_cast<float>(p.to.y), static_cast<float>(p.to.z),
            1, p.ignore, 4).value_or(0);
    }
    ProbeResult poll(std::int32_t handle) noexcept override {
        const auto nan = std::numeric_limits<float>::quiet_NaN();
        NativeVector point{nan, 0, nan, 0, nan, 0}, normal = point;
        std::int32_t hit = -1, entity = 0;
        const auto status = invoke_value<std::int32_t>(0xEDE8AC7C5108FB1DULL,
            handle, &hit, &point, &normal, &entity);
        // Unknown status retains ownership: never leak handles by issuing unbounded replacements.
        if (!status || *status == 1 || (*status != 0 && *status != 2)) return {};
        if (*status == 0) return {ProbeState::Invalid, {}};
        if (hit == 0) return {ProbeState::Miss, {}};
        if (hit != 1) return {ProbeState::Invalid, {}};
        return {ProbeState::Hit, {point.value(), normal.x, normal.y, normal.z, 1}};
    }
};

} // namespace blockwater::rdr2
