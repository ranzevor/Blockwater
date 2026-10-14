#pragma once

#include "blockwater/link.hpp"
#include "blockwater/coordinates.hpp"
#include <span>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <unordered_map>
#include <unordered_set>

namespace blockwater {

// Implement this interface using verified ScriptHookRDR2 natives. Every method
// is called on the game's script thread, never from an IPC worker thread.
class GameHost {
public:
    virtual ~GameHost() = default;
    virtual bool story_mode() const = 0;
    virtual Pose camera() const = 0;
    virtual Vec3 player_feet() const = 0;
    virtual bool sample_terrain(double x, double y, TerrainSample& out) = 0;
    virtual std::size_t nearby_entities(std::span<HostEntity> output) = 0;
    virtual std::uint64_t add_block(Vec3 centre, std::uint32_t material) = 0;
    virtual void remove_block(std::uint64_t handle) = 0;
    virtual bool explode(const Vec3& at, float radius) = 0;
    virtual bool damage_entity(std::uint64_t target, float amount) = 0;
};

struct BlockKey {
    std::int32_t x, y, z;
    bool operator==(const BlockKey&) const = default;
};
struct BlockKeyHash {
    std::size_t operator()(BlockKey key) const noexcept {
        auto h = std::uint64_t(std::uint32_t(key.x)) * 0x9e3779b185ebca87ULL;
        h ^= std::uint64_t(std::uint32_t(key.y)) * 0xc2b2ae3d27d4eb4fULL;
        h ^= std::uint64_t(std::uint32_t(key.z)) * 0x165667b19e3779f9ULL;
        return static_cast<std::size_t>(h ^ (h >> 32));
    }
};

class HostAdapter {
public:
    static constexpr std::size_t kMaxProps = 400;
    HostAdapter(Endpoint& endpoint, GameHost& game, std::uint64_t session_epoch);
    ~HostAdapter();
    bool tick();
    std::size_t block_count() const { return blocks_.size(); }
    std::uint64_t anchor() const { return epoch_; }
private:
    bool apply(const Event& event);
    void reject(const Event& event);
    void reset_guest();
    void flush_replies();
    bool seen(std::uint64_t event_id);
    Endpoint& link_;
    GameHost& game_;
    std::uint64_t epoch_;
    WorldTransform transform_;
    std::uint64_t guest_epoch_ = 0;
    std::uint64_t frame_ = 0;
    std::size_t terrain_cursor_ = 0;
    bool announced_ = false;
    std::unordered_map<BlockKey, std::uint64_t, BlockKeyHash> blocks_;
    std::unordered_set<std::uint64_t> recent_events_;
    std::unordered_set<std::uint64_t> exposed_entities_;
    std::deque<std::uint64_t> event_order_;
    std::deque<Event> replies_;
};

} // namespace blockwater
