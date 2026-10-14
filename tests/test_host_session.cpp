#include "blockwater/host_session.hpp"
#include "test.hpp"
#include <exception>
#include <iostream>
#include <limits>
#include <memory>
#include <thread>

using namespace blockwater;

namespace {
struct Fixture {
    std::unique_ptr<SharedBridge> memory = std::make_unique<SharedBridge>();
    std::optional<Endpoint> guest;
    std::optional<HostSession> host;
    std::uint64_t clock = now_us();
    Fixture(std::uint64_t epoch = 100) {
        initialize(*memory);
        guest.emplace(*memory, Role::Guest);
        host.emplace(*memory, epoch);
        guest->announce(200);
        pulse();
    }
    void pulse() {
        clock += 100;
        std::atomic_ref<std::uint64_t>(memory->guest.heartbeat_us).store(clock);
    }
    HostPhase tick(HostContext context = HostContext::StoryMode) {
        return host->tick(context, clock);
    }
    void connect() {
        REQUIRE(tick() == HostPhase::Waiting);
        pulse();
        REQUIRE(tick() == HostPhase::Connected);
    }
    Event command(std::uint64_t id) {
        Event request{};
        request.id = id;
        request.kind = EventKind::PlaceBlock;
        request.block_x = 3;
        REQUIRE(guest->send(request));
        return request;
    }
};

void states() {
    Fixture f;
    HostFrame absent{};
    REQUIRE(!f.guest->read(absent));
    REQUIRE(f.tick(HostContext::Unavailable) == HostPhase::Suspended);
    REQUIRE(f.guest->peer_state() == PeerState::Closed);
    f.connect();
    REQUIRE(f.guest->peer_epoch() == 100);
    REQUIRE(!f.guest->read(absent));
    REQUIRE(f.tick(HostContext::Paused) == HostPhase::Suspended);
    REQUIRE(f.guest->peer_state() == PeerState::Closed);
    f.connect();
    REQUIRE(f.guest->peer_epoch() == 101);
    f.host->stop();
    REQUIRE(f.tick() == HostPhase::Stopped);
    REQUIRE(f.guest->peer_state() == PeerState::Closed);
    f.host->stop();
}

void multiplayer() {
    Fixture f;
    f.connect();
    f.command(1);
    REQUIRE(f.tick(HostContext::Multiplayer) == HostPhase::Refused);
    REQUIRE(f.guest->peer_state() == PeerState::Refused);
    REQUIRE(f.guest->peer_reason() == RejectReason::Multiplayer);
    REQUIRE(f.memory->to_host.consumed == 0);
    REQUIRE(f.tick() == HostPhase::Refused);
    REQUIRE(f.tick(HostContext::Paused) == HostPhase::Refused);
    f.host->stop();
    REQUIRE(f.guest->peer_state() == PeerState::Refused);
}

void rejection() {
    Fixture f;
    f.connect();
    f.command(7);
    f.tick();
    Event response{};
    REQUIRE(f.guest->receive(response));
    REQUIRE(response.id == 7 && response.kind == EventKind::Rejected);
    REQUIRE(response.payload == static_cast<std::uint32_t>(EventKind::PlaceBlock));
    REQUIRE(response.block_x == 3 && response.sender_epoch == 100);
    REQUIRE(!f.guest->receive(response));
    Event acknowledgement{};
    acknowledgement.id = 8;
    acknowledgement.kind = EventKind::Rejected;
    REQUIRE(f.guest->send(acknowledgement));
    f.tick();
    REQUIRE(!f.guest->receive(response));
}

void backpressure() {
    Fixture f;
    f.connect();
    Event filler{};
    filler.id = 900;
    filler.recipient_epoch = 200;
    filler.sender_epoch = 100;
    filler.kind = EventKind::Rejected;
    for (std::size_t i = 0; i < kEventCapacity; ++i)
        REQUIRE(f.memory->to_guest.push(filler));
    f.command(1);
    f.command(2);
    f.tick();
    REQUIRE(f.memory->to_host.consumed == 1);
    f.tick();
    REQUIRE(f.memory->to_host.consumed == 1);
    Event result{};
    for (std::size_t i = 0; i < kEventCapacity; ++i)
        REQUIRE(f.guest->receive(result) && result.id == 900);
    f.tick();
    REQUIRE(f.guest->receive(result) && result.id == 1);
    REQUIRE(f.guest->receive(result) && result.id == 2);
    REQUIRE(!f.guest->receive(result));
}

void stale_budget() {
    Fixture f;
    f.connect();
    Event stale{};
    stale.id = 1;
    stale.sender_epoch = 200;
    stale.recipient_epoch = 99;
    for (std::size_t i = 0; i < kEventCapacity; ++i)
        REQUIRE(f.memory->to_host.push(stale));
    f.tick();
    REQUIRE(f.memory->to_host.consumed == 32);
    Event reply{};
    REQUIRE(!f.guest->receive(reply));
}

void reconnect() {
    Fixture f;
    f.connect();
    f.command(1);
    f.clock += kHeartbeatTimeoutUs + 1;
    REQUIRE(f.tick() == HostPhase::Waiting);
    REQUIRE(f.host->epoch() == 101);
    REQUIRE(f.memory->to_host.consumed == 0);
    f.pulse();
    REQUIRE(f.tick() == HostPhase::Waiting);
    f.pulse();
    REQUIRE(f.tick() == HostPhase::Connected);
    Event result{};
    REQUIRE(!f.guest->receive(result));
    f.command(2);
    f.tick();
    REQUIRE(f.guest->receive(result) && result.id == 2);
    f.guest->announce(201);
    f.pulse();
    REQUIRE(f.tick() == HostPhase::Waiting);
    REQUIRE(f.host->epoch() == 102);
    f.pulse();
    REQUIRE(f.tick() == HostPhase::Waiting);
    f.pulse();
    REQUIRE(f.tick() == HostPhase::Connected);
    f.guest->close();
    REQUIRE(f.tick() == HostPhase::Waiting);
    REQUIRE(f.host->epoch() == 103);
}

void thread_guard() {
    Fixture f;
    f.connect();
    const auto stamp = f.memory->host.heartbeat_us;
    bool refused = false;
    std::thread wrong([&] {
        try { f.host->tick(HostContext::StoryMode); }
        catch (const std::logic_error&) { refused = true; }
    });
    wrong.join();
    REQUIRE(refused);
    REQUIRE(f.host->phase() == HostPhase::Connected);
    REQUIRE(f.memory->host.heartbeat_us == stamp);
}

void epoch_exhaustion() {
    Fixture f(std::numeric_limits<std::uint64_t>::max());
    f.connect();
    f.tick(HostContext::Paused);
    REQUIRE(f.tick() == HostPhase::Refused);
    REQUIRE(f.guest->peer_reason() == RejectReason::Protocol);
}

void concurrent_heartbeats() {
    auto memory = std::make_unique<SharedBridge>();
    initialize(*memory);
    Endpoint guest(*memory, Role::Guest);
    guest.announce(200);
    HostSession host(*memory, 100);
    std::atomic<bool> running{true};
    std::atomic<bool> began{false};
    std::thread writer([&] {
        guest.heartbeat();
        began.store(true, std::memory_order_release);
        while (running.load(std::memory_order_relaxed)) guest.heartbeat();
    });
    while (!began.load(std::memory_order_acquire)) std::this_thread::yield();
    std::size_t connected = 0;
    for (unsigned i = 0; i < 100'000; ++i)
        connected += host.tick(HostContext::StoryMode) == HostPhase::Connected;
    running.store(false, std::memory_order_relaxed);
    writer.join();
    REQUIRE(connected > 0);
    REQUIRE(host.epoch() == 100);
}

void frame_invalidation() {
    auto memory = std::make_unique<SharedBridge>();
    initialize(*memory);
    HostFrame frame{};
    frame.flags = kStoryMode;
    frame.frame_id = 45;
    memory->host_frame.publish(frame);
    Endpoint guest(*memory, Role::Guest);
    REQUIRE(guest.read(frame));
    HostSession host(*memory, 300);
    REQUIRE(!guest.read(frame));
}
}

int main() {
    try {
        states(); multiplayer(); rejection(); backpressure(); stale_budget();
        reconnect(); thread_guard(); epoch_exhaustion(); frame_invalidation(); concurrent_heartbeats();
        std::cout << "10 host session scenarios passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
