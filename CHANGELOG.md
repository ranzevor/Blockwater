# Changelog

Changes are recorded against released source tags. Protocol compatibility is tracked separately using `kVersion` in `include/blockwater/wire.hpp`.

## Unreleased

- Native C++20 host/guest bridge core with cross-process shared memory, atomic snapshots and bounded event queues.
- Simulated RDR2 host contracts for terrain samples, block proxies, explosions, entity damage and reconnection.
- CPU color/depth/HUD compositor reference and integration smoke test.
- English and Simplified Chinese startup-loaded language catalogs, with matching-key checks.
- Public contribution, security, localization and compatibility documentation; Linux/Windows pull-request CI.
- Complete English and Simplified Chinese README with build/run examples, expected output, troubleshooting and contributor entry points.
- Windows build fixes for explicit ABI-preserving cache-line padding and the test assertion header's direct `<string>` dependency; Linux and Windows CI passed.

- Peer heartbeat observation precedes local clock sampling, preventing concurrent heartbeats from causing false reconnects.
- Windows x64 ScriptHookRDR2 lifecycle plugin with read-only context checks, latched multiplayer refusal and script-thread session handling.
- Epoch renewal and invalidation across pause, script stalls, disconnect and reload; bounded rejection of unimplemented gameplay requests without changing ABI v1.
- Portable session regression tests, isolated Windows ASI loader scenarios and installation-manifest checks; bilingual native-host documentation.

A playable RDR2 build and Minecraft Fabric mod have not been released.
