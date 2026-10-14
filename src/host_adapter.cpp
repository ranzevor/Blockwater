#include "blockwater/host_adapter.hpp"
#include <cmath>
#include <utility>

namespace blockwater {

HostAdapter::HostAdapter(Endpoint& endpoint, GameHost& game, std::uint64_t session_epoch)
    : link_(endpoint), game_(game), epoch_(session_epoch ? session_epoch : 1) {
    transform_.host_origin = game_.player_feet();
    transform_.guest_origin = {0, 64, 0};
}

HostAdapter::~HostAdapter() {
    if (game_.story_mode()) for (const auto& [key, handle] : blocks_) {
        (void)key;
        game_.remove_block(handle);
    }
    link_.close();
}

void HostAdapter::reset_guest() {
    for (const auto& [key, handle] : blocks_) {
        (void)key;
        game_.remove_block(handle);
    }
    blocks_.clear();
    exposed_entities_.clear();
    recent_events_.clear();
    event_order_.clear();
    replies_.clear();
}

void HostAdapter::flush_replies() {
    while (!replies_.empty()) {
        if (!link_.send(replies_.front())) return;
        replies_.pop_front();
    }
}

void HostAdapter::reject(const Event& original) {
    Event out{};
    out.id = original.id;
    out.recipient_epoch = original.sender_epoch;
    out.timestamp_us = now_us();
    out.kind = EventKind::Rejected;
    out.payload = static_cast<std::uint32_t>(original.kind);
    out.block_x = original.block_x;
    out.block_y = original.block_y;
    out.block_z = original.block_z;
    replies_.push_back(out);
}

bool HostAdapter::seen(std::uint64_t id) {
    if (!recent_events_.insert(id).second) return true;
    event_order_.push_back(id);
    if (event_order_.size() > 4096) {
        recent_events_.erase(event_order_.front());
        event_order_.pop_front();
    }
    return false;
}

bool HostAdapter::apply(const Event& event) {
    if (!valid(event)) return false;
    const auto feet = game_.player_feet();
    const auto nearby = [&](Vec3 target) {
        return finite(target) && finite(feet) &&
            std::abs(target.x - feet.x) <= 96 &&
            std::abs(target.y - feet.y) <= 96 &&
            std::abs(target.z - feet.z) <= 96;
    };
    const BlockKey cell{event.block_x, event.block_y, event.block_z};
    switch (event.kind) {
    case EventKind::PlaceBlock: {
        if (blocks_.contains(cell)) return true;
        if (blocks_.size() >= kMaxProps) return false;
        const Vec3 centre = transform_.guest_to_host({cell.x + 0.5, cell.y + 0.5, cell.z + 0.5});
        if (!nearby(centre)) return false;
        const auto handle = game_.add_block(centre, event.payload);
        if (handle == 0) return false;
        blocks_.emplace(cell, handle);
        return true;
    }
    case EventKind::RemoveBlock: {
        auto found = blocks_.find(cell);
        if (found == blocks_.end()) return true;
        game_.remove_block(found->second);
        blocks_.erase(found);
        return true;
    }
    case EventKind::Explosion:
        return nearby(event.position) && event.magnitude >= 0.1f && event.magnitude <= 50.0f && game_.explode(event.position, event.magnitude);
    case EventKind::DamageEntity:
        return event.target != 0 && exposed_entities_.contains(event.target) &&
            event.magnitude > 0 && event.magnitude <= 500 &&
            game_.damage_entity(event.target, event.magnitude);
    default:
        return false;
    }
}

bool HostAdapter::tick() {
    if (!game_.story_mode()) {
        link_.refuse(RejectReason::Multiplayer);
        return false;
    }
    if (link_.refused()) return false;
    if (!announced_) {
        link_.announce(epoch_);
        announced_ = true;
    } else {
        link_.heartbeat();
    }
    HostFrame snapshot{};
    snapshot.frame_id = ++frame_;
    snapshot.timestamp_us = now_us();
    snapshot.anchor = epoch_;
    snapshot.flags = kStoryMode;
    snapshot.camera = game_.camera();
    snapshot.player_feet = game_.player_feet();
    snapshot.host_origin = transform_.host_origin;
    snapshot.guest_origin = transform_.guest_origin;
    snapshot.metres_per_block = transform_.metres_per_block;
    snapshot.entity_count = static_cast<std::uint32_t>(game_.nearby_entities(snapshot.entities));
    if (snapshot.entity_count > kEntityMax) return false;
    exposed_entities_.clear();
    for (std::uint32_t i = 0; i < snapshot.entity_count; ++i)
        if (snapshot.entities[i].id) exposed_entities_.insert(snapshot.entities[i].id);

    // Incremental sampling avoids a large per-frame synchronous raycast cost.
    // The collision data is in RDR2's native Z-up metre coordinates.
    constexpr std::size_t kProbeWidth = 17;
    constexpr std::size_t kSamplesPerFrame = 16;
    const double origin_x = std::floor(snapshot.player_feet.x);
    const double origin_y = std::floor(snapshot.player_feet.y);
    for (std::size_t n = 0; n < kSamplesPerFrame; ++n) {
        const auto index = terrain_cursor_++ % (kProbeWidth * kProbeWidth);
        const auto dx = int(index % kProbeWidth) - 8;
        const auto dy = int(index / kProbeWidth) - 8;
        TerrainSample sample{};
        if (game_.sample_terrain(origin_x + dx, origin_y + dy, sample))
            snapshot.terrain[snapshot.terrain_count++] = sample;
    }
    if (!link_.publish(snapshot)) return false;

    const auto remote_epoch = link_.peer_epoch();
    if (remote_epoch != guest_epoch_) {
        reset_guest();
        guest_epoch_ = remote_epoch;
    }
    if (!link_.peer_ready()) return true;
    flush_replies();
    // When the reverse queue is full, retain unsent rejects and do not consume
    // further commands; a processed command must not vanish silently.
    for (std::size_t n = 0; n < 32 && replies_.size() < kEventCapacity; ++n) {
        Event event{};
        if (!link_.receive(event)) break;
        if (event.id == 0 || event.sender_epoch != guest_epoch_) continue;
        if (seen(event.id)) continue;
        if (!apply(event)) reject(event);
    }
    flush_replies();
    return true;
}

} // namespace blockwater
