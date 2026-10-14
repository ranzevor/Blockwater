// SPDX-License-Identifier: MIT
// Adapted from Garry's Redemption; see LICENSES/Garrys-Redemption.txt.
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#ifndef BW_SCRIPTHOOK_API
#define BW_SCRIPTHOOK_API __declspec(dllimport)
#endif

// These C++ signatures determine the imported MSVC x64 decorated names.
BW_SCRIPTHOOK_API void scriptWait(DWORD time);
BW_SCRIPTHOOK_API void scriptRegister(HMODULE module, void (*script_main)());
BW_SCRIPTHOOK_API void scriptUnregister(HMODULE module);
BW_SCRIPTHOOK_API void nativeInit(UINT64 hash);
BW_SCRIPTHOOK_API void nativePush64(UINT64 value);
BW_SCRIPTHOOK_API PUINT64 nativeCall();
