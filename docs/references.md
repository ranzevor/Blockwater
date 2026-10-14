# Related work and dependency boundaries

- [universal-modder / Minecraft × GTA V passthrough](https://github.com/rehan-remade/universal-modder/tree/main/examples/minecraft-gta5-passthrough) — reference for Fabric runtime bridge, frame export, depth compositing, event mapping and fake-host tests. MIT License, Copyright (c) 2026 Rehan and universal-modder contributors.
- [Garry's Redemption](https://github.com/codeByAlexff/garrys-redemption) — reference for verified RDR2 native calls, multiplayer exclusion, terrain collision, player ownership and scene/object integration. MIT License, Copyright (c) 2026 Garry's Redemption contributors.
- [RDR3 Native DB](https://alloc8or.re/rdr3/nativedb/) — RDR2 native signatures and hash evidence.
- [ScriptHookRDR2](https://www.dev-c.com/rdr2/scripthookrdr2/) — native mod SDK, supplied separately under its own terms.
- [Fabric](https://fabricmc.net/) — Minecraft client modding platform.
- [ReShade](https://reshade.me/) — graphics add-on API and shader integration.

The B0 core is newly implemented and contains no copied upstream source files, game assets, embedded game executables or proprietary SDK binaries. Any future upstream code incorporated into the repository must retain its required license text and provenance. Users supply their own licensed game installations.

## Incorporated interface declarations

`rdr2/src/scripthook.hpp` adapts the six imported function declarations from Garry's Redemption commit `4ada8a12a85d6f30e2c6485970792cc1b837c6ac`, path `rdr2/src/scripthook.h` (blob `e2e42dd27480275f96793fd2b249af81a1b3256c`). The matching decorated symbols are listed in `rdr2/scripthook.def`. The original [MIT license](../LICENSES/Garrys-Redemption.txt) is retained and included in native-plugin staging.

The read-only native register and source-level validation boundary are recorded in [RDR2 native host](rdr2-host.md#native-interface-register). The loader uses [Microsoft DLL lifecycle guidance](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-best-practices) to separate script work from DLL attach/detach. The test runtime is project test code, not a redistributable substitute for the proprietary game runtime.
