# RDR2 native host

[简体中文](zh-CN/rdr2-host.md)

## Implemented scope

`Blockwater.asi` is the B1 lifecycle component for Windows x64/MSVC. It registers with ScriptHookRDR2, checks the game context on the script thread and connects to the existing ABI v1 bridge. Camera/terrain publication, collision objects and gameplay effects are subsequent B1 work. The lifecycle component publishes no valid world frame and rejects current-session gameplay requests rather than simulating success.

## Build and test

From the repository root on Windows with the C++ desktop toolchain:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
ctest --test-dir build -C Release -R "^rdr2_" --output-on-failure
cmake --install build --config Release --prefix build/package
```

The plugin is `build/rdr2/plugin/Release/Blockwater.asi`. `BLOCKWATER_BUILD_RDR2` defaults to `ON` on Windows; set `-DBLOCKWATER_BUILD_RDR2=OFF` to build only the portable core. The native plugin requires the MSVC x64 ABI. No proprietary SDK is downloaded or committed: CMake generates an import library from the six interface symbols in `rdr2/scripthook.def`.

The staging directory contains only `Blockwater.asi` and two license files. The locally built test `ScriptHookRDR2.dll` lives under `build/rdr2/harness/Release/`; it is not the game's runtime and must not be installed into a game directory. `rdr2_install_manifest` checks the staged file set. Live execution requires the separately obtained [ScriptHookRDR2 runtime and ASI loader](https://www.dev-c.com/rdr2/scripthookrdr2/); no supported game-version matrix is established by these loader tests.

## Lifecycle and ownership

| Context or transition | Host behavior |
| --- | --- |
| DLL attach | Register the script entry only; no game-native calls or shared-memory construction |
| First script entry | Acquire a non-waiting, exclusive host-presence handle; open a valid retained mapping or create a new one; invalidate retained host frames |
| Main menu, unavailable player, dead player or missing native result | Keep the bridge closed |
| Controlled live player outside a network context | Announce a nonzero epoch and heartbeat; wait for an advancing guest heartbeat |
| Pause or loss of player control | Close the peer record, invalidate world data and discard local pending responses |
| Guest closes, times out or changes epoch | Invalidate the session, advance the host epoch and require a fresh heartbeat handshake |
| Any positive network-context check | Refuse with `Multiplayer`; refusal remains latched until the plugin is unloaded |
| Explicit DLL unload | Unregister the script, close its transport and release mapping/host-presence handles; invoke no game natives |
| Process termination | Let the operating system reclaim process resources; do not run teardown against other unloading DLLs |

The mapping is `Local\Blockwater_rdr2`; the separate presence handle is `Local\Blockwater_rdr2_host`. A second plugin instance fails closed without replacing the first. The presence handle is created and retained, not waited on. Reload can reopen a mapping still held by the guest, but never reinitializes an incompatible existing region. The new epoch is greater than the retained host epoch; exhaustion refuses startup rather than wrapping to an old generation.

`scriptWait(0)` yields the ScriptHook fiber to the next frame. There are no IPC waits, worker threads, disk operations or resource creation inside the frame loop. Shared-memory cleanup calls no game functions. No camera, player-visibility state or game object is owned by this first component; later gameplay adapters must release their game resources on the script thread before unload.

## Session protocol

Transport readiness is not gameplay readiness. The guest must also require a valid, current `HostFrame` before treating a host as a gameplay provider. `HostSession` deliberately retains a zero-flag invalid frame until real world acquisition is implemented.

A session drains at most 32 raw incoming entries per tick, including stale entries. It discards invalid packets, mismatched sender/recipient epochs and response-only event kinds. Current-epoch placement, removal, explosion and damage requests receive `Rejected`, preserving request ID, kind and block coordinates. Duplicate unsupported requests remain harmless and may receive the same rejection again.

If the outgoing queue is full, one pending rejection is retained and further incoming consumption stops. A generation transition discards that pending response; old events cannot become actions in the new session. Neither side resets shared queue cursors. Heartbeat validation reads the peer timestamp before sampling the local clock, so a concurrent update is not misclassified as a future heartbeat. The ABI layout and version remain unchanged.

## Native interface register

The context reader uses only these read-only calls. Names, hashes and signatures were cross-checked against the pinned [Garry's Redemption native register](https://github.com/codeByAlexff/garrys-redemption/blob/4ada8a12a85d6f30e2c6485970792cc1b837c6ac/docs/NATIVES.md), whose source is the [RDR3 Native DB](https://alloc8or.re/rdr3/nativedb/). This is source-level evidence, not a Blockwater game run.

| Native | Hash | Signature |
| --- | --- | --- |
| `NETWORK_IS_IN_SESSION` | `0xCA97246103B63917` | `BOOL()` |
| `NETWORK_IS_SESSION_STARTED` | `0x9DE624D2FC4B603F` | `BOOL()` |
| `NETWORK_IS_GAME_IN_PROGRESS` | `0x10FAB35428CCC9D7` | `BOOL()` |
| `IS_PAUSE_MENU_ACTIVE` | `0x535384D6067BA42E` | `BOOL()` |
| `PLAYER_PED_ID` | `0x096275889B8E0EE0` | `Ped()` |
| `DOES_ENTITY_EXIST` | `0xD42BD6EB2E0F1677` | `BOOL(Entity)` |
| `IS_ENTITY_DEAD` | `0x7D5B1F88E7504BBA` | `BOOL(Entity)` |
| `PLAYER_ID` | `0x217E9DC48139933D` | `Player()` |
| `IS_PLAYER_CONTROL_ON` | `0x7964097FCE4C244B` | `BOOL(Player)` |

All three network checks are evaluated. A positive result takes precedence over another check's missing result. With no positive result, any missing result keeps the bridge unavailable. Integer arguments occupy the low 32 bits of the SDK's 64-bit argument slots. No arbitrary native hash or console command is accepted through IPC.

The six ScriptHook function declarations are adapted from its [published integration declarations](https://github.com/codeByAlexff/garrys-redemption/blob/4ada8a12a85d6f30e2c6485970792cc1b837c6ac/rdr2/src/scripthook.h), preserving their MSVC-decorated import names. Attribution and the original MIT text are retained in [the reference register](references.md) and [license file](../LICENSES/Garrys-Redemption.txt).

## Automated evidence and remaining acceptance

`blockwater_host_session` tests the portable lifecycle, stale-event budget, full reverse queue, thread ownership, epoch exhaustion, world-frame invalidation and 100,000 concurrent-heartbeat checks without a false epoch change. On Windows the loader tests load the actual compiled `.asi` against an isolated test runtime. Its native allowlist checks call signatures and script-fiber ownership; unknown calls, off-thread calls and nonzero frame waits fail the tests. Scenarios cover independent network flags, partial native failure, pause, unavailable/dead/uncontrolled players, disconnect, guest restart, retained-mapping reload, corrupt mapping, duplicate plugin and unload before first script entry. Separate tests cover a missing runtime DLL and installation contents.

Per-commit execution evidence is in [CI](https://github.com/ranzevor/Blockwater/actions/workflows/ci.yml). Loading against the genuine ScriptHook runtime, real game-context behavior and reload inside Story Mode still require the exact-version [game acceptance](milestones.md#required-game-validation). B1 also still requires camera/terrain acquisition and real collision creation/removal.
