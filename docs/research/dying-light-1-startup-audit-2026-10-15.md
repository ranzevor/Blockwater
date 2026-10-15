# Dying Light 1 startup audit — 2026-10-15

[简体中文](dying-light-1-startup-audit-2026-10-15.zh-CN.md)

## Decision

**Host-migration startup is not cleared.** Dying Light 1 remains the proposed target; the evidence does not establish that the full fusion is impossible. It establishes that the reference implementation cannot yet supply the required single-player gate, safe mutation phase, native block-collider lifecycle, or verified bidirectional damage contract. The experiments below are unresolved work, not passed prerequisites.

This change records research only. Blockwater's existing RDR2 implementation, active host contract, ABI, builds and tests remain unchanged. No Dying Light adapter, Minecraft guest, installer or playable release is delivered by this audit. Further RDR2 gameplay development and alternative target selection are outside this change.

## Fixed sources and release identity

| Source | Read revision | Result |
|---|---|---|
| Blockwater main | `bdd2e93bcdbb25e5bbffcca3b33e8e5e6c2a9ce6` | Unchanged from the handoff; no open PR at initial inspection |
| Existing research branch | `6bbc1a23d8d674b0a2df7fd0826835d0df3d09f1` | Ten commits ahead of main; eight research/workflow/tool additions and no application changes |
| GModLight main | `e53209d25c295550e40b8fa977cc36147567b7de` | Source used for every upstream code conclusion below |
| GModLight v0.1.0 tag | `e3ad71bc2e38141321a9804192d7864ec6c96bde` | Different from audited main; main is two commits ahead and changes fourteen files |

The author's [v0.1.0 release][release] was published on 2026-10-04 at 20:12:15 UTC. The tag-to-main difference includes runtime code, configuration, the bridge and guest code, not only documentation. Its ZIP metadata is not evidence that the audited main was shipped or that Blockwater was tested. Release bytes were not executed or compared with a rebuilt binary. [Compare][release-diff]

The engine research names Steam `DyingLightGame.exe` 1.55.0.0, build 24076292. That is the upstream research target, not a verified current retail version or a Blockwater compatibility promise. [Engine history][history]

The [previous target review][previous] remains on its original research revision. Its RedCraft attachment and static scans are not repeated. That branch's temporary workflow and audit tools are not imported by this change.

## Duplicate-project search

Search date: **2026-10-15**. No directly equivalent DL1 × Minecraft dual-runtime project was established in the results examined. Coverage is incomplete; this is not a uniqueness claim.

| Surface and actual queries | Observed result | Coverage limit |
|---|---|---|
| GitHub repository search: `"dying light" minecraft`; `DyingLight Minecraft` | Both calls returned `ReadTimeout` | Repository-search coverage remains unresolved; the timeouts are not zero-result searches |
| Web: `"Dying Light" "Minecraft" "passthrough"`; `"Dying Light" "Minecraft" "fusion"`; `Dying Light Minecraft mod mashup 2026` | GModLight and Minecraft-themed projects appeared; no inspected result established the requested two-runtime pairing | Public indexed results only |
| Name checks: `"MineLight" "Dying Light"`; `"LightCraft" "Dying Light"` | No matching author/code/release identity was established | Similar names alone are not project identity |
| Video/community: `site:youtube.com "Dying Light" "Minecraft" "mod"`; `site:youtube.com "Dying Light" "Minecraft inside"`; `site:reddit.com/r/dyinglight "Minecraft" "mod"`; `GModLight GooseMcGee video` | No independently verified video-to-source-to-release chain for a DL1 × Minecraft fusion | Search metadata is not playback or an in-game test; private communities were not covered |
| Chinese: `消逝的光芒 我的世界 双游戏 融合`; `"消逝的光芒" "我的世界" "融合"` | A creator's Minecraft modpack video was identified, not an established DL1 host integration | Indexed description only; direct page access was blocked by robots.txt and footage was not inspected |

**Identity distinctions.** GooseMcGee's GModLight has matching repository and author release identity, but the guest is Garry's Mod. The creator page for [Last One Alive][modpack] describes a Forge Minecraft modpack inspired by Dying Light. The [光芒消逝之日 video][video], by 小小伊布酱 and published 2023-01-13, describes a Minecraft modpack. Neither description supplies evidence of two original game runtimes. Minecraft-themed maps, weapons, enemies and the host's Minecraft Easter egg are not accepted as equivalent fusion evidence. [GModLight README][up-readme]

The inspected overlap does not remove the proposed project's value. A directly equivalent project, if later found, needs a comparison of actual interaction, distribution and maintenance capabilities rather than an automatic target rejection.

## Host-interface evidence

All rows concern the fixed GModLight main above. Upstream game observations are author-reported; **Blockwater has not run any DL1 in-game test for these interfaces**. Export names and source implementations are evidence of candidate interfaces, not of successful invocation against an available retail binary.

| Interface | Source and static finding | Upstream observation / unresolved condition |
|---|---|---|
| Loading and version identity | [Initialization][init], [entry point][detach], [CMake][cmake]: ASI DLL, mangled engine exports, fixed game-DLL offsets | The research build is identified, but the observed initialization path does not validate a full accepted binary fingerprint before hooks and mutations |
| Frame phase and thread | [Message pump][tick], [world dispatch][world] | Current code hooks PeekMessageA/W, filters to the window thread and uses a 3 ms throttle. If the last tick is older than 250 ms, it falls back to world work in Present. Neither throttle nor a thread-ID match proves an engine-safe update phase |
| Camera and projection | [Camera implementation][camera] | Reads camera vectors, view, combined and projection matrices; uses plausibility-based fallbacks and derives FOV from projection. Upstream reports Y-up meters and the forward-axis correction. No Blockwater frame/depth pairing, jitter or resize test exists |
| World query | [TraceProbes][events] | Bounded guest segments are traced against host geometry. This is useful query evidence, not a native obstacle-creation or complete world-geometry interface |
| Object / collider lifecycle | [Model creation/deletion][models], [SyncVisuals][world] | Creates CModelObject from an existing host mesh. There is no arbitrary MC block collision-shape construction/registration in this path. Creation failure can lose an allocated object; deletion failure is not acknowledged to its caller |
| Damage and death | [TakeDamage][damage], [ApplyEvents][events], [bridge][abi] | Builds a descriptor using fixed offsets and invokes a virtual slot. A slot/export mismatch is logged but does not prevent the call. Normal damage, first-hit correctness, attribution and reverse settlement remain unverified |
| Single-player mode | [Plan section 1.2][plan], [initialization][init], [world dispatch][world] | Upstream states that its only co-op guard is the private-visibility instruction. SessionCooperative / SessionCooperativeDI are research leads, not an implemented classifier |
| Exit and restoration | [Level-change handling][world], [entry point][detach], [model deletion][models] | Level-pointer comparison clears state; DllMain handles attach but has no detach restoration branch. Null level propagation, generation reuse, owned-resource rollback and controlled unload still require experiments |

Additional candidate exports include `IsLoading`, `IsLevelShutdown`, object-destruction queries and native frame-handler control entries. These narrow the next inspection targets; they do not identify a callable callback ABI, safe phase, or single-player state. `IsNetPlayerPlaying`, online-services initialization, one connected player and private visibility are not substitutes for a verified session-mode contract. [Lifecycle exports][exports-life] [Frame-handler exports][exports-frame]

## Three core gates: new findings

### Rendering

The audited shader declares only the guest frame texture; alpha-derived distance drives reprojection. Its depth-stencil state disables depth testing. This is direct source evidence that the present compositor does not compare the guest with the host scene's depth. [Compositor][compositor]

The PLAN's proposed scene-depth capture and its guessed depth convention are unimplemented hypotheses. The native-model alternative draws existing host meshes, not arbitrary Minecraft geometry. Neither route has a measured DL1 depth-resource identity, encoding, camera/frame correspondence, transparency or HUD-ordering result. [Plan][plan] [Models][models]

D3D11 supports depth resources with compatible typed views, subject to resource creation and binding requirements. That makes depth capture a concrete graphics experiment, not proof that any DSV found in DL1 is the correct scene surface. Inspect the actual resource format, view, sample count and pass before designing copies or shader access. [Microsoft depth guide][d3d-depth] [Resource-view contract][d3d-view]

### Collision

`CreateModelRaw` can allocate a game object and then return null after a failed cast or exception without deleting the allocated object. `DeleteModel` reports no deletion outcome to `SyncVisuals`, which erases its bookkeeping entry. The latter also has an early return after repeated creation failures, before its normal removal sweep. These are static ownership/rollback gaps in the reference path, not observed Blockwater crashes. [Models][models] [SyncVisuals][world]

A mesh creation call, a ray hit, moving an existing actor or guessing a collision flag does not prove that MC placement adds a native obstacle. Shape creation, binding, collision filtering, activation, deactivation and reclamation must be traced together. Player, native-enemy and projectile responses must be tested separately; navigation is a different result.

### Damage

The current damage implementation still uses a game-DLL vtable offset and descriptor constants. Its virtual-slot comparison only logs a mismatch and continues. The bool success path means the call returned, not that the expected amount of health was removed. [TakeDamage][damage]

The bridge has guest-to-host damage and kill events but no reverse damage-settlement event contract. These event records carry neither a session generation nor an event identity or per-event attacker identity. The caller selects the host player as attacker; a kill event independently calls TakeDamageToKill or Kill. This cannot establish Blockwater's required state authority, exactly-once effects across reconnects, attacker attribution or bidirectional TNT behavior. [Bridge][abi] [ApplyEvents][events]

The historical first-hit fault remains relevant until reproduced and resolved through normal nonlethal damage. Forced death is a separate operation and must not be counted as its repair. [Engine history, rounds 5–7][history] [Plan][plan]

## Experiments required to clear startup and the core path

These are test specifications, **not implemented or passed tests**. Full gameplay is not required before beginning a probe, but host-changing work requires validated version, mode and execution-phase gates. In the absence of those gates, only non-mutating observation belongs in an isolated probe.

| ID | Experiment | Acceptance evidence |
|---|---|---|
| DL1-S0 | Identify accepted EXE/engine/game-DLL fingerprints; observe session state and engine callback cadence without changing the world | Exact image identities; positive single-player classification; unknown/loading/shutdown/disallowed states reject every mutation. No config override, player-count shortcut, timer-only tick claim or Present mutation fallback |
| DL1-G0 | Identify a scene-depth resource and camera from the same rendering pass; alternatively trace native geometry submission | Actual resource/view/sample descriptors and frame correspondence; near/far and depth convention measured; foreground and background cases distinguish correct occlusion from alignment; resize, jitter, transparent layers and HUD ordering tested |
| DL1-P0 | Create one owned block collision shape, register it, remove it, and repeat across level/session changes | Native player, enemy and projectile blocking while present and no blocking after removal; failure after each allocation/registration step rolls back; ownership ledger returns to baseline; navigation reported independently |
| DL1-D0 | Execute ordinary nonlethal damage, then reverse damage and a bounded explosion | Before/after authoritative health, victim and attacker identity, unique event and generation, one application per event/victim, no feedback loop; first hit works without Kill; duplicate, late, reconnected and dead-target cases reject or settle correctly |
| DL1-R0 | Stop/reconnect the guest, leave/re-enter a level and perform controlled shutdown | Mutations stop before teardown; stale queues and identities cannot affect a new session; input/HUD state and all owned objects/hooks/resources are restored or retired through a verified lifetime path |

S0 must validate transitions, not just initial entry. Real disallowed-mode observation needs an explicitly authorized isolated test arrangement; no multiplayer gameplay, invitations or invasions are initiated by this audit. Negative protocol tests alone cannot prove the game-state classifier.

G0 must not classify a resource as the world depth solely because it is the largest DSV. P0 must not use a guest-only collider or an existing stock prop as proof of arbitrary block collision. D0 must measure ordinary damage before adding a death shortcut. R0 must track session/level generations, not just raw pointer equality. Per-frame mutation callbacks must not perform blocking IPC, waits, file I/O or synchronous GPU readback.

After S0 has a validated path and the remaining feasibility risks have concrete experimental support, create the actual DL1 adapter and amend project-specific host/build/documentation contracts on an independent implementation branch. Retain the existing ABI authority and regressions. Then implement a real Fabric guest and perform G0/P0/D0 in the same scene before peripheral gameplay expansion. Minecraft/Fabric versions remain unselected pending guest-source and dependency validation; Garry's Mod is not a final runtime dependency.

## Available validation conditions and results

The inspected execution environment is Linux x86_64. CMake, Ninja, C++ compilers and Java are available. The inspected tool inventory has no Windows runtime, Wine, Steam, MinGW cross-compiler or exposed GPU device. No Blockwater checkout or Dying Light executable was found in the inspected working roots. This inventory does not describe machines or accounts outside the accessible environment.

Blockwater's existing [CI][blockwater-ci] provides Ubuntu 24.04 and Windows 2025 compilation/tests. The [PR #3 source-head run][base-ci] for `23c52e6dbe16c5c2ca1202ae06ee2e87e4662167`, incorporated into the inspected main, completed successfully on both platforms. Those results are existing engineering evidence, not tests run again by this audit or DL1 compatibility results. No push workflow runs were returned for the merge SHA; the workflow is PR/manual-triggered.

GModLight's CMake fakehost target and [fakehost source][fakehost] exercise a simulated host with real Garry's Mod. Its [CI][up-ci] builds/packages on Windows; its release job does not provide a DL1 gameplay validation. Neither is a substitute for the missing host runtime.

**Missing runtime prerequisites:** an accessible, authorized Windows x64 graphics session with a legitimately available DL1 copy; exact EXE and DLL identities; permission for isolated probe execution and disposable test data. The later real guest tests also need a legitimate Minecraft runtime.

**Result of this change:** static source/release/search/environment evidence and an explicit startup decision. DL1 process integration, GPU rendering, native collision, normal damage, mode rejection and restoration remain unrun. They cannot be reported as passed by reference to unit tests, a Windows build or upstream observations.

## Reuse boundary

GModLight's own [LICENSE][license] is MIT with GModLight contributors as copyright holder. Its [third-party inventory][notices] separately lists MinHook, Ultimate ASI Loader and gmod-module-base; the latter is a GMod-specific dependency, not a planned Minecraft dependency. This audit incorporates no upstream implementation or binary. Any later copied component needs its own pinned source and retained applicable license before integration. No game asset, proprietary SDK, save or test replacement is included.

[up-readme]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/README.md
[plan]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/docs/PLAN.md
[history]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/docs/ENGINE_ANALYSIS.md
[init]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/dl_plugin/src/engine.cpp#L1-L145
[camera]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/dl_plugin/src/engine.cpp#L260-L360
[tick]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/dl_plugin/src/main.cpp#L572-L616
[world]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/dl_plugin/src/main.cpp#L1250-L1499
[detach]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/dl_plugin/src/main.cpp#L1499-L1518
[compositor]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/dl_plugin/src/overlay.cpp#L80-L210
[models]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/dl_plugin/src/engine.cpp#L1371-L1420
[damage]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/dl_plugin/src/engine.cpp#L590-L680
[events]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/dl_plugin/src/main.cpp#L906-L1035
[abi]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/shared/bridge.h
[exports-frame]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/docs/re-data/engine_x64_rwdi.dll.exports.txt#L2153-L2225
[exports-life]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/docs/re-data/engine_x64_rwdi.dll.exports.txt#L3932-L4077
[fakehost]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/tools/fakehost.cpp#L1-L120
[cmake]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/CMakeLists.txt
[up-ci]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/.github/workflows/build.yml
[license]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/LICENSE
[notices]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/THIRD_PARTY_NOTICES.md
[release]: https://github.com/GooseMcGee/DyingLight-GMod/releases/tag/v0.1.0
[release-diff]: https://github.com/GooseMcGee/DyingLight-GMod/compare/e3ad71bc2e38141321a9804192d7864ec6c96bde...e53209d25c295550e40b8fa977cc36147567b7de
[previous]: https://github.com/ranzevor/Blockwater/blob/6bbc1a23d8d674b0a2df7fd0826835d0df3d09f1/docs/research/host-target-review-2026-10-15.md
[base-ci]: https://github.com/ranzevor/Blockwater/actions/runs/38657624738
[blockwater-ci]: https://github.com/ranzevor/Blockwater/blob/bdd2e93bcdbb25e5bbffcca3b33e8e5e6c2a9ce6/.github/workflows/ci.yml
[modpack]: https://modrinth.com/modpack/last-one-alive
[video]: https://www.bilibili.com/video/BV1bM411F7o4/
[d3d-depth]: https://learn.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-depth-stencil
[d3d-view]: https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_shader_resource_view_desc
