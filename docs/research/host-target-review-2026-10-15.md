# Host target assessment — 2026-10-15

[简体中文](host-target-review-2026-10-15.zh-CN.md)

## Decision context

The project owner is leaning toward replacing RDR2 as the Minecraft integration target. This assessment compares alternatives; it does not select a new target or change the existing application contract. No further RDR2 gameplay implementation is included. Existing Blockwater source and tests remain at main `bdd2e93bcdbb25e5bbffcca3b33e8e5e6c2a9ce6`.

A target change does not require proving that RedCraft is inferior. The original objective is a compelling, playable open-source crossover with world occlusion and meaningful two-way interaction. Native collision is part of that objective, not a mandatory reason to retain RDR2.

## RedCraft collision follow-up

The [third inspection run](https://github.com/ranzevor/Blockwater/actions/runs/38668864600) succeeded against audit code `afc1b4df422ab0cdcdf74d91ad66c306ec4fc96a`. It verified the same [3.0.0 release](https://github.com/Keel62155/Minecraft-X-Games/releases/tag/3.0.0-RedCraft) with SHA-256 `f003eb89654b7d3ac09c6fd211cf9902c46c7c68d4f18156bcfb9b63038901d1`.

The lexical index covered all 14 C++/header/shader files in the inner archive. It found 46 `invoke<T>` bindings, all using literal hashes, and no dynamic hash arguments in that invocation form. The scanner's self-tests passed. These are static inspection results, not a compiled call graph or game test.

| Trace | Observed result |
| --- | --- |
| `Collision.cpp:409–688` | `BuildAndSend` constructs regions; `SendRegion` invokes `WriteCollision` at lines 620 and 687, including `kColRegion`. |
| `Link.cpp:121–148` | `WriteCollision` publishes collision data to the shared-memory channel. Together with the earlier protocol review, this identifies the host-world-to-Minecraft direction. |
| `Renderer.cpp:691–770,1312` | `kWeBlock` is referenced by `BuildWorldEntities`; `ReadWorldEntities` is consumed by `RenderFrame`. These paths generate/render geometry, not native host obstacles. |
| `RedCraft.cpp:1015,1019,1079` | Explosion, hit-actor and `CopyHazards` paths are present. They must not be mistaken for proof of rigid-body collision registration. |
| Native obstacle creation | The indexed sources contain no conventional object/model/collision-enable path for placed blocks. The earlier native-hash inventory also lacked the known object-creation hash. |

**Supported conclusion:** the inspected source does not demonstrate a Minecraft-block-to-RDR2-native-collider implementation; its identifiable collision export is in the reverse direction. **Still unverified:** whether the distributed binary physically stops an NPC, horse or wagon. No live game run or binary/source equivalence test was performed. The [previous assessment](redcraft-audit-2026-10-15.md) records the own-source permission restriction.

## Candidate A: Dying Light 1

This is the first title, not Dying Light 2 or The Beast. [GModLight](https://github.com/GooseMcGee/DyingLight-GMod) already bridges it with Garry's Mod. Its [MIT license](https://github.com/GooseMcGee/DyingLight-GMod/blob/main/LICENSE) was read directly. Its README describes camera/input transport, native zombie proxies, damage calls, reprojection and a fake-host test harness. This is a stronger integration reference than a simple asset spawner, but is not a Minecraft adapter.

The [renderer source](https://github.com/GooseMcGee/DyingLight-GMod/blob/main/dl_plugin/src/overlay.cpp) was inspected directly: the shader samples the guest frame and uses encoded guest distance for reprojection; the depth-stencil state disables depth testing. This is not host-world depth occlusion. The [engine analysis](https://github.com/GooseMcGee/DyingLight-GMod/blob/main/docs/ENGINE_ANALYSIS.md) also identifies unresolved physics paths and targets Steam executable 1.55.0.0. Current executable compatibility was not tested.

**Proposed gameplay:** Minecraft construction, tools and TNT in Dying Light's world, with blocks affecting native enemies and native attacks affecting the Minecraft player. This is a design objective, not an existing capability. Preserving native parkour and making AI navigate changed construction require separate design and verification; physical obstruction alone does not update navigation.

**First technical gates:** host-depth acquisition, native block-collider create/remove, and correct native damage semantics. Camera/IPC infrastructure may be reused after version and license checks. Do not start by rewriting the complete bridge or claim that replacing the GMod endpoint is sufficient.

## Candidate B: Just Cause 3

[Console Thingy](https://github.com/aaronkirkham/jc3-console-thingy) provides a relevant [MIT-licensed](https://github.com/aaronkirkham/jc3-console-thingy/blob/master/LICENSE) entry point for spawning vehicles, characters, animals and props. Direct inspection of [spawn_system.h](https://github.com/aaronkirkham/jc3-console-thingy/blob/master/src/game/spawn_system.h) found a callback-based spawn request implemented through hard-coded addresses. [graphics.h](https://github.com/aaronkirkham/jc3-console-thingy/blob/master/src/graphics.h) accesses D3D11 for console drawing, not a complete world-depth or arbitrary-geometry integration.

**Proposed gameplay:** Minecraft structures interacting with vehicles and explosions. This is closer to the vehicle-centered GTA example, but has weaker verified integration coverage than GModLight. Camera extraction, terrain queries, controllable collider geometry, cleanup, damage and current-build address compatibility remain to be established. An entity spawner does not solve arbitrary block collision.

## Existing-project check and ranking

A direct Witcher 3 alternative is already represented by [WitchCraft](https://github.com/Henvag/WitchCraft). Its README reports a Minecraft/SkyCraft prototype with movement, inventory and block placement, while identifying unfinished combat and full-scene occlusion. It is not an unoccupied replacement.

Public searches for the exact Minecraft/Dying Light and Minecraft/Just Cause 3 pairings with passthrough/bridge terms did not identify a direct equivalent in this round. Searches also used possible MineLight/LightCraft names. Unrelated Minecraft themed mods are not evidence of dual-runtime integration. GitHub repository search timed out, so repository-wide coverage is incomplete. No claim of being first is justified for either candidate.

**Recommendation:** prioritize Dying Light 1 for the next narrowly scoped feasibility study because the inspected integration reference covers more of the required host interface. Retain Just Cause 3 as the more vehicle-focused alternative. Neither is approved or verified as a complete GTA-level replacement. Do not change repository identity or rewrite the roadmap until a target is chosen and its most consequential technical gaps are assessed.

## Preserved work

Retain the shared-memory/core contracts, lifecycle tests, validation utilities, localization, documentation and CI structure. RDR2 Native bindings and host-specific camera/terrain assumptions do not automatically transfer. The existing host remains separate; no new host implementation or playable release was produced by this assessment.

## Inspected source identities

| File | Git blob SHA |
| --- | --- |
| GModLight `README.md` | `1de5ac0d1cf0239cebef03b9b894ce7c936d251a` |
| GModLight `LICENSE` | `febb14127c189b8067849e766e46b33f5d19cd9a` |
| GModLight `dl_plugin/src/overlay.cpp` | `16fdde25b5da6ad9c169b07a50b686fed1406beb` |
| GModLight `docs/ENGINE_ANALYSIS.md` | `b73f279076d9c22e76270e9306a070019f922c4a` |
| JC3 Console Thingy `src/game/spawn_system.h` | `51ad1c5f7aed96a3d6fb815bd6ac2703b39384ef` |
| JC3 Console Thingy `src/graphics.h` | `605a0d1905b3f147cb01536fb0b2adb72afa4cbd` |
| JC3 Console Thingy `LICENSE` | `af58cec86597a2b572aa8dfa23497aa2f427e709` |
| WitchCraft `README.md` | `18ef8256ed661937eb273e4c7c41d805cc5bbebc` |
