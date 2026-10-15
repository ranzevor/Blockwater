#include "fake_scripthook.hpp"
#include "blockwater/link.hpp"
#include "blockwater/mapping.hpp"
#include "../../tests/test.hpp"
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string>

using namespace blockwater;

namespace {
class Module {
public:
    explicit Module(const wchar_t* path) {
        handle_ = LoadLibraryExW(path, nullptr,
            LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        if (!handle_) throw std::runtime_error("LoadLibraryExW failed: " + std::to_string(GetLastError()));
    }
    ~Module() { if (handle_) FreeLibrary(handle_); }
    void unload() {
        REQUIRE(handle_ != nullptr);
        REQUIRE(FreeLibrary(handle_) != 0);
        handle_ = nullptr;
    }
private:
    HMODULE handle_ = nullptr;
};
void advance(Endpoint* guest = nullptr) {
    if (guest) { Sleep(2); guest->heartbeat(); }
    BWTestAdvance();
    REQUIRE(BWTestCounts().errors == 0);
}
void attach(Endpoint& guest) {
    guest.announce(200);
    advance();
    advance(&guest);
}
void expect_rejection(Endpoint& guest, std::uint64_t id) {
    Event request{};
    request.id = id;
    request.kind = EventKind::Explosion;
    request.magnitude = 4;
    REQUIRE(guest.send(request));
    advance(&guest);
    Event response{};
    REQUIRE(guest.receive(response));
    REQUIRE(response.id == id && response.kind == EventKind::Rejected);
    REQUIRE(response.payload == static_cast<std::uint32_t>(EventKind::Explosion));
    HostFrame frame{};
    REQUIRE(guest.read(frame));
    REQUIRE(frame.camera.position.x == 100 && frame.camera.position.z == 12);
    REQUIRE(frame.camera.yaw == 25 && frame.camera.pitch == -5 && frame.camera.roll == 2);
    REQUIRE(frame.camera.fov == 60 && frame.player_feet.z == 10);
    REQUIRE(frame.anchor != 0 && frame.timestamp_us != 0 && frame.entity_count == 0);
}
void expect_lease_released() {
    const auto lease = OpenMutexW(SYNCHRONIZE, FALSE, L"Local\\Blockwater_rdr2_host");
    if (lease) CloseHandle(lease);
    REQUIRE(lease == nullptr);
}
void expect_no_mapping() {
    bool absent = false;
    try { Mapping unexpected("rdr2", Mapping::Mode::Open); }
    catch (const std::runtime_error&) { absent = true; }
    REQUIRE(absent);
}
void world_scenario(Endpoint& guest, const std::wstring& scenario) {
    HostFrame before{};
    REQUIRE(guest.read(before));
    unsigned flags = Story;
    if (scenario == L"world_invalid") flags = BadCamera;
    if (scenario == L"world_bounds") flags = BadBounds;
    if (scenario == L"world_null_camera") flags = NullCamera;
    if (flags != Story) {
        BWTestContext(flags);
        advance(&guest);
        HostFrame frame{};
        REQUIRE(!guest.read(frame));
        BWTestContext(Story);
        advance(&guest);
        REQUIRE(guest.read(frame) && frame.anchor > before.anchor && frame.terrain_count == 0);
        return;
    }
    if (scenario == L"world_teleport") {
        BWTestContext(Teleport);
        advance(&guest);
        HostFrame frame{};
        REQUIRE(guest.read(frame) && frame.anchor > before.anchor);
        REQUIRE(frame.host_origin.x == 200 && frame.terrain_count == 0);
        for (int i = 0; i < 20; ++i) advance(&guest);
        REQUIRE(guest.read(frame) && frame.terrain_count > 0);
        for (std::uint32_t i = 0; i < frame.terrain_count; ++i) REQUIRE(frame.terrain[i].point.x >= 192);
        BWTestContext(Story);
        return;
    }
    BWTestContext(Paused);
    advance();
    HostFrame frame{};
    REQUIRE(!guest.read(frame));
    const auto calls = BWTestCounts().world_calls;
    advance();
    REQUIRE(BWTestCounts().world_calls == calls);
    if (scenario == L"world_pending") flags = PendingGround;
    if (scenario == L"world_miss") flags = NoGround;
    if (scenario == L"world_bad_ground") flags = BadGround;
    BWTestContext(flags);
    advance(&guest); advance(&guest);
    for (int i = 0; i < 30; ++i) advance(&guest);
    REQUIRE(guest.read(frame) && frame.anchor > before.anchor);
    REQUIRE(BWTestCounts().ground_peak <= 8);
    if (flags != Story) REQUIRE(frame.terrain_count == 0);
    else REQUIRE(frame.terrain_count > 0);
    BWTestContext(Story);
}

void run(const wchar_t* path, const std::wstring& scenario) {
    BWTestReset();
    std::optional<Mapping> retained;
    if (scenario == L"corrupt") {
        retained.emplace("rdr2", Mapping::Mode::Create);
        retained->bridge().signature = 17;
        retained->bridge().host.epoch = 333;
    }
    unsigned initial = Story;
    if (scenario == L"network0") initial = Network0;
    if (scenario == L"network1") initial = Network1;
    if (scenario == L"network2") initial = Network2;
    if (scenario == L"network_with_null") initial = Network1 | NullNative;
    if (scenario == L"paused") initial = Paused;
    if (scenario == L"unavailable") initial = NoPlayer;
    if (scenario == L"dead") initial = Dead;
    if (scenario == L"cutscene") initial = Cutscene;
    if (scenario == L"null_native") initial = NullNative;
    BWTestContext(initial);
    Module plugin(path);
    REQUIRE(BWTestCounts().registrations == 1);
    REQUIRE(BWTestCounts().native_calls == 0);
    if (scenario == L"early_unload") {
        expect_no_mapping();
        plugin.unload();
        REQUIRE(BWTestCounts().unregistrations == 1);
        expect_lease_released();
        return;
    }
    advance();
    if (scenario == L"corrupt") {
        REQUIRE(retained->bridge().signature == 17);
        REQUIRE(retained->bridge().host.epoch == 333);
        REQUIRE(BWTestCounts().native_calls == 0);
        plugin.unload();
        expect_lease_released();
        return;
    }
    retained.emplace("rdr2", Mapping::Mode::Open);
    Endpoint guest(retained->bridge(), Role::Guest);
    if (initial & (Network0 | Network1 | Network2)) {
        REQUIRE(guest.peer_state() == PeerState::Refused);
        REQUIRE(guest.peer_reason() == RejectReason::Multiplayer);
        const auto calls = BWTestCounts().native_calls;
        BWTestContext(Story);
        advance();
        REQUIRE(BWTestCounts().native_calls == calls);
        REQUIRE(guest.peer_state() == PeerState::Refused);
    } else {
        if (initial != Story) {
            REQUIRE(guest.peer_state() == PeerState::Closed);
            BWTestContext(Story);
            advance();
        }
        REQUIRE(guest.peer_state() == PeerState::Ready);
        attach(guest);
        expect_rejection(guest, 1);
        const auto old_epoch = guest.peer_epoch();
        if (scenario.rfind(L"world_", 0) == 0) {
            world_scenario(guest, scenario);
        } else if (scenario == L"paused") {
            BWTestContext(Paused);
            advance();
            REQUIRE(guest.peer_state() == PeerState::Closed);
            Event old{}; old.id = 90;
            REQUIRE(guest.send(old));
            BWTestContext(Story);
            advance();
            REQUIRE(guest.peer_epoch() > old_epoch);
            advance(&guest);
            expect_rejection(guest, 2);
        } else if (scenario == L"disconnect" || scenario == L"restart" || scenario == L"host_stall") {
            Event old{}; old.id = 90;
            REQUIRE(guest.send(old));
            if (scenario == L"disconnect") {
                std::atomic_ref<std::uint64_t>(retained->bridge().guest.heartbeat_us)
                    .store(now_us() - kHeartbeatTimeoutUs - 1000);
            } else if (scenario == L"restart") guest.announce(201);
            else {
                Sleep(2100);
                guest.heartbeat();
            }
            advance();
            REQUIRE(guest.peer_epoch() > old_epoch);
            advance(&guest);
            advance(&guest);
            expect_rejection(guest, 2);
        } else if (scenario == L"reload") {
            Event old{}; old.id = 90;
            REQUIRE(guest.send(old));
            const auto calls = BWTestCounts().native_calls;
            plugin.unload();
            REQUIRE(guest.peer_state() == PeerState::Closed);
            REQUIRE(BWTestCounts().native_calls == calls);
            expect_lease_released();
            {
                Module reloaded(path);
                advance();
                REQUIRE(guest.peer_epoch() > old_epoch);
                advance(&guest);
                expect_rejection(guest, 2);
            }
            REQUIRE(guest.peer_state() == PeerState::Closed);
            REQUIRE(BWTestCounts().registrations == 2);
            REQUIRE(BWTestCounts().unregistrations == 2);
            expect_lease_released();
            retained.reset();
            expect_no_mapping();
            return;
        } else if (scenario == L"duplicate") {
            const auto duplicate = std::filesystem::path(path).parent_path() / L"duplicate.asi";
            REQUIRE(CopyFileW(path, duplicate.c_str(), TRUE) != 0);
            {
                Module second(duplicate.c_str());
                advance(&guest);
                REQUIRE(guest.peer_epoch() == old_epoch);
                expect_rejection(guest, 2);
            }
            REQUIRE(DeleteFileW(duplicate.c_str()) != 0);
            REQUIRE(guest.peer_state() == PeerState::Ready);
            expect_rejection(guest, 3);
        }
    }
    const auto calls = BWTestCounts().native_calls;
    plugin.unload();
    REQUIRE(BWTestCounts().native_calls == calls);
    REQUIRE(BWTestCounts().registrations == BWTestCounts().unregistrations);
    REQUIRE(guest.peer_state() == PeerState::Closed || guest.peer_state() == PeerState::Refused);
    expect_lease_released();
    retained.reset();
    expect_no_mapping();
}
}

int wmain(int argc, wchar_t** argv) {
    if (argc != 3) return 2;
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
    try {
        run(argv[1], argv[2]);
        REQUIRE(BWTestCounts().errors == 0);
        std::wcout << L"PASS " << argv[2] << L"\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
