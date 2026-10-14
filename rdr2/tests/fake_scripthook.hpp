#pragma once
#include "../src/scripthook.hpp"

#ifdef BW_FAKE_BUILD
#define BW_TEST_API __declspec(dllexport)
#else
#define BW_TEST_API __declspec(dllimport)
#endif

enum FakeContext {
    Story = 0, Network0 = 1, Network1 = 2, Network2 = 4,
    Paused = 8, NoPlayer = 16, Dead = 32, Cutscene = 64, NullNative = 128,
    BadCamera = 256, BadBounds = 512, PendingGround = 1024, NoGround = 2048,
    Teleport = 4096, NullCamera = 8192, BadGround = 16384
};
struct FakeCounts {
    unsigned registrations;
    unsigned unregistrations;
    unsigned native_calls;
    unsigned errors;
    unsigned yields;
    unsigned world_calls;
    unsigned ground_peak;
};
BW_TEST_API void BWTestReset();
BW_TEST_API void BWTestContext(unsigned flags);
BW_TEST_API void BWTestAdvance();
BW_TEST_API FakeCounts BWTestCounts();
