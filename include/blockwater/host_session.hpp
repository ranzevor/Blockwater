#pragma once

#include "blockwater/link.hpp"
#include <optional>
#include <thread>

namespace blockwater {

enum class HostContext { Unavailable, StoryMode, Paused, Multiplayer };
enum class HostPhase { Dormant, Waiting, Connected, Suspended, Refused, Stopped };

// Owns only transport state. Gameplay requires the separate GameHost adapter.
class HostSession {
public:
    HostSession(SharedBridge& shared, std::uint64_t epoch);
    ~HostSession();
    HostSession(const HostSession&) = delete;
    HostSession& operator=(const HostSession&) = delete;
    HostPhase tick(HostContext context);
    HostPhase tick(HostContext context, std::uint64_t timestamp);
    void stop() noexcept;
    HostPhase phase() const noexcept { return phase_; }
    std::uint64_t epoch() const noexcept { return epoch_; }
private:
    HostPhase tick_impl(HostContext context, std::optional<std::uint64_t> timestamp);
    void invalidate() noexcept;
    void suspend() noexcept;
    bool start_generation();
    void reject_commands(std::uint64_t timestamp);
    SharedBridge& shared_;
    Endpoint endpoint_;
    const std::thread::id thread_;
    std::uint64_t epoch_;
    std::uint64_t guest_epoch_ = 0;
    bool started_ = false;
    HostPhase phase_ = HostPhase::Dormant;
    std::optional<Event> pending_;
};

} // namespace blockwater
