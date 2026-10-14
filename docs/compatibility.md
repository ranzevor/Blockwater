# Compatibility and verification

| Component | Environment / version | Verified state |
| --- | --- | --- |
| Native bridge core | Linux x86-64, GCC 14.2.0, C++20 | Release and sanitizer tests passed on 2026-10-14 |
| Native bridge core | Linux x86-64, Clang 17, C++20 | Release tests passed on 2026-10-14 |
| Native bridge core | Windows x64, MSVC | Automated CI configured; Windows execution not yet verified in this repository |
| RDR2 ScriptHook plugin | RDR2 Story Mode | Not implemented or tested in Blockwater |
| Minecraft client | Java Edition / Fabric | Not implemented or tested in Blockwater |
| GPU color/depth compositor | DX12 / ReShade | Not implemented or tested in Blockwater |

The Windows test executable contains a child-process IPC scenario to be executed by the Windows CI runner. No game binaries, accounts or network services are required for native core tests.

A claim of game compatibility requires recorded versions for RDR2, ScriptHookRDR2, Minecraft/Fabric, Windows, GPU driver and graphics API, plus representative RDR2 Story Mode gameplay tests. See [milestones](milestones.md). Neither a reference implementation's working versions nor a successfully compiled native core establishes Blockwater game support.
