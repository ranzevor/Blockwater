#include "blockwater/link.hpp"
#include "blockwater/mapping.hpp"
#include "test.hpp"
#include <chrono>
#include <string>
#include <memory>
#include <thread>
#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

using namespace blockwater;

int run_ipc_child(const std::string& name) {
    try {
        Mapping opened(name, Mapping::Mode::Open);
        Endpoint guest(opened.bridge(), Role::Guest);
        guest.announce(456);
        HostFrame snapshot{};
        if (!guest.read(snapshot) || snapshot.frame_id != 999) return 2;
        Event event{};
        event.id = 345;
        event.kind = EventKind::Explosion;
        event.magnitude = 3;
        return guest.send(event) ? 0 : 3;
    } catch (...) { return 4; }
}

void test_ipc() {
    auto mem = std::make_unique<SharedBridge>();
    initialize(*mem);
    Endpoint host(*mem, Role::Host);
    Endpoint guest(*mem, Role::Guest);
    host.announce(10);
    REQUIRE(!guest.peer_ready());
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    host.heartbeat();
    REQUIRE(guest.peer_ready());
    REQUIRE(!host.peer_ready());
    guest.announce(20);
    REQUIRE(!host.peer_ready());
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    guest.heartbeat();
    REQUIRE(host.peer_ready());
    REQUIRE(!guest.peer_ready(now_us() + kHeartbeatTimeoutUs + 10));

    Event command{};
    command.id = 100;
    command.kind = EventKind::PlaceBlock;
    for (std::size_t i = 0; i < kEventCapacity; ++i) {
        command.id = i + 1;
        REQUIRE(guest.send(command));
    }
    REQUIRE(!guest.send(command));
    for (std::size_t i = 0; i < kEventCapacity; ++i) {
        Event got{};
        REQUIRE(host.receive(got));
        REQUIRE(got.id == i + 1);
        REQUIRE(got.sender_epoch == 20 && got.recipient_epoch == 10);
    }
    Event empty{};
    REQUIRE(!host.receive(empty));
    for (std::size_t i = 0; i < kEventCapacity * 4; ++i) {
        command.id = i + 1;
        REQUIRE(guest.send(command));
        REQUIRE(host.receive(empty));
        REQUIRE(empty.id == i + 1);
    }
    command.id = 0;
    REQUIRE(!guest.send(command));
    guest.refuse(RejectReason::Multiplayer);
    REQUIRE(host.peer_state() == PeerState::Refused);
    REQUIRE(host.peer_reason() == RejectReason::Multiplayer);
    guest.announce(21);
    REQUIRE(guest.refused());
    REQUIRE(host.peer_state() == PeerState::Refused);

#ifdef _WIN32
    const auto unique = "ipc_" + std::to_string(GetCurrentProcessId());
#else
    const auto unique = "ipc_" + std::to_string(getpid());
#endif
    {
        Mapping created(unique, Mapping::Mode::Create);
        bool exists = false;
        try { Mapping fail(unique, Mapping::Mode::Create); }
        catch (const std::runtime_error&) { exists = true; }
        REQUIRE(exists);
        Endpoint other(created.bridge(), Role::Host);
        HostFrame sent{};
        sent.flags = kStoryMode;
        sent.frame_id = 999;
        REQUIRE(other.publish(sent));
#ifndef _WIN32
        const pid_t child = fork();
        REQUIRE(child >= 0);
        if (child == 0) {
            try {
                Mapping opened(unique, Mapping::Mode::Open);
                Endpoint child_guest(opened.bridge(), Role::Guest);
                child_guest.announce(456);
                HostFrame read_back{};
                if (!child_guest.read(read_back) || read_back.frame_id != 999) _exit(2);
                Event response{};
                response.id = 345;
                response.kind = EventKind::Explosion;
                response.magnitude = 3;
                if (!child_guest.send(response)) _exit(3);
            } catch (...) { _exit(4); }
            _exit(0);
        }
        int status = 0;
        REQUIRE(waitpid(child, &status, 0) == child);
        REQUIRE(WIFEXITED(status) && WEXITSTATUS(status) == 0);
        Event result{};
        REQUIRE(other.receive(result));
        REQUIRE(result.id == 345 && result.kind == EventKind::Explosion);
#else
        wchar_t exe[MAX_PATH] = {};
        const auto len = GetModuleFileNameW(nullptr, exe, MAX_PATH);
        REQUIRE(len != 0 && len < MAX_PATH);
        const std::wstring child_arg(unique.begin(), unique.end());
        std::wstring command = L"\"" + std::wstring(exe) + L"\" --ipc-child " + child_arg;
        STARTUPINFOW startup{};
        startup.cb = sizeof(startup);
        PROCESS_INFORMATION process{};
        REQUIRE(CreateProcessW(exe, command.data(), nullptr, nullptr, FALSE,
            CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process) != 0);
        const auto wait_result = WaitForSingleObject(process.hProcess, 10000);
        if (wait_result == WAIT_TIMEOUT) TerminateProcess(process.hProcess, 5);
        DWORD child_code = 255;
        const auto got_status = GetExitCodeProcess(process.hProcess, &child_code);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        REQUIRE(wait_result == WAIT_OBJECT_0 && got_status && child_code == 0);
        Event result{};
        REQUIRE(other.receive(result));
        REQUIRE(result.id == 345 && result.kind == EventKind::Explosion);
#endif
    }
    bool removed = false;
    try { Mapping fail(unique, Mapping::Mode::Open); }
    catch (const std::runtime_error&) { removed = true; }
    REQUIRE(removed);
}
