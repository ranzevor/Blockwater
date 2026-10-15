# RDR2 world acquisition

[简体中文](zh-CN/rdr2-world.md) · [Native host](rdr2-host.md)

## Published observations

The native plugin publishes ABI v1 `HostFrame` observations only after Story Mode gating and an advancing guest-heartbeat handshake. `NativeWorld` reads the final-rendered camera position, rotation order 2 and reported FOV. Host angles remain in degrees: yaw is rotation Z, pitch X and roll Y. Camera projection, FOV interpretation and Minecraft angle conversion still require game calibration; no gameplay-camera pose is mixed into this acquisition path.

Player feet are the world-space transform of the player model's local `(0, 0, bounds.min.z)`. This is a model-bounds reference, not a skeleton or animation measurement. Crouching, riding, altered player models and steep surfaces require game-run comparison before this reference can be treated as accurate contact geometry. Missing or invalid camera, FOV, model bounds or coordinates invalidates the complete snapshot instead of publishing zero coordinates.

Frames carry monotonic sequence numbers, acquisition timestamps, a nonzero anchor, a host origin at the anchoring feet, guest origin `(0, 64, 0)` and one metre per block. Session renewal, invalid observation recovery, a movement jump above 32 metres, travel beyond 128 metres from the origin, or a clock discontinuity reanchors and clears terrain. Anchor exhaustion invalidates output rather than reusing an old identifier. The guest must reject invalid or stale frames and rebuild anchor-relative data after an anchor change.

## Asynchronous terrain

`WorldStream` samples a 9 × 9 grid at two-metre spacing, nearest rings first. The grid has at most 81 valid observations within the existing ABI capacity of 96. Moving into another grid cell or changing reference height by more than two metres invalidates the grid. This is a local height-sampling representation, not a collision mesh for walls, fences, interiors or stacked floors.

A low ray runs from two metres above the reference feet to 80 metres below. A completed miss schedules a later high ray from 150 metres above to two metres above the reference. Shape-test flags select static world geometry, ignore the player and use collider option 4. Dynamic entities are not represented. A hit is accepted only when its finite point lies on the requested vertical segment and its finite normal has a plausible length. `TerrainSample.flags = 1` marks a measured hit; missing, invalid, expired or pending observations are omitted rather than replaced with a synthetic floor. The native entity count stays zero in this batch.

At most eight engine query handles are owned simultaneously. Each tick polls each occupied slot at most once and can start at most eight new probes. Dispatch stops when the elapsed acquisition time reaches the 500-microsecond budget; the budget cannot preempt an individual native call and is not a measured game frame-time guarantee. No synchronous expensive raycast, waiting loop or per-frame heap allocation is used.

Results older than 250 milliseconds are discarded. A timed-out handle stays owned and is polled until the engine reports completion or invalidity; it is not replaced while pending. Consequently a stalled engine can stop terrain progress but cannot cause unbounded handle allocation. Valid samples expire after 500 milliseconds. Pause, disconnect and multiplayer refusal stop acquisition; retained queries are reaped only after the script resumes in a permitted connected session. Their old results cannot enter the new grid. Unload performs no natives under the loader lock; genuine engine reclamation of pending queries must be checked during live reload acceptance.

## Native contract

Hashes and signatures come from the [RDR3 Native DB](https://github.com/alloc8or/rdr3-nativedb-data/blob/master/natives.json), inspected at blob `292f072816470c5dee885f4270861db38bffd406`. ScriptHook argument and vector layouts follow the integration declarations documented in the [host register](rdr2-host.md#native-interface-register). Source inspection does not establish game-version compatibility.

| Native | Hash | Signature |
| --- | --- | --- |
| `GET_FINAL_RENDERED_CAM_COORD` | `0x5352E025EC2B416F` | `Vector3()` |
| `GET_FINAL_RENDERED_CAM_ROT` | `0x602685BD85DD26CA` | `Vector3(int rotationOrder)` |
| `GET_FINAL_RENDERED_CAM_FOV` | `0x04AF77971E508F6A` | `float()` |
| `GET_ENTITY_MODEL` | `0xDA76A9F39210D365` | `Hash(Entity)` |
| `GET_MODEL_DIMENSIONS` | `0xDCB8DDD5D054A7E7` | `void(Hash, Vector3* min, Vector3* max)` |
| `GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS` | `0x1899F328B0E12848` | `Vector3(Entity, float x, float y, float z)` |
| `START_SHAPE_TEST_LOS_PROBE` | `0x7EE9F5D83DD4F90E` | `int(float x1, float y1, float z1, float x2, float y2, float z2, int flags, Entity ignore, int options)` |
| `GET_SHAPE_TEST_RESULT` | `0xEDE8AC7C5108FB1D` | `int(int handle, BOOL* hit, Vector3* point, Vector3* normal, Entity* entity)` |

Integers and float bit patterns occupy the low bytes of one zero-initialized 64-bit argument slot; pointers retain their complete x64 value. Native vectors use three eight-byte slots, with float components at offsets 0, 8 and 16. Output buffers start with NaN values so an unwritten result cannot become a valid origin. `GET_MODEL_DIMENSIONS` is a void call, so its output values, not its return pointer, determine success. Probe status 0 releases an invalid handle, 1 retains a pending handle and 2 consumes a completed result. Missing or unknown status retains ownership without treating the output as a hit.

## Tests and acceptance

`blockwater_world_stream` covers acquisition, finite-value rejection, sample expiry, dispatch budget, failed starts, clock rollback, reanchoring, bounded pending queries, counter exhaustion and native value encoding. The Windows loader adds eight real-ASI scenarios for invalid camera, absent camera result, invalid bounds, teleport, pause, pending terrain, misses and invalid normals. Its test runtime independently checks native hashes, argument counts, scalar bits, output-vector layout, player exclusion and script-fiber ownership. Existing lifecycle, command-rejection and installation tests remain.

[CI run 8](https://github.com/ranzevor/Blockwater/actions/runs/38656719116) tested source `8c4aaba9ce27d5121a4ffb5bfa24e09938d6325e`: Windows x64/MSVC built the plugin and passed 31 CTest entries; Ubuntu passed its build and tests. The nine portable acquisition/value cases also passed GCC, Clang and ASan/UBSan in Linux; the GCC suite passed 30 repetitions.

Live acceptance must record actual RDR2 and ScriptHook versions, compare final camera/FOV and model-bounds feet against the game, measure query/loop costs, exercise terrain streaming near roofs and loading boundaries, and check pending-query cleanup across unload/reload. Real collision objects, nearby-entity publication, Fabric integration and GPU compositing remain separate work. This batch does not establish a playable release or complete B1.
