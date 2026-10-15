# Architecture

## Runtime ownership

The RDR2 host owns world geometry, native NPC state, mission systems, traffic, law enforcement and game-world effects. Minecraft owns blocks, inventory, tools, entities and block interaction rules. Each runtime keeps its own original game installation and state. Cross-runtime actions are exchanged only after explicit ownership mapping and validation.

RDR2's script thread performs host callbacks. No IPC wait or file I/O is permitted on this frame loop. The core does not spawn, attach to or modify a game process on its own.

## Shared-memory ABI v1

`SharedBridge` has two independently owned frame snapshots, one event queue in each direction and one peer record per runtime. `kMagic`, `kVersion` and `sizeof(SharedBridge)` must match before a peer accesses the payload. Schema changes require a version increment.

- `HostFrame`: sequence, timestamp, session anchor, story-mode flag, camera, player feet, explicit transform origin, terrain observations and bounded nearby entity records.
- `GuestFrame`: sequence, timestamp, matching anchor, camera, player feet, velocity and validity flags.
- `Event`: monotonic ID within a peer epoch, sender and recipient epochs, a typed operation, target and location. The native host accepts only enumerated operation kinds and bounded scalar values for validation; unimplemented gameplay operations receive rejection.
- `EventRing`: single producer / single consumer. Full queues reject the push rather than silently overwrite unread events. The consumer owns the read cursor. Every payload word is atomically transferred, and snapshot sequence words detect partial reads.
- `PeerControl`: ready/refused/closed state, epoch, monotonic heartbeat and refusal reason. A peer must be observed advancing its heartbeat before being regarded as connected, and stale peers are excluded after two seconds.

The current protocol targets 64-bit little-endian desktops with lock-free 64-bit atomics. It is not a network protocol. Mapping permissions restrict access to the local user where supported; access by other programs running as the same user is not an authenticated channel. No arbitrary console commands or native function invocations are accepted through the event schema.

### Session transitions

Each host and guest run uses its own nonzero epoch. An event's sender epoch is stamped by the publishing endpoint. The simulated `HostAdapter` clears its block proxies and recently processed event IDs when the guest epoch changes. The native plugin instead invalidates its acquired world data and renews the transport session; it owns no gameplay objects yet. Events left over from the previous guest generation are discarded; responses targeting an old recipient epoch are also skipped. Refusal of multiplayer is sticky for that host endpoint lifetime.

### Coordinates and terrain publication

RDR2 uses metres and Z-up. Minecraft uses blocks and Y-up. `WorldTransform` defines a host origin, guest origin and positive scale; the forward transform maps `(x, y, z)` to `(x, z, -y)` relative to those origins. Block centres are mapped into host-space before requesting a physics proxy. Angular mappings will be added only after in-game camera convention validation.

The B0 simulated `HostAdapter` contract samples 16 terrain points each tick across a 17 × 17 local grid. Its example guest accumulates these incremental samples. This simulation is not the native terrain publisher.

The native `WorldStream` publishes the complete set of currently valid observations from its 9 × 9 grid at two-metre spacing, at most 81 samples per frame. Its consumer must replace the previous terrain observation set with each current snapshot, not accumulate omitted points indefinitely. Omission represents absent, pending, expired or invalidated observations, not a known empty volume or a synthetic floor. The native publisher currently reports zero entities. [World acquisition](rdr2-world.md) specifies query ownership, age limits, dispatch budgets, anchors and invalidation. Walls, fences, roofs and stacked floors require more complete geometry before full collision interoperability can be accepted.

### Gameplay callbacks

`HostAdapter::tick()` publishes a host snapshot and, once the guest is live, drains up to 32 bounded events per frame. A placed block is represented by a host object handle. The adapter refuses placements above 400 tracked objects, mirrors removal to the correct handle and cleans tracked objects on guest restart or adapter destruction. It forwards bounded explosion magnitudes and accepts NPC damage only for entities exposed in the most recently published host snapshot. Block placements and explosions must occur within a 96 m axis-aligned neighbourhood of the player. Rejected actions are retained in a bounded local response queue if the outbound ring is currently full.

The [RDR2 native component](rdr2-host.md) implements read-only game-context checks, transport ownership and actual native calls for camera, player and terrain acquisition. Automated tests execute these calls against an isolated runtime; live-game behavior remains unverified. It does not invoke gameplay callbacks or publish simulated world data. A real gameplay host must verify each spawned object model, wait for its asynchronous streaming state without blocking the game thread, and ensure that collision is suitable for characters, horses and wagons. Host-proxy limits must be calibrated in the real game.

## Image composition reference

`composite()` is a CPU oracle, not a GPU renderer. Its inputs are premultiplied linear RGBA layers for the host world, guest world and guest HUD, plus positive linear depths in metres. The world pixels are depth-tested before blending; the HUD overlays the resulting image. In-game rendering additionally requires host depth extraction, the current camera's view/projection matrices, reversed-Z normalization, reprojection and GPU resource-lifecycle handling.

## Localization boundary

Public-facing status and error text is keyed separately from the wire protocol and loaded from UTF-8 catalogs at startup. Messages are never IPC payload types; language selection does not affect game-state ownership, validation or event deduplication.

## Dependencies and validation

The bridge core builds without game files. The reference implementations and their respective licenses are recorded in [references.md](references.md). Host-native APIs must be checked against the RDR3 Native DB and validated in the target version of Story Mode before being treated as supported. Game rendering and live physics can only be accepted after in-game measurement.
