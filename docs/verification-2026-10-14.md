# Bridge core verification — 2026-10-14

## Build matrix

| Environment | Build | Test |
| --- | --- | --- |
| Linux x86-64, GCC 14.2.0, C++20, Release | PASS | PASS |
| Linux x86-64, GCC 14.2.0, C++20, Debug with AddressSanitizer + UndefinedBehaviorSanitizer | PASS | PASS |
| Linux x86-64, Clang 17.0.0, C++20, Release | PASS | PASS |

`ctest --output-on-failure` reports The initial baseline passed the protocol, IPC, RDR2-host-simulation and compositor suites. The first-public-commit update adds a fifth localization suite and a separate fake-pair CTest entry. The GCC Release executable also passed 30 consecutive independent process runs.

## Executed simulation

```text
Host frame=1 ground samples=16
Explosion propagated: radius=4
Host block proxies=1
```

## Validated boundaries

- Atomic snapshot transfer while a writer is active; invalid payload rejection.
- Shared-memory mapping creation, second-process opening, mapping exclusivity and cleanup on POSIX.
- Event ordering, full-queue backpressure, sequence wrap over the ring and peer heartbeat/epoch handling.
- Simulated host world samples; block proxy creation, removal, duplicate suppression, 400-proxy cap and cleanup.
- Guest reconnect discards old-session events and releases previously owned block proxies.
- Local NPC target validation, bounded explosions, out-of-range rejection and multiplayer refusal.
- Depth occlusion, HUD overlay, alpha composition and dimension validation in the CPU image oracle.

## Not validated

No RDR2 executable, ScriptHookRDR2 runtime, Minecraft/Fabric installation, DX12 compositor or Windows game process was executed. A Windows child-process IPC test has been added but has not yet been executed under Windows. Consequently, no in-game compatibility claim or playable release is established.
