#pragma once

#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace blockwater {

inline constexpr std::uint32_t kMagic = 0x31525742; // BWR1 in little-endian
inline constexpr std::uint32_t kVersion = 1;
inline constexpr std::size_t kTerrainMax = 96;
inline constexpr std::size_t kEntityMax = 48;
inline constexpr std::size_t kEventCapacity = 128;
inline constexpr std::uint64_t kHeartbeatTimeoutUs = 2'000'000;
inline constexpr std::uint32_t kStoryMode = 1;
inline constexpr std::uint32_t kGuestPlayerValid = 1;

struct Vec3 {
    double x = 0, y = 0, z = 0;
};

struct Pose {
    Vec3 position;
    float yaw = 0, pitch = 0, roll = 0, fov = 70;
};

struct TerrainSample {
    Vec3 point;
    float nx = 0, ny = 0, nz = 1;
    std::uint32_t flags = 0;
};

struct HostEntity {
    std::uint64_t id = 0;
    Vec3 position;
    Vec3 velocity;
    std::uint32_t type = 0;
    float health = 0;
};

struct HostFrame {
    std::uint64_t frame_id = 0;
    std::uint64_t timestamp_us = 0;
    std::uint64_t anchor = 0;
    std::uint32_t flags = 0;
    std::uint32_t terrain_count = 0;
    std::uint32_t entity_count = 0;
    std::uint32_t reserved = 0;
    Pose camera;
    Vec3 player_feet;
    Vec3 host_origin;
    Vec3 guest_origin;
    double metres_per_block = 1;
    std::array<TerrainSample, kTerrainMax> terrain{};
    std::array<HostEntity, kEntityMax> entities{};
};

struct GuestFrame {
    std::uint64_t frame_id = 0;
    std::uint64_t timestamp_us = 0;
    std::uint64_t anchor = 0;
    std::uint32_t flags = 0;
    std::uint32_t reserved = 0;
    Pose camera;
    Vec3 player_feet;
    Vec3 velocity;
};

enum class EventKind : std::uint32_t {
    PlaceBlock = 1,
    RemoveBlock = 2,
    Explosion = 3,
    DamageEntity = 4,
    HostDamage = 5,
    Rejected = 6,
};

struct Event {
    std::uint64_t id = 0;
    std::uint64_t timestamp_us = 0;
    std::uint64_t sender_epoch = 0;
    std::uint64_t recipient_epoch = 0;
    EventKind kind = EventKind::PlaceBlock;
    std::uint32_t payload = 0;
    Vec3 position;
    std::int32_t block_x = 0, block_y = 0, block_z = 0;
    float magnitude = 0;
    std::uint64_t target = 0;
};

static_assert(std::is_trivially_copyable_v<HostFrame>);
static_assert(std::is_trivially_copyable_v<GuestFrame>);
static_assert(std::is_trivially_copyable_v<Event>);
static_assert(std::atomic_ref<std::uint64_t>::is_always_lock_free);
static_assert(sizeof(HostFrame) < 16 * 1024);

// Each payload word is atomic. The surrounding sequence counter detects a
// partial snapshot without introducing C++ data races on the payload itself.
template <typename T>
struct alignas(64) AtomicSnapshot {
    static constexpr std::size_t kWords = (sizeof(T) + 7) / 8;
    alignas(64) std::uint64_t sequence = 0;
    alignas(8) std::array<std::uint64_t, kWords> words{};

    void publish(const T& value) {
        std::array<std::uint64_t, kWords> copy{};
        std::memcpy(copy.data(), &value, sizeof(T));
        auto seq = std::atomic_ref<std::uint64_t>(sequence);
        const auto current = seq.load(std::memory_order_seq_cst);
        seq.store(current + 1, std::memory_order_seq_cst);
        for (std::size_t i = 0; i < kWords; ++i) {
            std::atomic_ref<std::uint64_t>(words[i]).store(copy[i], std::memory_order_seq_cst);
        }
        seq.store(current + 2, std::memory_order_seq_cst);
    }

    bool read(T& output, unsigned attempts = 8) const {
        std::array<std::uint64_t, kWords> copy{};
        auto& mutable_seq = const_cast<std::uint64_t&>(sequence);
        auto seq = std::atomic_ref<std::uint64_t>(mutable_seq);
        for (unsigned attempt = 0; attempt < attempts; ++attempt) {
            const auto before = seq.load(std::memory_order_seq_cst);
            if (before & 1) continue;
            for (std::size_t i = 0; i < kWords; ++i) {
                auto& word = const_cast<std::uint64_t&>(words[i]);
                copy[i] = std::atomic_ref<std::uint64_t>(word).load(std::memory_order_seq_cst);
            }
            const auto after = seq.load(std::memory_order_seq_cst);
            if (before != after) continue;
            T candidate{};
            std::memcpy(static_cast<void*>(&candidate), copy.data(), sizeof(T));
            output = candidate;
            return true;
        }
        return false;
    }
};

// Single producer, single consumer. The producer cannot overwrite an unread
// event, and the consumer only acknowledges a fully copied event.
// ABI v1 keeps the counters on distinct 64-byte cache lines.
struct alignas(64) EventRing {
    alignas(64) std::uint64_t produced = 0;
    std::array<std::byte, 56> reserved_produced{};
    alignas(64) std::uint64_t consumed = 0;
    std::array<std::byte, 56> reserved_consumed{};
    using Words = std::array<std::uint64_t, (sizeof(Event) + 7) / 8>;
    alignas(64) std::array<Words, kEventCapacity> entries{};

    bool push(const Event& event) {
        auto write = std::atomic_ref<std::uint64_t>(produced);
        auto read = std::atomic_ref<std::uint64_t>(consumed);
        const auto w = write.load(std::memory_order_seq_cst);
        const auto r = read.load(std::memory_order_seq_cst);
        if (r > w || w - r >= kEventCapacity) return false;
        Words data{};
        std::memcpy(data.data(), &event, sizeof(event));
        auto& entry = entries[w % kEventCapacity];
        for (std::size_t i = 0; i < data.size(); ++i)
            std::atomic_ref<std::uint64_t>(entry[i]).store(data[i], std::memory_order_seq_cst);
        write.store(w + 1, std::memory_order_seq_cst);
        return true;
    }

    bool pop(Event& event) {
        auto write = std::atomic_ref<std::uint64_t>(produced);
        auto read = std::atomic_ref<std::uint64_t>(consumed);
        const auto r = read.load(std::memory_order_seq_cst);
        if (r >= write.load(std::memory_order_seq_cst)) return false;
        Words data{};
        auto& entry = entries[r % kEventCapacity];
        for (std::size_t i = 0; i < data.size(); ++i)
            data[i] = std::atomic_ref<std::uint64_t>(entry[i]).load(std::memory_order_seq_cst);
        std::memcpy(static_cast<void*>(&event), data.data(), sizeof(event));
        read.store(r + 1, std::memory_order_seq_cst);
        return true;
    }
};

enum class PeerState : std::uint64_t { Offline = 0, Ready = 1, Refused = 2, Closed = 3 };
enum class RejectReason : std::uint64_t { None = 0, Multiplayer = 1, Protocol = 2 };

struct alignas(64) PeerControl {
    alignas(64) std::uint64_t epoch = 0;
    std::array<std::byte, 56> reserved_epoch{};
    alignas(64) std::uint64_t heartbeat_us = 0;
    std::array<std::byte, 56> reserved_heartbeat{};
    alignas(64) std::uint64_t state = 0;
    std::array<std::byte, 56> reserved_state{};
    alignas(64) std::uint64_t reason = 0;
    std::array<std::byte, 56> reserved_reason{};
};

static_assert(std::is_standard_layout_v<EventRing>);
static_assert(alignof(EventRing) == 64);
static_assert(offsetof(EventRing, produced) == 0);
static_assert(offsetof(EventRing, consumed) == 64);
static_assert(offsetof(EventRing, entries) == 128);
static_assert(sizeof(EventRing) == 128 + sizeof(EventRing::Words) * kEventCapacity);
static_assert(std::is_standard_layout_v<PeerControl>);
static_assert(alignof(PeerControl) == 64);
static_assert(offsetof(PeerControl, epoch) == 0);
static_assert(offsetof(PeerControl, heartbeat_us) == 64);
static_assert(offsetof(PeerControl, state) == 128);
static_assert(offsetof(PeerControl, reason) == 192);
static_assert(sizeof(PeerControl) == 256);

struct alignas(64) SharedBridge {
    alignas(64) std::uint64_t signature = 0;
    std::uint64_t size_bytes = 0;
    PeerControl host;
    PeerControl guest;
    AtomicSnapshot<HostFrame> host_frame;
    AtomicSnapshot<GuestFrame> guest_frame;
    EventRing to_host;
    EventRing to_guest;
};

inline constexpr std::uint64_t kSignature = (std::uint64_t(kVersion) << 32) | kMagic;

inline bool finite(const Vec3& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
inline bool valid(const Pose& pose) {
    return finite(pose.position) && std::isfinite(pose.fov) && pose.fov >= 1 && pose.fov <= 170 &&
        std::isfinite(pose.yaw) && std::isfinite(pose.pitch) && std::isfinite(pose.roll);
}
inline bool valid(const HostFrame& frame) {
    if (!(frame.flags & kStoryMode) || frame.terrain_count > kTerrainMax ||
        frame.entity_count > kEntityMax || !valid(frame.camera) || !finite(frame.player_feet) ||
        !finite(frame.host_origin) || !finite(frame.guest_origin) || !std::isfinite(frame.metres_per_block) ||
        frame.metres_per_block <= 0) return false;
    for (std::uint32_t i = 0; i < frame.terrain_count; ++i)
        if (!finite(frame.terrain[i].point)) return false;
    for (std::uint32_t i = 0; i < frame.entity_count; ++i)
        if (!finite(frame.entities[i].position) || !finite(frame.entities[i].velocity) ||
            !std::isfinite(frame.entities[i].health)) return false;
    return true;
}
inline bool valid(const GuestFrame& frame) {
    return valid(frame.camera) && finite(frame.player_feet) && finite(frame.velocity);
}
inline bool valid(const Event& e) {
    if (e.id == 0 || !finite(e.position) || !std::isfinite(e.magnitude)) return false;
    return e.kind >= EventKind::PlaceBlock && e.kind <= EventKind::Rejected;
}

} // namespace blockwater
