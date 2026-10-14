#pragma once

#include "blockwater/wire.hpp"
#include <chrono>
#include <stdexcept>

namespace blockwater {

enum class Role { Host, Guest };

inline std::uint64_t now_us() {
    using namespace std::chrono;
    return duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
}

class Endpoint {
public:
    Endpoint(SharedBridge& shared, Role role) : shared_(shared), role_(role) {
        if (std::atomic_ref<std::uint64_t>(shared_.signature).load(std::memory_order_acquire) != kSignature ||
            shared_.size_bytes != sizeof(SharedBridge))
            throw std::invalid_argument("bridge layout mismatch");
    }
    Endpoint(const Endpoint&) = delete;
    Endpoint& operator=(const Endpoint&) = delete;

    void announce(std::uint64_t epoch) {
        if (refused_) return;
        auto& mine = own();
        store(mine.epoch, epoch ? epoch : 1);
        store(mine.reason, static_cast<std::uint64_t>(RejectReason::None));
        store(mine.heartbeat_us, now_us());
        store(mine.state, static_cast<std::uint64_t>(PeerState::Ready));
    }
    void heartbeat() {
        if (!refused_) store(own().heartbeat_us, now_us());
    }
    void refuse(RejectReason reason) {
        refused_ = true;
        auto& mine = own();
        store(mine.reason, static_cast<std::uint64_t>(reason));
        store(mine.heartbeat_us, now_us());
        store(mine.state, static_cast<std::uint64_t>(PeerState::Refused));
    }
    void close() {
        if (!refused_) store(own().state, static_cast<std::uint64_t>(PeerState::Closed));
    }
    bool refused() const { return refused_; }
    void forget_peer() noexcept {
        observed_epoch_ = 0;
        observed_heartbeat_ = 0;
        peer_seen_alive_ = false;
    }
    bool peer_ready(std::uint64_t now = now_us()) {
        const auto& p = peer();
        if (load(p.state) != static_cast<std::uint64_t>(PeerState::Ready)) { peer_seen_alive_ = false; return false; }
        const auto epoch = load(p.epoch);
        const auto stamp = load(p.heartbeat_us);
        if (epoch == 0 || stamp == 0 || stamp > now || now - stamp > kHeartbeatTimeoutUs) { peer_seen_alive_ = false; return false; }
        if (epoch != observed_epoch_) {
            observed_epoch_ = epoch;
            observed_heartbeat_ = stamp;
            peer_seen_alive_ = false;
            return false;
        }
        if (stamp != observed_heartbeat_) {
            observed_heartbeat_ = stamp;
            peer_seen_alive_ = true;
            return true;
        }
        return peer_seen_alive_;
    }
    std::uint64_t peer_epoch() const { return load(peer().epoch); }
    PeerState peer_state() const { return static_cast<PeerState>(load(peer().state)); }
    RejectReason peer_reason() const { return static_cast<RejectReason>(load(peer().reason)); }

    bool publish(const HostFrame& frame) {
        if (role_ != Role::Host || refused_ || !valid(frame)) return false;
        shared_.host_frame.publish(frame);
        return true;
    }
    bool publish(const GuestFrame& frame) {
        if (role_ != Role::Guest || refused_ || !valid(frame)) return false;
        shared_.guest_frame.publish(frame);
        return true;
    }
    bool read(HostFrame& frame) const {
        return role_ == Role::Guest && shared_.host_frame.read(frame) && valid(frame);
    }
    bool read(GuestFrame& frame) const {
        return role_ == Role::Host && shared_.guest_frame.read(frame) && valid(frame);
    }
    bool send(const Event& event) {
        if (refused_ || !valid(event)) return false;
        const auto epoch = load(own().epoch);
        if (epoch == 0) return false;
        Event outgoing = event;
        outgoing.sender_epoch = epoch;
        if (outgoing.recipient_epoch == 0) outgoing.recipient_epoch = load(peer().epoch);
        return role_ == Role::Host ? shared_.to_guest.push(outgoing) : shared_.to_host.push(outgoing);
    }
    bool receive(Event& event) {
        if (refused_) return false;
        Event candidate{};
        auto& queue = role_ == Role::Host ? shared_.to_host : shared_.to_guest;
        const auto epoch = load(own().epoch);
        while (queue.pop(candidate)) {
            if (candidate.recipient_epoch != 0 && candidate.recipient_epoch != epoch) continue;
            event = candidate;
            return true;
        }
        return false;
    }
private:
    static std::uint64_t load(const std::uint64_t& val) {
        return std::atomic_ref<std::uint64_t>(const_cast<std::uint64_t&>(val)).load(std::memory_order_seq_cst);
    }
    static void store(std::uint64_t& val, std::uint64_t v) {
        std::atomic_ref<std::uint64_t>(val).store(v, std::memory_order_seq_cst);
    }
    PeerControl& own() { return role_ == Role::Host ? shared_.host : shared_.guest; }
    const PeerControl& peer() const { return role_ == Role::Host ? shared_.guest : shared_.host; }
    SharedBridge& shared_;
    Role role_;
    bool refused_ = false;
    std::uint64_t observed_epoch_ = 0;
    std::uint64_t observed_heartbeat_ = 0;
    bool peer_seen_alive_ = false;
};

inline void initialize(SharedBridge& memory) {
    // Only the mapping creator calls this, before another process can open it.
    // SharedBridge is value-initialised in its newly created mapping.
    memory.size_bytes = sizeof(SharedBridge);
    std::atomic_ref<std::uint64_t>(memory.signature).store(kSignature, std::memory_order_release);
}

} // namespace blockwater
