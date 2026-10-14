#pragma once
#include "../src/scripthook.hpp"

#ifdef BW_FAKE_BUILD
#define BW_TEST_API __declspec(dllexport)
#else
#define BW_TEST_API __declspec(dllimport)
#endif

enum FakeContext {
    Story = 0, Network0 = 1, Network1 = 2, Network2 = 4,
    Paused = 8, NoPlayer = 16, Dead = 32, Cutscene = 64, NullNative = 128
};
struct FakeCounts {
    unsigned registrations;
    unsigned unregistrations;
    unsigned native_calls;
    unsigned errors;
    unsigned yields;
};
BW_TEST_API void BWTestReset();
BW_TEST_API void BWTestContext(unsigned flags);
BW_TEST_API void BWTestAdvance();
BW_TEST_API FakeCounts BWTestCounts();
