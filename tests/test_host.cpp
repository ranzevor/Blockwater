#include "blockwater/host_adapter.hpp"
#include "test.hpp"
#include <map>
#include <memory>
#include <set>
#include <chrono>
#include <thread>

using namespace blockwater;

namespace {
class TestHost final : public GameHost {
public:
    bool single = true;
    std::uint64_t next = 0;
    int blocks_added = 0, blocks_removed = 0, explosions = 0, damages = 0, rays = 0;
    bool story_mode() const override { return single; }
    Pose camera() const override { return {.position = {1, 2, 3}, .yaw = 33, .fov = 80}; }
    Vec3 player_feet() const override { return {1, 2, 3}; }
    bool sample_terrain(double x, double y, TerrainSample& result) override {
        ++rays;
        result.point = {x, y, 10};
        return true;
    }
    std::size_t nearby_entities(std::span<HostEntity> target) override {
        target[0].id = 12345;
        target[0].health = 100;
        return 1;
    }
    Vec3 last_block{};
    std::uint64_t add_block(Vec3 centre, std::uint32_t) override {
        last_block = centre;
        ++blocks_added;
        return ++next;
    }
    void remove_block(std::uint64_t) override { ++blocks_removed; }
    bool explode(const Vec3&, float) override { ++explosions; return true; }
    bool damage_entity(std::uint64_t, float) override { ++damages; return true; }
};
}

void test_host() {
    {
        auto mem = std::make_unique<SharedBridge>();
        initialize(*mem);
        Endpoint host(*mem, Role::Host), guest(*mem, Role::Guest);
        TestHost fake;
        {
            HostAdapter adapter(host, fake, 99);
            guest.announce(100);
            REQUIRE(adapter.tick());
            guest.heartbeat();
            REQUIRE(adapter.tick());
            HostFrame snapshot{};
            REQUIRE(guest.read(snapshot));
            REQUIRE(snapshot.terrain_count == 16);
            REQUIRE(snapshot.anchor == 99 && snapshot.flags == kStoryMode);
            REQUIRE(fake.rays == 32);
            REQUIRE(snapshot.entity_count == 1 && snapshot.entities[0].id == 12345);
            REQUIRE(snapshot.host_origin.x == 1 && snapshot.guest_origin.y == 64);
            Event place{};
            place.id = 1;
            place.kind = EventKind::PlaceBlock;
            place.block_x = 1;
            place.block_y = 64;
            place.block_z = 1;
            REQUIRE(guest.send(place));
            REQUIRE(adapter.tick());
            REQUIRE(adapter.block_count() == 1 && fake.blocks_added == 1);
            REQUIRE(fake.last_block.x == 2.5 && fake.last_block.y == 0.5 && fake.last_block.z == 3.5);
            REQUIRE(guest.send(place));
            REQUIRE(adapter.tick());
            REQUIRE(fake.blocks_added == 1);
            Event boom{};
            boom.id = 2;
            boom.kind = EventKind::Explosion;
            boom.magnitude = 8;
            REQUIRE(guest.send(boom));
            REQUIRE(adapter.tick());
            REQUIRE(fake.explosions == 1);
            Event dmg{};
            dmg.id = 3;
            dmg.kind = EventKind::DamageEntity;
            dmg.target = 12345;
            dmg.magnitude = 30;
            REQUIRE(guest.send(dmg));
            REQUIRE(adapter.tick());
            REQUIRE(fake.damages == 1);
            dmg.id = 6;
            dmg.target = 54321;
            REQUIRE(guest.send(dmg));
            REQUIRE(adapter.tick());
            Event denied{};
            REQUIRE(guest.receive(denied));
            REQUIRE(denied.id == 6 && denied.kind == EventKind::Rejected);
            REQUIRE(fake.damages == 1);
            Event distant{};
            distant.id = 7;
            distant.kind = EventKind::Explosion;
            distant.position = {1000,1000,1000};
            distant.magnitude = 2;
            REQUIRE(guest.send(distant));
            REQUIRE(adapter.tick());
            REQUIRE(guest.receive(denied));
            REQUIRE(denied.id == 7 && denied.kind == EventKind::Rejected);
            REQUIRE(fake.explosions == 1);
            Event bad{};
            bad.id = 4;
            bad.kind = EventKind::Explosion;
            bad.magnitude = 60;
            REQUIRE(guest.send(bad));
            REQUIRE(adapter.tick());
            Event reply{};
            REQUIRE(guest.receive(reply));
            REQUIRE(reply.kind == EventKind::Rejected && reply.id == 4);
            Event erase{};
            erase.id = 5;
            erase.kind = EventKind::RemoveBlock;
            erase.block_x = 1;
            erase.block_y = 64;
            erase.block_z = 1;
            REQUIRE(guest.send(erase));
            REQUIRE(adapter.tick());
            REQUIRE(adapter.block_count() == 0 && fake.blocks_removed == 1);
            for (std::size_t i = 0; i < HostAdapter::kMaxProps + 1; ++i) {
                place.id = 100 + i;
                place.block_x = static_cast<std::int32_t>(i % 20);
                place.block_z = static_cast<std::int32_t>(i / 20);
                REQUIRE(guest.send(place));
                if ((i + 1) % 24 == 0) REQUIRE(adapter.tick());
            }
            for (int i = 0; i < 3; ++i) REQUIRE(adapter.tick());
            REQUIRE(adapter.block_count() == HostAdapter::kMaxProps);
            REQUIRE(fake.blocks_added == HostAdapter::kMaxProps + 1);
            REQUIRE(guest.receive(reply));
            REQUIRE(reply.kind == EventKind::Rejected && reply.id == 100 + HostAdapter::kMaxProps);
        }
        REQUIRE(fake.blocks_removed == HostAdapter::kMaxProps + 1);
    }
    {
        auto mem = std::make_unique<SharedBridge>();
        initialize(*mem);
        Endpoint host(*mem, Role::Host), guest(*mem, Role::Guest);
        TestHost fake;
        HostAdapter adapter(host, fake, 999);
        guest.announce(11);
        REQUIRE(adapter.tick());
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        guest.heartbeat();
        REQUIRE(adapter.tick());
        Event place{};
        place.id = 50;
        place.kind = EventKind::PlaceBlock;
        place.block_y = 64;
        REQUIRE(guest.send(place));
        REQUIRE(adapter.tick());
        REQUIRE(adapter.block_count() == 1);
        place.id = 51;
        REQUIRE(guest.send(place));
        guest.announce(12);
        REQUIRE(adapter.tick());
        REQUIRE(adapter.block_count() == 0);
        REQUIRE(fake.blocks_removed == 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        guest.heartbeat();
        REQUIRE(adapter.tick());
        REQUIRE(adapter.block_count() == 0);  // Old session's pending event was discarded.
        place.id = 50; // IDs may restart at 1 on a fresh guest process.
        REQUIRE(guest.send(place));
        REQUIRE(adapter.tick());
        REQUIRE(adapter.block_count() == 1);
        REQUIRE(fake.blocks_added == 2);
    }
    {
        auto mem = std::make_unique<SharedBridge>();
        initialize(*mem);
        Endpoint host(*mem, Role::Host), guest(*mem, Role::Guest);
        TestHost fake;
        fake.single = false;
        HostAdapter adapter(host, fake, 99);
        REQUIRE(!adapter.tick());
        REQUIRE(host.refused());
        REQUIRE(guest.peer_reason() == RejectReason::Multiplayer);
        fake.single = true;
        REQUIRE(!adapter.tick());
        REQUIRE(fake.rays == 0);
    }
}
