# Compatibility and verification

| Component | Environment / version | Verified state |
| --- | --- | --- |
| Native bridge core | Linux x86-64, GCC 14.2.0, C++20 | Release and sanitizer tests passed on 2026-10-14 |
| Native bridge core | Linux x86-64, Clang 17, C++20 | Release tests passed on 2026-10-14 |
| Native bridge core | Windows x64, MSVC, `windows-2025` runner | Release build and CTest passed on 2026-10-14 in [CI run 3](https://github.com/ranzevor/Blockwater/actions/runs/38587538291) |
| RDR2 ScriptHook plugin | Windows x64/MSVC; RDR2 Story Mode target | Lifecycle and read-only camera/player/terrain acquisition implemented. Plugin build and 31 CTest entries passed in [CI run 8](https://github.com/ranzevor/Blockwater/actions/runs/38656719116). Genuine ScriptHook and game behavior remain unverified; see [world acquisition](rdr2-world.md). |
| Minecraft client | Java Edition / Fabric | Not implemented or tested in Blockwater |
| GPU color/depth compositor | DX12 / ReShade | Not implemented or tested in Blockwater |

[CI run 3](https://github.com/ranzevor/Blockwater/actions/runs/38587538291) tested source commit `38c082fee28d6682766e41af28443ca482c35ab8`. Both the Ubuntu and Windows jobs passed their configure, build and CTest steps, including the platform-specific child-process IPC scenario. No game binaries, accounts or network services are required for native core tests.

The world-acquisition evidence in CI run 8 applies to source `8c4aaba9ce27d5121a4ffb5bfa24e09938d6325e`, with Windows Server 2025 and MSVC 19.51.36260.0. Its `.asi` runs against an isolated test runtime, not the genuine game. Pending-query reclamation on real unload, camera/FOV calibration, player-foot reference accuracy and game frame costs remain unverified.

A claim of game compatibility requires recorded versions for RDR2, ScriptHookRDR2, Minecraft/Fabric, Windows, GPU driver and graphics API, plus representative RDR2 Story Mode gameplay tests. See [milestones](milestones.md). Neither a reference implementation's working versions nor a successfully compiled native core establishes Blockwater game support.
