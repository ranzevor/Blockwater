#define BW_FAKE_BUILD
#include "fake_scripthook.hpp"
#include <array>

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
std::array<UINT64, 4> arguments{};
unsigned argument_count = 0;
UINT64 result = 0;

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
    result = 0;
    // An independent read-only allowlist detects unknown calls and wrong arguments.
    switch (hash) {
    case 0xCA97246103B63917ULL: result = (context & Network0) != 0; break;
    case 0x9DE624D2FC4B603FULL: result = (context & Network1) != 0; break;
    case 0x10FAB35428CCC9D7ULL: result = (context & Network2) != 0; break;
    case 0x535384D6067BA42EULL: result = (context & Paused) != 0; break;
    case 0x096275889B8E0EE0ULL: result = (context & NoPlayer) ? 0 : 7; break;
    case 0xD42BD6EB2E0F1677ULL:
        expected_arguments = 1; expected_value = 7; result = 1; break;
    case 0x7D5B1F88E7504BBAULL:
        expected_arguments = 1; expected_value = 7; result = (context & Dead) != 0; break;
    case 0x217E9DC48139933DULL: result = 0; break;
    case 0x7964097FCE4C244BULL:
        expected_arguments = 1; expected_value = 0; result = (context & Cutscene) == 0; break;
    default: ++counts.errors; return nullptr;
    }
    if (argument_count != expected_arguments ||
        (expected_arguments && arguments[0] != expected_value)) ++counts.errors;
    if ((context & NullNative) && hash == 0xCA97246103B63917ULL) return nullptr;
    return &result;
}

void BWTestReset() {
    thread_id = GetCurrentThreadId();
    if (!scheduler) scheduler = ConvertThreadToFiber(nullptr);
    counts = {};
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
