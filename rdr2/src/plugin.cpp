#include "scripthook.hpp"
#include "native_gate.hpp"
#include "native_world.hpp"
#include "blockwater/mapping.hpp"
#include <limits>
#include <optional>
#include <stdexcept>

namespace {

class HostLease {
public:
    HostLease() {
        handle_ = CreateMutexW(nullptr, FALSE, L"Local\\Blockwater_rdr2_host");
        const auto error = GetLastError();
        if (!handle_ || error == ERROR_ALREADY_EXISTS) {
            if (handle_) CloseHandle(handle_);
            handle_ = nullptr;
            throw std::runtime_error("RDR2 host lease unavailable");
        }
    }
    ~HostLease() { if (handle_) CloseHandle(handle_); }
    HostLease(const HostLease&) = delete;
    HostLease& operator=(const HostLease&) = delete;
private:
    HANDLE handle_ = nullptr;
};

class Runtime {
public:
    Runtime() {
        // Opening first permits reload while a guest retains the mapping. If an
        // existing region is invalid, exclusive creation also fails without rewriting it.
        try { mapping_.emplace("rdr2", blockwater::Mapping::Mode::Open); }
        catch (const std::runtime_error&) {
            mapping_.emplace("rdr2", blockwater::Mapping::Mode::Create);
        }
        auto& shared = mapping_->bridge();
        const auto previous = std::atomic_ref<std::uint64_t>(shared.host.epoch).load();
        if (previous == std::numeric_limits<std::uint64_t>::max())
            throw std::runtime_error("RDR2 host epoch exhausted");
        const auto clock = blockwater::now_us();
        const auto epoch = clock > previous ? clock : previous + 1;
        session_.emplace(shared, epoch);
    }
    void tick() {
        if (session_->phase() == blockwater::HostPhase::Refused) return;
        const auto phase = session_->tick(blockwater::rdr2::read_context());
        if (phase != blockwater::HostPhase::Connected) {
            world_.reset();
            return;
        }
        world_.collect(source_, session_->epoch(), frame_);
        // The script thread is the sole publisher; a failed collection writes an invalid snapshot.
        mapping_->bridge().host_frame.publish(frame_);
    }
private:
    HostLease lease_;
    std::optional<blockwater::Mapping> mapping_;
    std::optional<blockwater::HostSession> session_;
    blockwater::rdr2::NativeWorld source_;
    blockwater::WorldStream world_;
    blockwater::HostFrame frame_{};
};

Runtime* runtime = nullptr;

void script_main() {
    try {
        runtime = new Runtime;
        for (;;) {
            runtime->tick();
            scriptWait(0);
        }
    } catch (...) {
        delete runtime;
        runtime = nullptr;
        OutputDebugStringA("Blockwater: native host initialization or session failed.\n");
        for (;;) scriptWait(0);
    }
}

} // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        scriptRegister(module, script_main);
    } else if (reason == DLL_PROCESS_DETACH && reserved == nullptr) {
        // Unregister the script before releasing its transport. No game natives,
        // worker joins or file operations are allowed under the loader lock.
        scriptUnregister(module);
        delete runtime;
        runtime = nullptr;
    }
    return TRUE;
}
