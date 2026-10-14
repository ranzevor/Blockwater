# Blockwater

Minecraft × Red Dead Redemption 2 passthrough integration.

The goal is to bring Minecraft's native building, tools, physics, and entities into the landscape and interactions of Red Dead Redemption 2's Story Mode, while retaining both games as separate runtimes.

## Source tree

| Path | Component |
| --- | --- |
| `include/blockwater/wire.hpp` | Versioned shared-memory layout, atomic frame snapshots, bounded event queues |
| `include/blockwater/coordinates.hpp` | RDR2 metres/Z-up ↔ Minecraft blocks/Y-up transforms |
| `include/blockwater/link.hpp` | Peer lifecycle, heartbeat, event ownership, game-mode rejection |
| `src/mapping.cpp` | Windows named mappings and POSIX mappings for integration tests |
| `src/host_adapter.cpp` | RDR2 host contract: terrain samples, block proxies, explosions, damage events |
| `src/compositor.cpp` | CPU reference for colour/depth/HUD compositing |
| `examples/fake_pair.cpp` | Run the host–guest gameplay flow without either game |
| `tests/` | Protocol, cross-process IPC, restart, collision proxy, and compositing tests |

## Build and test

Requires CMake 3.20+ and a C++20 compiler. No game files or third-party libraries are required to build the bridge core.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/blockwater_fakes
```

On Windows, the last executable is `build/Release/blockwater_fakes.exe` for a multi-config Visual Studio build, or `build/blockwater_fakes.exe` for a single-config build.

The fake pair must print the host frame, a mirrored explosion and one created block proxy. The tests additionally exercise event queue overflow, process sharing on POSIX, peer restarts, duplicate events, session refusal and depth occlusion.

## Integration boundary

The bridge core and simulated host are implemented. It does **not yet install into either game**. The current source does not contain a ScriptHookRDR2 `.asi`, Fabric `.jar`, DirectX compositor or playable RDR2 integration. Refer to [integration milestones](docs/milestones.md) for the acceptance criteria of these components.

The RDR2 adapter contract is defined against the game's script-thread operations. The Minecraft side will use Fabric, following the approach in the open-source [Minecraft × GTA V passthrough](https://github.com/rehan-remade/universal-modder/tree/main/examples/minecraft-gta5-passthrough). RDR2 Native API mappings are being evaluated against [Garry's Redemption](https://github.com/codeByAlexff/garrys-redemption).

## Runtime boundaries

Only RDR2 **Story Mode** is in scope. The game-side adapter must refuse multiplayer before invoking game-affecting actions. Users must own both games. No game executable, assets, proprietary SDK binaries or game data may be included in this repository or release packages.

See [architecture](docs/architecture.md), [milestones](docs/milestones.md) and [third-party references](docs/references.md).

## License

MIT — see [LICENSE](LICENSE).
