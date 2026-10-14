#include "blockwater/host_adapter.hpp"
#include "blockwater/mapping.hpp"
#include <iostream>
#include <thread>

using namespace blockwater;

namespace {
class FakeGame final : public GameHost {
public:
    bool story_mode() const override { return true; }
    Pose camera() const override { return {.position = {4, 8, 2}, .fov = 70}; }
    Vec3 player_feet() const override { return {4, 8, 1}; }
    bool sample_terrain(double x, double y, TerrainSample& out) override {
        out.point = {x,y,0};
        return true;
    }
    std::size_t nearby_entities(std::span<HostEntity>) override { return 0; }
    std::uint64_t add_block(Vec3 centre, std::uint32_t material) override {
        (void)centre; (void)material;
        return ++next;
    }
    void remove_block(std::uint64_t) override {}
    bool explode(const Vec3&, float radius) override {
        std::cout << "Explosion propagated: radius=" << radius << "\n";
        return true;
    }
    bool damage_entity(std::uint64_t, float) override { return true; }
private:
    std::uint64_t next = 0;
};
}

int main() {
    Mapping map("fake_demo", Mapping::Mode::Create);
    Endpoint host(map.bridge(), Role::Host);
    Endpoint guest(map.bridge(), Role::Guest);
    FakeGame game;
    HostAdapter adapter(host, game, 42);
    guest.announce(16);
    adapter.tick();
    HostFrame snapshot{};
    if (!guest.read(snapshot)) return 1;
    std::cout << "Host frame=" << snapshot.frame_id << " ground samples=" << snapshot.terrain_count << "\n";
    Event place{};
    place.id = 1;
    place.kind = EventKind::PlaceBlock;
    place.block_x = 4;
    place.block_y = 64;
    place.block_z = 1;
    if (!guest.send(place)) return 2;
    Event boom{};
    boom.id = 2;
    boom.kind = EventKind::Explosion;
    boom.position = {4,8,1};
    boom.magnitude = 4;
    if (!guest.send(boom)) return 3;
    guest.heartbeat();
    adapter.tick();
    std::cout << "Host block proxies=" << adapter.block_count() << "\n";
    return adapter.block_count() == 1 ? 0 : 4;
}
