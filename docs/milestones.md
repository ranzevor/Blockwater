# Integration milestones

| Stage | Acceptance condition | Current evidence |
| --- | --- | --- |
| B0 — Bridge core | Native shared memory on Windows/POSIX, consistent frame snapshots, lossless bounded event flow, two-process test, session recovery, reversible fake gameplay CPU depth compositor, and bilingual startup language catalogs | Source implemented. Linux C++20 build, POSIX child-process IPC and automated tests passed on 2026-10-14. Windows child-process IPC case added for CI; runner verification and game interoperability remain unverified. |
| B1 — RDR2 adapter | ScriptHookRDR2 plugin enters Story Mode only, streams camera/ground, converts verified coordinates, creates and removes an actual collision object and safely unloads | GameHost contract and fake implementation are built. Native plugin and game test remain. |
| B2 — Minecraft guest | Fabric client publishes camera/depth/HUD, consumes sampled terrain, supplies block and explosion events, reconnects safely | Shared-memory protocol prepared; Fabric integration remains. |
| B3 — Rendering | Real DX12 host frame and depth, depth-aware Minecraft compositing, calibration and view reprojection | CPU reference compositor and occlusion tests passed; DX12/ReShade pipeline remains. |
| B4 — Playable scene | In a fixed Story Mode area, player can place and remove MC blocks that block at least one RDR2 NPC/vehicle, trigger corresponding TNT and observe correct world occlusion | Requires B1–B3 and repeatable game-run test. |
| B5 — Extended gameplay | Damage mediation, MC hostile mob proxies, equipment use and persistent lifecycle tested through game restarts | Native behaviour is not yet implemented. |

## Required game validation

Record the exact RDR2 executable version, ScriptHookRDR2 version, graphics API, Minecraft/Fabric versions, renderer and operating-system version for each accepted gameplay run. For the first playable scene check normal walking, camera orbit, foreground/background occlusion, block add/remove, actor/vehicle collision, TNT once-only behaviour, scene loading/unloading, and reconnect with stale events. An automated fake-host pass does not substitute for these checks.
