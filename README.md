# Blockwater

**Minecraft × Red Dead Redemption 2 — a building sandbox meets the frontier.**

[English](README.md) · [简体中文](README.zh-CN.md) · [Documentation](docs/README.md) · [Contributing](CONTRIBUTING.md) · [CI](https://github.com/ranzevor/Blockwater/actions/workflows/ci.yml) · [MIT](LICENSE)

Blockwater is an open-source passthrough project that aims to bring Minecraft's building, tools and creatures into **Red Dead Redemption 2's Story Mode**. Both games keep running: Minecraft supplies its sandbox rules, RDR2 supplies its world, and a local bridge connects their interactions. This is runtime integration, rather than a Minecraft-themed texture pack or a recreation of either game.

The intended experience is to build a wall that stops an RDR2 character or wagon, set off TNT that affects the surrounding world, and let Minecraft creatures interact with RDR2 NPCs.

**Current stage: RDR2 adapter development.** The bridge core and the first ScriptHookRDR2 lifecycle component are implemented. Windows x64/MSVC builds `Blockwater.asi` with context checks and session handling. Real camera/terrain acquisition, gameplay effects, the Fabric client and GPU compositor remain; there is no playable game release yet.

[Current capabilities](#current-capabilities) · [How it works](#how-it-works) · [Build and run](#build-and-run) · [Compatibility](#compatibility) · [Roadmap](#roadmap) · [Contribute](#contribute)

## Current capabilities

| Component | What is implemented |
| --- | --- |
| Local bridge | Windows named mappings and POSIX shared memory, version/size checks, atomic frame snapshots and bounded event queues in both directions |
| Peer lifecycle | Heartbeats, session epochs, stale-event rejection, duplicate suppression and multiplayer-context refusal in the host contract |
| World mapping | Position and scale conversion between RDR2's metre/Z-up coordinates and Minecraft's block/Y-up coordinates; incremental terrain samples and nearby-entity snapshots |
| Host adapter | Simulated block-proxy creation/removal, explosion and entity-damage callbacks, input bounds and owned-object cleanup on guest restart |
| RDR2 native lifecycle | ScriptHook registration, read-only game-context checks, pause/disconnect/reload handling and explicit rejection of unimplemented gameplay requests; [native-host guide](docs/rdr2-host.md) |
| Image composition | A CPU reference implementation for depth-tested world layers and HUD blending; it accepts prepared images and depths, not live game frames |
| Localization | Startup-loaded English and Simplified Chinese catalogs with English fallback for missing translated keys |
| Developer tests | Protocol, child-process IPC, host simulation, CPU compositing and localization suites, plus the standalone fake-pair example |

The [architecture](docs/architecture.md) specifies the protocol and current resource limits. The [verification record](docs/verification-2026-10-14.md) describes the Linux baseline; [CI runs](https://github.com/ranzevor/Blockwater/actions/workflows/ci.yml) show subsequent automated results for their tested commits.

## How it works

The intended two-game integration separates responsibility rather than reimplementing Minecraft inside RDR2:

| Runtime | Responsibility | Bridge interaction |
| --- | --- | --- |
| RDR2 host | Landscape, native NPCs, horses, wagons, missions and world effects | Publish camera, terrain and entity state; apply mapped block collisions, explosions and damage through game-native callbacks |
| Minecraft guest | Blocks, inventory, tools, creatures and Minecraft rules | Consume host state and collision data; publish Minecraft interactions and rendered layers |
| Blockwater | Coordinate conversion, state exchange, event validation and rendering integration | Connect the two runtimes while keeping ownership and session boundaries explicit |

The **state path** uses versioned, local shared memory. The planned **graphics path** exports Minecraft's world color, depth and HUD, then composites them against RDR2's camera and depth buffer. The current shared-memory ABI carries state and events; GPU frame transport, depth extraction and reprojection remain separate integration work.

Game-native operations belong on the RDR2 script thread. The bridge core itself does not start or attach to a game process. See [runtime ownership and ABI](docs/architecture.md) for lifecycle, coordinates and callback contracts.

## Build and run

### Requirements

The bridge core needs **no game installation, game account, mod SDK or third-party C++ library**.

| Requirement | Details |
| --- | --- |
| Platform | 64-bit little-endian desktop with lock-free 64-bit atomics; current development targets are Linux x86-64 and Windows x64 |
| C++ toolchain | C++20, including `std::atomic_ref`; GCC/Clang on Linux or MSVC on Windows |
| CMake | 3.20+ for the core; **3.21+** for the [Visual Studio 2022 generator](https://cmake.org/cmake/help/latest/generator/Visual%20Studio%2017%202022.html) used below |
| Build tool | Make or Ninja on Linux; Visual Studio 2022 with C++ desktop tools and a Windows SDK for the Windows example |
| Python | 3.9+ for the repository's catalog and documentation validator; not required to build the C++ core |
| Git | Used to obtain the source in the commands below |

Get the source, then keep the following commands in the repository root:

```sh
git clone https://github.com/ranzevor/Blockwater.git
cd Blockwater
```

### Linux

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
./build/blockwater_fakes
python3 tools/validate_repository.py
```

### Windows — PowerShell and Visual Studio 2022

Use a PowerShell session where CMake, Git and Python are available, with the C++ toolchain installed.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
.\build\Release\blockwater_fakes.exe
python tools/validate_repository.py
```

Visual Studio is a multi-configuration generator: keep `--config Release` for the build and `-C Release` for CTest. Single-configuration generators such as Ninja place the executable directly in `build/` instead. Use a new build directory when switching generators.

### Example and expected result

Both examples run [`blockwater_fakes`](examples/fake_pair.cpp). It creates a shared-memory mapping and simulates the host and guest **inside one process**, publishes a terrain snapshot, places one block proxy and forwards one explosion:

```text
Host frame=1 ground samples=16
Explosion propagated: radius=4
Host block proxies=1
```

The example exits with code `0` on success. Its output comes from simulated callbacks; it does not open a game window or modify RDR2. A separate child-process shared-memory scenario is exercised by [`tests/test_ipc.cpp`](tests/test_ipc.cpp).

The portable build produces `blockwater_core` (a static library), `blockwater_tests`, `blockwater_session_tests` and `blockwater_fakes`. Windows x64/MSVC also builds `build/rdr2/plugin/Release/Blockwater.asi` by default. Set `-DBLOCKWATER_BUILD_RDR2=OFF` for core-only development. The [native-host guide](docs/rdr2-host.md) covers loader tests and isolated staging; no Fabric `.jar` or playable installer is produced.

### Tests and troubleshooting

CTest registers `blockwater_tests`, `blockwater_fake_pair` and `blockwater_host_session`; Windows native builds additionally register the `rdr2_` loader and staging tests. The first covers frame consistency, invalid payloads, queue saturation, peer restarts, duplicate events, multiplayer refusal, coordinate transforms, host callbacks, depth occlusion and locale lookup. The Python validator checks UTF-8 catalogs, message-key/placeholder parity and local Markdown file links.

| Symptom | Action |
| --- | --- |
| CMake cannot find a compiler or rejects C++20/atomic support | Select a C++20-capable 64-bit toolchain; on Windows, check that the C++ desktop tools and Windows SDK are installed |
| Visual Studio build succeeds but the executable or test is not found | Use the matching `Release` configuration and `build/Release/` executable path |
| A mapping-creation error occurs when launching the example twice | Run one `blockwater_fakes` instance at a time; the example uses a fixed mapping name |
| Catalog or documentation validation fails | Run from the repository root and check the reported resource or link; both files under `locales/` must be present |

The [CI workflow](.github/workflows/ci.yml) builds and runs CTest on `ubuntu-24.04` and `windows-2025`; catalog/documentation validation runs in its Linux job. It runs on pull requests or manual dispatch and does not publish releases.

## Compatibility

**Windows x64 is the target for the two-game integration.** Linux supports bridge-core development and tests, not an established Linux game-integration path. Recorded compiler results and game-component status are maintained in the [compatibility matrix](docs/compatibility.md).

The Minecraft guest targets **Java Edition through Fabric**. Minecraft/Fabric, RDR2, ScriptHookRDR2, graphics API and driver versions will be listed as supported only after the corresponding game tests. Versions used by the reference projects are not a Blockwater compatibility matrix.

Only RDR2 **Story Mode** is in scope; Red Dead Online is not supported. Game integration requires separately licensed copies of both games and the applicable mod dependencies. See [game-run acceptance](docs/milestones.md#required-game-validation) before reporting a version as working.

## Roadmap

| Stage | Deliverable |
| --- | --- |
| B0 — Bridge core | Implemented: native protocol, host contract, simulation, CPU composition reference and localization; platform validation is tracked separately |
| B1 — RDR2 adapter | A ScriptHookRDR2 plugin with camera/terrain streaming, real block collision objects, Story Mode gating and safe unload |
| B2 — Minecraft guest | A Fabric client with terrain consumption, block/explosion events, rendered-layer export and reconnect handling |
| B3 — Rendering | DX12/ReShade integration with real host depth, camera calibration, depth-aware compositing and reprojection |
| B4 — First playable scene | Place/remove Minecraft blocks that stop an RDR2 actor or vehicle, trigger a corresponding TNT explosion and verify world occlusion in one repeatable scene |
| B5 — Extended gameplay | Cross-game damage and mob proxies, equipment interactions and persistence across game restarts |

Acceptance conditions and dependencies live in [integration milestones](docs/milestones.md). Release checks are described in [releasing](docs/releasing.md); actual changes belong in the [changelog](CHANGELOG.md).

## Repository layout

| Path | Purpose |
| --- | --- |
| [`include/blockwater/`](include/blockwater/) | Public core headers; `wire.hpp` defines the shared-memory ABI, `link.hpp` peer lifecycle and `coordinates.hpp` world transforms |
| [`src/`](src/) | Mapping implementation, host adapter, CPU compositor and localization catalog |
| [`rdr2/`](rdr2/) | Windows native plugin, ScriptHook import contract and isolated loader tests |
| [`examples/`](examples/) | Runnable fake host–guest integration |
| [`tests/`](tests/) | Native regression suites, including child-process IPC |
| [`locales/`](locales/) | English and Simplified Chinese message catalogs |
| [`tools/`](tools/) | Catalog and documentation validation |
| [`docs/`](docs/README.md) | Architecture, compatibility, milestones, verification and contributor documentation |
| [`.github/`](.github/) | CI workflow, issue/PR templates and code ownership |

## Localization

The README is available in English and Simplified Chinese. Runtime messages are separate from the wire protocol: [`en-US.lang`](locales/en-US.lang) is the required English fallback and [`zh-CN.lang`](locales/zh-CN.lang) provides Simplified Chinese.

`Catalog(directory, requested_locale)` loads the catalogs once. `zh` and `zh-CN` select Simplified Chinese; other locale tags select English. Missing translated keys fall back to English, while a missing required catalog file is an error. There is no game UI or command-line language switch yet. Translation format and validation steps are in the [localization guide](docs/localization.md).

## Documentation

| Task | Read |
| --- | --- |
| Build or test the native plugin | [RDR2 native host](docs/rdr2-host.md) |
| Understand the protocol and rendering design | [Architecture](docs/architecture.md) |
| Check platform evidence or integration progress | [Compatibility](docs/compatibility.md), [milestones](docs/milestones.md), [baseline verification](docs/verification-2026-10-14.md) |
| Submit code or translations | [Contributing](CONTRIBUTING.md), [localization](docs/localization.md) |
| Report a security issue | [Security policy](SECURITY.md) |
| Inspect dependencies or prepare a release | [Third-party references](docs/references.md), [release procedure](docs/releasing.md), [changelog](CHANGELOG.md) |

The [documentation index](docs/README.md) also links the available Chinese guides.

## Contribute

Contributions are welcome in the bridge core, RDR2/Fabric adapters, rendering, regression tests, documentation and translations. Start with the [contribution guide](CONTRIBUTING.md), [engineering rules](AGENTS.md) and [code of conduct](CODE_OF_CONDUCT.md), then check [existing issues](https://github.com/ranzevor/Blockwater/issues) before beginning overlapping work.

Use the [issue templates](https://github.com/ranzevor/Blockwater/issues/new/choose) for reproducible bugs and feature proposals. Include the affected commit, environment, expected behavior, actual result and relevant sanitized diagnostics. Protocol and gameplay changes need regression evidence; game-compatibility reports need the corresponding live-game run. Report vulnerabilities through the [security policy](SECURITY.md), not a public issue with exploit details.

## Related work and license

The project draws on the approaches explored by [Minecraft × GTA V passthrough](https://github.com/rehan-remade/universal-modder/tree/main/examples/minecraft-gta5-passthrough) for Minecraft integration and composition, and [Garry's Redemption](https://github.com/codeByAlexff/garrys-redemption) for RDR2 integration. The [reference register](docs/references.md) identifies these projects and the native/modding tools relevant to future adapters. They are references, not runtime dependencies of the current bridge-core build.

Blockwater source is licensed under [MIT](LICENSE). Incorporated third-party source retains its applicable license and attribution. Game executables, extracted assets, saves and proprietary SDK binaries are not distributed with the project. Minecraft and Red Dead Redemption 2, including their trademarks and game assets, belong to their respective rights holders; Blockwater is an independent community project.
