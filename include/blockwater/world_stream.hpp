#pragma once

#include "blockwater/wire.hpp"
#include <array>
#include <optional>

namespace blockwater {

struct WorldView {
    Pose camera{};
    Vec3 feet{};
    std::int32_t player = 0;
};

struct GroundProbe {
    Vec3 from{};
    Vec3 to{};
    std::int32_t ignore = 0;
};

enum class ProbeState { Pending, Miss, Hit, Invalid };
struct ProbeResult {
    ProbeState state = ProbeState::Pending;
    TerrainSample sample{};
};

// All methods run on the owning script thread. Probe handles never enter the wire format.
class WorldSource {
public:
    virtual ~WorldSource() = default;
    virtual std::uint64_t clock_us() const noexcept = 0;
    virtual std::optional<WorldView> view() noexcept = 0;
    virtual std::int32_t begin(const GroundProbe&) noexcept = 0;
    virtual ProbeResult poll(std::int32_t handle) noexcept = 0;
};

class WorldStream {
public:
    static constexpr std::size_t kPendingMax = 8;
    static constexpr std::size_t kGridWidth = 9;
    static constexpr std::size_t kCells = kGridWidth * kGridWidth;
    static constexpr std::uint64_t kBudgetUs = 500;
    static constexpr std::uint64_t kProbeTimeoutUs = 250'000;
    static constexpr std::uint64_t kSampleLifetimeUs = 500'000;
    static constexpr double kSpacing = 2;
    static_assert(kCells <= kTerrainMax);

    void reset() noexcept;
    bool collect(WorldSource&, std::uint64_t epoch, HostFrame& output) noexcept;
    std::size_t pending_count() const noexcept;
private:
    struct Cell {
        TerrainSample sample{};
        std::uint64_t sampled_at = 0;
        bool valid = false;
    };
    struct Pending {
        std::int32_t handle = 0;
        std::size_t cell = 0;
        GroundProbe probe{};
        std::uint64_t started_at = 0;
        bool discarded = false;
        bool high = false;
    };
    void clear_grid() noexcept;
    bool reanchor(std::uint64_t epoch, const Vec3& feet) noexcept;
    std::array<Cell, kCells> cells_{};
    std::array<Pending, kPendingMax> pending_{};
    std::array<bool, kCells> high_{};
    Vec3 origin_{}, previous_{};
    double centre_x_ = 0, centre_y_ = 0, reference_z_ = 0;
    std::uint64_t epoch_ = 0, anchor_ = 0, frame_ = 0, last_us_ = 0;
    std::size_t cursor_ = 0;
    bool active_ = false;
};

} // namespace blockwater
