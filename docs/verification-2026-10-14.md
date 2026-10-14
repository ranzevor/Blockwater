# Bridge core verification — 2026-10-14

## Initial Linux baseline

| Environment | Build | Test |
| --- | --- | --- |
| Linux x86-64, GCC 14.2.0, C++20, Release | PASS | PASS |
| Linux x86-64, GCC 14.2.0, C++20, Debug with AddressSanitizer + UndefinedBehaviorSanitizer | PASS | PASS |
| Linux x86-64, Clang 17.0.0, C++20, Release | PASS | PASS |

The initial baseline passed the protocol, IPC, RDR2-host-simulation and compositor suites. The first-public-commit update adds a fifth localization suite and a separate fake-pair CTest entry. The GCC Release executable also passed 30 consecutive independent process runs.

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

## Subsequent cross-platform CI

Source commit `38c082fee28d6682766e41af28443ca482c35ab8` passed [CI run 3](https://github.com/ranzevor/Blockwater/actions/runs/38587538291) on 2026-10-14. Both `ubuntu-24.04` and `windows-2025` completed Release configuration, compilation and both CTest entries successfully. The Windows native test includes a separate child process opening the shared-memory mapping and returning an event. The catalog and documentation check passed in the Linux job.

The first Windows build exposed implicit cache-line padding warnings promoted to errors by `/W4 /WX`. The reserved bytes in `EventRing` and `PeerControl` now occupy the same ABI v1 gaps explicitly, with 13 compile-time layout assertions. The subsequent test build exposed a missing `<string>` include in the assertion header; the direct include resolves that dependency. The original warning policy and test assertions are unchanged.

GCC 14.2.0 and Clang 17.0.0 comparisons before and after the padding change preserve all original ring/control/shared-bridge field offsets, alignments and sizes: `EventRing` 11392 bytes, `PeerControl` 256 bytes and `SharedBridge` 30592 bytes. Linux Release and AddressSanitizer/UndefinedBehaviorSanitizer tests passed after the padding change; Clang Release tests also passed after the header dependency fix.

## Game integration not validated

No RDR2 executable, ScriptHookRDR2 runtime, Minecraft/Fabric installation, DX12 compositor or Windows game process was executed. These native-core tests do not establish in-game compatibility or a playable release.
