#include "blockwater/world_stream.hpp"
#include <algorithm>
#include <limits>

namespace blockwater {
namespace {
bool position_ok(const Vec3& p) noexcept {
    return finite(p) && std::abs(p.x) < 1'000'000 && std::abs(p.y) < 1'000'000 && std::abs(p.z) < 1'000'000;
}
double distance_sq(const Vec3& a, const Vec3& b) noexcept {
    const double x = a.x - b.x, y = a.y - b.y, z = a.z - b.z;
    return x*x + y*y + z*z;
}
bool hit_ok(const TerrainSample& s, const GroundProbe& p) noexcept {
    if (!position_ok(s.point) || !std::isfinite(s.nx) || !std::isfinite(s.ny) || !std::isfinite(s.nz)) return false;
    const double normal = double(s.nx)*s.nx + double(s.ny)*s.ny + double(s.nz)*s.nz;
    return normal > 0.5 && normal < 1.5 && std::abs(s.point.x-p.from.x) < 0.1 &&
        std::abs(s.point.y-p.from.y) < 0.1 && s.point.z <= p.from.z+0.1 && s.point.z >= p.to.z-0.1;
}
}

void WorldStream::clear_grid() noexcept {
    cells_ = {};
    high_ = {};
    cursor_ = 0;
    // A pending engine handle must be reaped before its slot is reused, even after a teleport.
    for (auto& p : pending_) p.discarded = true;
}
void WorldStream::reset() noexcept {
    clear_grid();
    active_ = false;
    last_us_ = 0;
}
std::size_t WorldStream::pending_count() const noexcept {
    std::size_t count = 0;
    for (const auto& p : pending_) if (p.handle != 0) ++count;
    return count;
}
bool WorldStream::reanchor(std::uint64_t epoch, const Vec3& feet) noexcept {
    if (anchor_ == std::numeric_limits<std::uint64_t>::max()) return false;
    anchor_ = std::max(anchor_+1, epoch);
    epoch_ = epoch;
    origin_ = feet;
    active_ = true;
    clear_grid();
    return true;
}

bool WorldStream::collect(WorldSource& source, std::uint64_t epoch, HostFrame& out) noexcept {
    out = {};
    const auto start = source.clock_us();
    const auto view = source.view();
    if (!view || epoch == 0 || view->player == 0 || !valid(view->camera) ||
        !position_ok(view->camera.position) || !position_ok(view->feet)) {
        reset();
        return false;
    }
    const bool discontinuity = last_us_ != 0 &&
        (start < last_us_ || start-last_us_ > kHeartbeatTimeoutUs);
    if (!active_ || epoch != epoch_ || discontinuity || distance_sq(view->feet, previous_) > 32*32 ||
        distance_sq(view->feet, origin_) > 128*128) {
        if (!reanchor(epoch, view->feet)) { reset(); return false; }
    }
    previous_ = view->feet;
    last_us_ = start;
    const auto cx = std::floor(view->feet.x/kSpacing)*kSpacing;
    const auto cy = std::floor(view->feet.y/kSpacing)*kSpacing;
    if (cx != centre_x_ || cy != centre_y_ || std::abs(view->feet.z-reference_z_) > 2) {
        clear_grid();
    }
    centre_x_ = cx; centre_y_ = cy;
    if (cursor_ == 0) reference_z_ = view->feet.z;
    for (auto& c : cells_) {
        if (c.valid && (start < c.sampled_at || start-c.sampled_at > kSampleLifetimeUs)) c.valid = false;
    }
    const auto within_budget = [&]() noexcept {
        const auto now = source.clock_us();
        return now >= start && now-start < kBudgetUs;
    };
    // Poll at most eight handles; never spin on a pending result.
    for (auto& p : pending_) {
        if (p.handle == 0 || !within_budget()) continue;
        if (start < p.started_at || start-p.started_at > kProbeTimeoutUs) p.discarded = true;
        const auto result = source.poll(p.handle);
        if (result.state == ProbeState::Pending) continue;
        if (!p.discarded) {
            if (result.state == ProbeState::Hit && hit_ok(result.sample, p.probe)) {
                cells_[p.cell] = {result.sample, start, true};
                high_[p.cell] = false;
            } else if (result.state == ProbeState::Miss && !p.high) {
                high_[p.cell] = true;
            } else {
                high_[p.cell] = false;
            }
        }
        p = {};
    }
    for (auto& slot : pending_) {
        if (slot.handle != 0 || !within_budget()) continue;
        for (std::size_t searched = 0; searched < kCells; ++searched) {
            const auto cell = cursor_++ % kCells;
            if (cells_[cell].valid) continue;
            bool busy = false;
            for (const auto& p : pending_) if (p.handle != 0 && !p.discarded && p.cell == cell) busy = true;
            if (busy) continue;
            // Enumerate rings around the player before the outer grid cells.
            std::size_t index = 0;
            int dx = 0, dy = 0;
            bool found = false;
            for (int ring = 0; ring <= 4 && !found; ++ring) {
                for (int y = -ring; y <= ring && !found; ++y) {
                    for (int x = -ring; x <= ring; ++x) {
                        if (std::max(std::abs(x), std::abs(y)) != ring) continue;
                        if (index++ == cell) { dx = x; dy = y; found = true; break; }
                    }
                }
            }
            const bool high = high_[cell];
            GroundProbe probe{{cx+dx*kSpacing, cy+dy*kSpacing, reference_z_+(high ? 150 : 2)},
                              {cx+dx*kSpacing, cy+dy*kSpacing, reference_z_+(high ? 2 : -80)}, view->player};
            const auto handle = source.begin(probe);
            if (handle != 0) slot = {handle, cell, probe, start, false, high};
            break;
        }
    }
    if (frame_ == std::numeric_limits<std::uint64_t>::max()) { reset(); return false; }
    out.frame_id = ++frame_;
    out.timestamp_us = start;
    out.anchor = anchor_;
    out.flags = kStoryMode;
    out.camera = view->camera;
    out.player_feet = view->feet;
    out.host_origin = origin_;
    out.guest_origin = {0, 64, 0};
    out.metres_per_block = 1;
    for (const auto& c : cells_) if (c.valid) out.terrain[out.terrain_count++] = c.sample;
    return true;
}

} // namespace blockwater
