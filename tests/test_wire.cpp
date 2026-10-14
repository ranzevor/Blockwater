#include "blockwater/link.hpp"
#include "blockwater/coordinates.hpp"
#include "test.hpp"
#include <cmath>
#include <limits>
#include <memory>
#include <thread>

using namespace blockwater;

void test_wire() {
    auto mem = std::make_unique<SharedBridge>();
    initialize(*mem);
    Endpoint host(*mem, Role::Host);
    Endpoint guest(*mem, Role::Guest);
    HostFrame frame{};
    frame.flags = kStoryMode;
    frame.camera.fov = 65;
    frame.frame_id = 8;
    REQUIRE(host.publish(frame));
    HostFrame received{};
    REQUIRE(guest.read(received));
    REQUIRE(received.frame_id == 8);
    REQUIRE(!host.read(received));
    frame.terrain_count = static_cast<std::uint32_t>(kTerrainMax + 1);
    REQUIRE(!host.publish(frame));
    frame.terrain_count = 0;
    frame.camera.fov = std::numeric_limits<float>::quiet_NaN();
    REQUIRE(!host.publish(frame));
    frame.camera.fov = 75;
    frame.flags = 0;
    REQUIRE(!host.publish(frame));
    frame.flags = kStoryMode;
    REQUIRE(host.publish(frame));
    GuestFrame guest_frame{};
    REQUIRE(guest.publish(guest_frame));
    REQUIRE(host.read(guest_frame));

    for (int i = 1; i <= 200; ++i) {
        frame.frame_id = i;
        frame.player_feet.x = i;
        frame.camera.position.x = i;
        REQUIRE(host.publish(frame));
        REQUIRE(guest.read(received));
        REQUIRE(received.frame_id == static_cast<std::uint64_t>(received.player_feet.x));
    }
    std::thread writer([&]() {
        for (int i = 1; i <= 1800; ++i) {
            frame.frame_id = i;
            frame.player_feet.x = i;
            frame.camera.position.x = i;
            REQUIRE(host.publish(frame));
        }
    });
    for (int i = 0; i < 1800; ++i) {
        if (!guest.read(received)) continue;
        REQUIRE(received.frame_id == static_cast<std::uint64_t>(received.player_feet.x));
        REQUIRE(received.camera.position.x == received.player_feet.x);
    }
    writer.join();
    REQUIRE(guest.read(received));
    REQUIRE(received.frame_id == 1800);

    WorldTransform t{{-1800, 2800, 800}, {0, 64, 0}, 1};
    REQUIRE(t.valid());
    const Vec3 host_world{-1796.5, 2808.25, 810.75};
    auto mc = t.host_to_guest(host_world);
    REQUIRE(mc.x == 3.5 && mc.y == 74.75 && mc.z == -8.25);
    auto restored = t.guest_to_host(mc);
    REQUIRE(restored.x == host_world.x && restored.y == host_world.y && restored.z == host_world.z);
    t.metres_per_block = 0;
    REQUIRE(!t.valid());

    mem->signature = 0;
    bool mismatch = false;
    try { Endpoint fail(*mem, Role::Guest); } catch (const std::invalid_argument&) { mismatch = true; }
    REQUIRE(mismatch);
}
