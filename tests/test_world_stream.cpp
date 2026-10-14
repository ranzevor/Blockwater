#include "blockwater/world_stream.hpp"
#include "../rdr2/src/native_values.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

#define CHECK(x) do { if (!(x)) throw std::runtime_error(std::string(__func__)+":"+std::to_string(__LINE__)+" " #x); } while(false)
using namespace blockwater;
namespace {
struct Fake final : WorldSource {
    mutable std::uint64_t time = 1000;
    unsigned begins = 0, polls = 0;
    std::uint64_t call_cost = 0;
    bool bad_view = false, wait = false, miss = false, bad_hit = false, fail_begin = false;
    WorldView value{{{10, 20, 6}, 25, -5, 2, 60}, {10, 20, 4}, 7};
    struct Probe { std::int32_t handle = 0; GroundProbe ray{}; };
    std::array<Probe, 8> live{};
    std::int32_t next = 1;
    std::uint64_t clock_us() const noexcept override { return time; }
    std::optional<WorldView> view() noexcept override { time += call_cost; return bad_view ? std::nullopt : std::optional{value}; }
    std::int32_t begin(const GroundProbe& p) noexcept override {
        ++begins; time += call_cost;
        if (fail_begin) return 0;
        for (auto& q : live) if (!q.handle) { q = {next++, p}; return q.handle; }
        return 0;
    }
    ProbeResult poll(std::int32_t h) noexcept override {
        ++polls; time += call_cost;
        for (auto& q : live) if (q.handle == h) {
            if (wait) return {};
            const auto ray = q.ray;
            q = {};
            if (miss) return {ProbeState::Miss, {}};
            TerrainSample sample{{ray.from.x, ray.from.y, (ray.from.z+ray.to.z)/2}, 0, 0, 1, 1};
            if (bad_hit) sample.nz = std::numeric_limits<float>::quiet_NaN();
            return {ProbeState::Hit, sample};
        }
        return {ProbeState::Invalid, {}};
    }
};
void basic() {
    Fake f; WorldStream s; HostFrame out{};
    CHECK(s.collect(f, 100, out)); CHECK(valid(out)); CHECK(out.anchor == 100);
    CHECK(out.terrain_count == 0); CHECK(s.pending_count() == 8); CHECK(out.entity_count == 0);
    CHECK(out.camera.yaw == 25 && out.player_feet.z == 4 && out.guest_origin.y == 64);
    f.time += 16'000;
    CHECK(s.collect(f, 100, out)); CHECK(out.terrain_count == 8); CHECK(out.terrain[0].point.x == 10);
    for (int i = 0; i < 15; ++i) { f.time += 1000; CHECK(s.collect(f, 100, out)); }
    CHECK(out.terrain_count == WorldStream::kCells); CHECK(s.pending_count() == 0);
}
void invalid_view() {
    Fake f; WorldStream s; HostFrame out{}; CHECK(s.collect(f, 100, out));
    f.value.camera.fov = 180; CHECK(!s.collect(f, 100, out)); CHECK(!valid(out)); CHECK(out.flags == 0);
    CHECK(f.begins == 8 && f.polls == 0);
    f.value.camera.fov = 60; f.value.feet.x = std::numeric_limits<double>::infinity();
    CHECK(!s.collect(f, 100, out)); f.value.feet.x = 10;
    CHECK(s.collect(f, 100, out)); CHECK(out.anchor == 101 && out.terrain_count == 0);
}
void transitions() {
    Fake f; WorldStream s; HostFrame out{};
    CHECK(s.collect(f, 100, out));
    f.value.feet.x += 100;
    CHECK(s.collect(f, 100, out)); CHECK(out.anchor == 101 && out.terrain_count == 0);
    CHECK(out.host_origin.x == 110);
    CHECK(s.collect(f, 102, out)); CHECK(out.anchor == 102 && out.terrain_count == 0);
    s.reset(); CHECK(s.collect(f, 102, out)); CHECK(out.anchor == 103 && out.terrain_count == 0);
}
void pending_timeout() {
    Fake f; WorldStream s; HostFrame out{}; f.wait = true;
    CHECK(s.collect(f, 100, out));
    for (int i = 0; i < 100; ++i) { s.reset(); f.time += 300'000; CHECK(s.collect(f, 100, out)); }
    CHECK(s.pending_count() == 8 && f.begins == 8 && out.terrain_count == 0);
    f.wait = false; CHECK(s.collect(f, 100, out)); CHECK(out.terrain_count == 0 && f.begins == 16);
    CHECK(s.collect(f, 100, out)); CHECK(out.terrain_count == 8);
}
void budget() {
    Fake f; WorldStream s; HostFrame out{}; f.call_cost = 300;
    CHECK(s.collect(f, 100, out)); CHECK(f.begins == 1 && f.polls == 0);
    f.call_cost = 600; CHECK(s.collect(f, 100, out)); CHECK(f.begins == 1 && f.polls == 0);
    f.call_cost = 0; CHECK(s.collect(f, 100, out)); CHECK(f.begins <= 9 && f.polls == 1);
}
void invalid_samples() {
    Fake f; WorldStream s; HostFrame out{}; CHECK(s.collect(f, 100, out));
    f.bad_hit = true; CHECK(s.collect(f, 100, out)); CHECK(out.terrain_count == 0);
    f.bad_hit = false; f.miss = true; CHECK(s.collect(f, 100, out)); CHECK(out.terrain_count == 0);
}
void expiry() {
    Fake f; WorldStream s; HostFrame out{}; CHECK(s.collect(f, 100, out)); CHECK(s.collect(f, 100, out));
    CHECK(out.terrain_count == 8); f.wait = true; f.time += 600'000;
    CHECK(s.collect(f, 100, out)); CHECK(out.terrain_count == 0);
    CHECK(f.begins == 16); f.time = 100; CHECK(s.collect(f, 100, out)); CHECK(out.anchor == 101);
}
void failure_and_overflow() {
    Fake f; WorldStream s; HostFrame out{}; f.fail_begin = true;
    CHECK(s.collect(f, 100, out)); CHECK(s.pending_count() == 0 && f.begins == 8);
    CHECK(!s.collect(f, 0, out)); CHECK(!valid(out));
    CHECK(s.collect(f, std::numeric_limits<std::uint64_t>::max(), out)); s.reset();
    CHECK(!s.collect(f, 100, out)); CHECK(!valid(out));
}
void native_abi() {
    using namespace blockwater::rdr2;
    CHECK(native_argument(1.5f) == 0x3fc00000);
    CHECK(native_argument(-1) == 0xffffffffULL);
    CHECK(native_argument(true) == 1);
    NativeVector v{1, 77, 2, 88, 3, 99};
    CHECK(native_argument(&v) == reinterpret_cast<std::uintptr_t>(&v));
    CHECK(!native_result<float>(nullptr)); CHECK(!native_vector(nullptr));
    const std::uint64_t slots[] = {0xffffFFFF3f800000ULL, 0x40000000ULL, 0x40400000ULL};
    const auto result = native_vector(slots); CHECK(result && result->x == 1 && result->y == 2 && result->z == 3);
    CHECK(native_result<float>(slots) == 1); CHECK(v.value().z == 3);
}
}
int main() {
    try {
        basic(); invalid_view(); transitions(); pending_timeout(); budget(); invalid_samples(); expiry();
        failure_and_overflow(); native_abi();
        std::cout << "PASS 9 world-stream and native-value cases\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
