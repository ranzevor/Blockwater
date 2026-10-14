#define BW_FAKE_BUILD
#include "fake_scripthook.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

namespace {
struct Script {
    HMODULE module = nullptr;
    void (*entry)() = nullptr;
    void* fiber = nullptr;
};
std::array<Script, 2> scripts{};
Script* current = nullptr;
void* scheduler = nullptr;
DWORD thread_id = 0;
unsigned context = Story;
FakeCounts counts{};
UINT64 hash = 0;
std::array<UINT64, 10> arguments{};
unsigned argument_count = 0;
std::array<UINT64, 3> result{};
struct Probe { int handle = 0; float x = 0, y = 0, z = 0; HMODULE owner = nullptr; };
std::array<Probe, 8> probes{};
int next_probe = 1;

void WINAPI run_script(void* parameter) {
    auto* script = static_cast<Script*>(parameter);
    script->entry();
    ++counts.errors;
    for (;;) SwitchToFiber(scheduler);
}
void check_thread() {
    if (GetCurrentThreadId() != thread_id || !current ||
        GetCurrentFiber() != current->fiber) ++counts.errors;
}
void require(bool condition) { if (!condition) ++counts.errors; }
float as_float(UINT64 v) { float out{}; std::memcpy(&out, &v, 4); return out; }
void vector_at(void* target, float x, float y, float z) {
    auto* bytes = static_cast<unsigned char*>(target);
    std::memcpy(bytes, &x, 4); std::memcpy(bytes+8, &y, 4); std::memcpy(bytes+16, &z, 4);
}
void set_float(float value) { std::memcpy(result.data(), &value, 4); }
PUINT64 world_call() {
    ++counts.world_calls;
    require((context & (Network0 | Network1 | Network2 | Paused | NoPlayer | Dead | Cutscene)) == 0);
    const float x = (context & Teleport) ? 200.0f : 100.0f;
    switch (hash) {
    case 0x5352E025EC2B416FULL:
        require(argument_count == 0);
        if (context & NullCamera) return nullptr;
        vector_at(result.data(), x, 200, 12); break;
    case 0x602685BD85DD26CAULL:
        require(argument_count == 1 && arguments[0] == 2);
        vector_at(result.data(), -5, 2, 25); break;
    case 0x04AF77971E508F6AULL:
        require(argument_count == 0);
        set_float((context & BadCamera) ? std::numeric_limits<float>::quiet_NaN() : 60.0f); break;
    case 0xDA76A9F39210D365ULL:
        require(argument_count == 1 && arguments[0] == 7); result[0] = 1234; break;
    case 0xDCB8DDD5D054A7E7ULL:
        require(argument_count == 3 && arguments[0] == 1234 && arguments[1] != 0 && arguments[2] != 0);
        if (!(context & BadBounds)) {
            vector_at(reinterpret_cast<void*>(arguments[1]), -0.3f, -0.3f, -1);
            vector_at(reinterpret_cast<void*>(arguments[2]), 0.3f, 0.3f, 1);
        }
        return nullptr;
    case 0x1899F328B0E12848ULL:
        require(argument_count == 4 && arguments[0] == 7 && arguments[1] == 0 && arguments[2] == 0);
        require(as_float(arguments[3]) == -1 && (arguments[3] >> 32) == 0);
        vector_at(result.data(), x, 200, 11+as_float(arguments[3])); break;
    case 0x7EE9F5D83DD4F90EULL: {
        require(argument_count == 9 && arguments[6] == 1 && arguments[7] == 7 && arguments[8] == 4);
        for (unsigned i = 0; i < 6; ++i) require((arguments[i] >> 32) == 0 && std::isfinite(as_float(arguments[i])));
        require(as_float(arguments[0]) == as_float(arguments[3]) && as_float(arguments[1]) == as_float(arguments[4]));
        require(as_float(arguments[2]) > as_float(arguments[5]));
        for (auto& p : probes) if (p.handle == 0) {
            p = {next_probe++, as_float(arguments[0]), as_float(arguments[1]),
                 (as_float(arguments[2])+as_float(arguments[5]))/2, current->module};
            result[0] = static_cast<UINT64>(p.handle);
            unsigned live = 0;
            for (const auto& q : probes) if (q.handle) ++live;
            counts.ground_peak = std::max(counts.ground_peak, live);
            return result.data();
        }
        ++counts.errors; break;
    }
    case 0xEDE8AC7C5108FB1DULL: {
        require(argument_count == 5);
        for (unsigned i = 1; i < 5; ++i) require(arguments[i] != 0);
        for (auto& p : probes) if (p.handle == static_cast<int>(arguments[0])) {
            if (context & PendingGround) { result[0] = 1; return result.data(); }
            const std::int32_t hit = (context & NoGround) ? 0 : 1, entity = 0;
            std::memcpy(reinterpret_cast<void*>(arguments[1]), &hit, 4);
            vector_at(reinterpret_cast<void*>(arguments[2]), p.x, p.y, p.z);
            vector_at(reinterpret_cast<void*>(arguments[3]), 0, 0,
                      (context & BadGround) ? std::numeric_limits<float>::quiet_NaN() : 1.0f);
            std::memcpy(reinterpret_cast<void*>(arguments[4]), &entity, 4);
            p = {}; result[0] = 2; return result.data();
        }
        break;
    }
    default: ++counts.errors; return nullptr;
    }
    return result.data();
}
}

void scriptRegister(HMODULE module, void (*entry)()) {
    ++counts.registrations;
    for (auto& script : scripts) {
        if (!script.module) { script = {module, entry, nullptr}; return; }
    }
    ++counts.errors;
}
void scriptUnregister(HMODULE module) {
    ++counts.unregistrations;
    for (auto& script : scripts) {
        if (script.module != module) continue;
        if (script.fiber) DeleteFiber(script.fiber);
        script = {};
        // Engine-owned asynchronous query state ends with this script context in the harness.
        for (auto& p : probes) if (p.owner == module) p = {};
        return;
    }
    ++counts.errors;
}
void scriptWait(DWORD time) {
    check_thread();
    if (time != 0) ++counts.errors;
    ++counts.yields;
    SwitchToFiber(scheduler);
}
void nativeInit(UINT64 value) {
    check_thread();
    hash = value;
    argument_count = 0;
    arguments.fill(0);
}
void nativePush64(UINT64 value) {
    check_thread();
    if (argument_count >= arguments.size()) { ++counts.errors; return; }
    arguments[argument_count++] = value;
}
PUINT64 nativeCall() {
    check_thread();
    ++counts.native_calls;
    unsigned expected_arguments = 0;
    UINT64 expected_value = 0;
    result.fill(0);
    switch (hash) {
    case 0xCA97246103B63917ULL: result[0] = (context & Network0) != 0; break;
    case 0x9DE624D2FC4B603FULL: result[0] = (context & Network1) != 0; break;
    case 0x10FAB35428CCC9D7ULL: result[0] = (context & Network2) != 0; break;
    case 0x535384D6067BA42EULL: result[0] = (context & Paused) != 0; break;
    case 0x096275889B8E0EE0ULL: result[0] = (context & NoPlayer) ? 0 : 7; break;
    case 0xD42BD6EB2E0F1677ULL:
        expected_arguments = 1; expected_value = 7; result[0] = 1; break;
    case 0x7D5B1F88E7504BBAULL:
        expected_arguments = 1; expected_value = 7; result[0] = (context & Dead) != 0; break;
    case 0x217E9DC48139933DULL: result[0] = 0; break;
    case 0x7964097FCE4C244BULL:
        expected_arguments = 1; expected_value = 0; result[0] = (context & Cutscene) == 0; break;
    default: return world_call();
    }
    if (argument_count != expected_arguments ||
        (expected_arguments && arguments[0] != expected_value)) ++counts.errors;
    if ((context & NullNative) && hash == 0xCA97246103B63917ULL) return nullptr;
    return result.data();
}

void BWTestReset() {
    thread_id = GetCurrentThreadId();
    if (!scheduler) scheduler = ConvertThreadToFiber(nullptr);
    counts = {};
    probes = {};
    next_probe = 1;
    if (!scheduler) ++counts.errors;
    context = Story;
}
void BWTestContext(unsigned flags) { context = flags; }
FakeCounts BWTestCounts() { return counts; }
void BWTestAdvance() {
    for (auto& script : scripts) {
        if (!script.module) continue;
        if (!script.fiber) script.fiber = CreateFiber(128 * 1024, run_script, &script);
        if (!script.fiber) { ++counts.errors; return; }
        current = &script;
        SwitchToFiber(script.fiber);
        current = nullptr;
    }
}
