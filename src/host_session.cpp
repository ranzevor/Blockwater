#include "blockwater/host_session.hpp"
#include <limits>
#include <stdexcept>

namespace blockwater {

HostSession::HostSession(SharedBridge& shared, std::uint64_t epoch)
    : shared_(shared), endpoint_(shared, Role::Host),
      thread_(std::this_thread::get_id()), epoch_(epoch ? epoch : 1) {
    endpoint_.close();
    invalidate();
}

HostSession::~HostSession() { stop(); }

void HostSession::invalidate() noexcept {
    // A zero-flag snapshot invalidates data retained by an older host generation.
    shared_.host_frame.publish(HostFrame{});
    pending_.reset();
    guest_epoch_ = 0;
    endpoint_.forget_peer();
}

void HostSession::stop() noexcept {
    if (phase_ == HostPhase::Stopped) return;
    endpoint_.close();
    invalidate();
    phase_ = HostPhase::Stopped;
}

void HostSession::suspend() noexcept {
    if (phase_ == HostPhase::Suspended) return;
    endpoint_.close();
    invalidate();
    phase_ = HostPhase::Suspended;
}

bool HostSession::start_generation() {
    if (started_) {
        if (epoch_ == std::numeric_limits<std::uint64_t>::max()) {
            endpoint_.refuse(RejectReason::Protocol);
            phase_ = HostPhase::Refused;
            return false;
        }
        ++epoch_;
    }
    started_ = true;
    endpoint_.announce(epoch_);
    phase_ = HostPhase::Waiting;
    return true;
}

HostPhase HostSession::tick(HostContext context) {
    return tick_impl(context, std::nullopt);
}

HostPhase HostSession::tick(HostContext context, std::uint64_t timestamp) {
    return tick_impl(context, timestamp);
}

HostPhase HostSession::tick_impl(HostContext context, std::optional<std::uint64_t> timestamp) {
    if (std::this_thread::get_id() != thread_)
        throw std::logic_error("host session requires its script thread");
    if (phase_ == HostPhase::Stopped || phase_ == HostPhase::Refused) return phase_;
    if (context == HostContext::Multiplayer) {
        invalidate();
        endpoint_.refuse(RejectReason::Multiplayer);
        return phase_ = HostPhase::Refused;
    }
    if (context != HostContext::StoryMode) {
        suspend();
        return phase_;
    }
    if (phase_ == HostPhase::Dormant || phase_ == HostPhase::Suspended) {
        if (!start_generation()) return phase_;
    }
    endpoint_.heartbeat();
    const auto remote_epoch = endpoint_.peer_epoch();
    const bool alive = timestamp ? endpoint_.peer_ready(*timestamp) : endpoint_.peer_ready();
    if (phase_ == HostPhase::Connected && (!alive || remote_epoch != guest_epoch_)) {
        suspend();
        start_generation();
        return phase_;
    }
    if (!alive) return phase_;
    guest_epoch_ = remote_epoch;
    phase_ = HostPhase::Connected;
    reject_commands(timestamp.value_or(now_us()));
    return phase_;
}

void HostSession::reject_commands(std::uint64_t timestamp) {
    if (pending_) {
        if (!endpoint_.send(*pending_)) return;
        pending_.reset();
    }
    // Bound raw pops as well as accepted messages: stale senders cannot extend a tick.
    for (unsigned count = 0; count < 32; ++count) {
        Event request{};
        if (!shared_.to_host.pop(request)) return;
        if (!valid(request) || request.kind > EventKind::DamageEntity ||
            request.sender_epoch != guest_epoch_ ||
            request.recipient_epoch != epoch_) continue;
        Event response{};
        response.id = request.id;
        response.timestamp_us = timestamp;
        response.recipient_epoch = request.sender_epoch;
        response.kind = EventKind::Rejected;
        response.payload = static_cast<std::uint32_t>(request.kind);
        response.block_x = request.block_x;
        response.block_y = request.block_y;
        response.block_z = request.block_z;
        if (!endpoint_.send(response)) {
            pending_ = response;
            return;
        }
    }
}

} // namespace blockwater
