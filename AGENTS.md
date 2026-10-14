# Blockwater engineering rules

1. RDR2 Story Mode is the sole game integration target. Host code must refuse all multiplayer contexts before game-affecting callbacks and must never target Red Dead Online.
2. Do not commit game binaries, extracted assets, textures, audio, proprietary mod SDK binaries, user saves or personal environment paths. Retain license and attribution notices for incorporated third-party source.
3. `include/blockwater/wire.hpp` is the single shared-memory ABI authority. Increment the protocol version on incompatible layout changes. Every field used across processes must have a validated producer and consumer, with epoch transitions and stale-event handling.
4. Game modifications run on the verified ScriptHookRDR2 script thread; no per-frame blocking IPC, waiting, filesystem access or GPU readback. Restrict resources and explicitly release owned objects on disconnect/unload.
5. Preserve existing operation semantics and compatibility while changing code. Test valid and invalid payloads, both directions, queue saturation, reconnection, multiplayer refusal, coordinate conversions and depth occlusion.
6. Distinguish build/test results from live game validation. A playable release requires recorded successful Story Mode and actual renderer/physics tests with exact supported version information.
7. Changes that affect repository standards or dependencies require evidence-based review. No third-party implementation or demo claim becomes a validated Blockwater capability without an actual project test.
