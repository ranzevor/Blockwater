# Compatibility and verification

| Component | Environment / version | Verified state |
| --- | --- | --- |
| Native bridge core | Linux x86-64, GCC 14.2.0, C++20 | Release and sanitizer tests passed on 2026-10-14 |
| Native bridge core | Linux x86-64, Clang 17, C++20 | Release tests passed on 2026-10-14 |
| Native bridge core | Windows x64, MSVC, `windows-2025` runner | Release build and CTest passed on 2026-10-14 in [CI run 3](https://github.com/ranzevor/Blockwater/actions/runs/38587538291) |
| RDR2 ScriptHook plugin | RDR2 Story Mode | Not implemented or tested in Blockwater |
| Minecraft client | Java Edition / Fabric | Not implemented or tested in Blockwater |
| GPU color/depth compositor | DX12 / ReShade | Not implemented or tested in Blockwater |

[CI run 3](https://github.com/ranzevor/Blockwater/actions/runs/38587538291) tested source commit `38c082fee28d6682766e41af28443ca482c35ab8`. Both the Ubuntu and Windows jobs passed their configure, build and CTest steps, including the platform-specific child-process IPC scenario. No game binaries, accounts or network services are required for native core tests.

A claim of game compatibility requires recorded versions for RDR2, ScriptHookRDR2, Minecraft/Fabric, Windows, GPU driver and graphics API, plus representative RDR2 Story Mode gameplay tests. See [milestones](milestones.md). Neither a reference implementation's working versions nor a successfully compiled native core establishes Blockwater game support.
