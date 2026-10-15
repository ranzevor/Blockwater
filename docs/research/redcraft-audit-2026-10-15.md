# RedCraft release and reuse assessment

[简体中文](redcraft-audit-2026-10-15.zh-CN.md)

**Inspected:** 2026-10-15. **Reference:** Keel62155/Minecraft-X-Games, `3.0.0-RedCraft`. **Blockwater baseline:** `bdd2e93bcdbb25e5bbffcca3b33e8e5e6c2a9ce6`.

## Conclusion

RedCraft is an existing Minecraft/RDR2 implementation with a downloadable plugin, an attached source archive, DX12 rendering and host interaction code. Repeating the same game pairing is not a novelty claim for Blockwater. However, the inspected RedCraft source is **not an immediately reusable open-source base**: its own notice expressly says no license has been chosen for its own code and requests permission before reuse.

Retain Blockwater's existing baseline. Do not import RedCraft's implementation or resume the complete RDR2 roadmap solely because development has started. The strongest remaining gameplay hypothesis is Minecraft construction that changes **RDR2-native collision for people, horses and wagons**, rather than only Minecraft-side movement. That hypothesis remains unverified for RedCraft and unimplemented in Blockwater; it is not an established competitive advantage.

## Inspected artifacts

The [release](https://github.com/Keel62155/Minecraft-X-Games/releases/tag/3.0.0-RedCraft) was published on **2026-10-04 at 19:15:15 UTC** (2026-10-05 at 03:15:15 UTC+08:00). Its attached `RedCraft.zip` is **149,274 bytes**. Downloaded bytes matched the release asset's SHA-256:

```text
f003eb89654b7d3ac09c6fd211cf9902c46c7c68d4f18156bcfb9b63038901d1
```

The outer archive contains four files: an installation README, `RedCraft.asi`, the `RedCraft.fx` shader, and `RedCraft-source.zip`. The inner archive contains 19 files: C++ source/headers, CMake configuration, the shader, documentation and third-party notices. No Java/Fabric source or external SDK is bundled in this archive.

| Artifact | SHA-256 |
| --- | --- |
| `RedCraft/RedCraft.asi` | `7c0fe25c7e7190c1b628769f5d71fc313865c78803fe095f20285d63bd3b161d` |
| `RedCraft/RedCraft-source.zip` | `6eadf1a436188f9e4c1243419108830575edd6871b7690ecd1d2f87b17d68269` |
| Inner `RedCraft/THIRD-PARTY-NOTICES.md` | `c1d135aa172a8c5c56e5df7761718ff7c0397adba5dc2b58aeddf8890163fc1f` |

The [tag's Git tree](https://github.com/Keel62155/Minecraft-X-Games/tree/3.0.0-RedCraft) contains only README and Git attributes. GitHub's automatically generated source archive is therefore not the implementation archive examined here. Matching the download digest establishes artifact identity, not equivalence between the bundled binary and source.

## License scope

The inner `THIRD-PARTY-NOTICES.md`, lines 16–17, states:

> No license has been chosen for RedCraft's own source yet.

The following sentence allows reading and asks permission before reuse. The same notice identifies SkyCraft's protocol/Fabric dependency as MIT and ReShade as BSD-3-Clause, while Script Hook RDR2 and its SDK must be obtained separately. Those dependency notices do not license RedCraft's own adapter. The notice also attributes depth-compositing ideas to universal-modder while explicitly stating that no code from that project is included.

**Reuse decision:** do not copy, translate or adapt the inspected RedCraft implementation into MIT-licensed Blockwater without an applicable grant. An upstream component with an explicit license can instead be assessed directly at its own source and version. Reading or GitHub's fork affordance does not provide a blanket grant to redistribute modified source; see [GitHub's licensing guidance](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/licensing-a-repository). No permission request or grant is recorded by this assessment.

## What the source structure establishes

The evidence below comes from archive inventory and comment/string-stripped function/call indexing. Function names, native hashes and call paths were reviewed together. This is a static structural assessment, not an exhaustive control-flow review, successful build or game test.

| Area | Observed source evidence | Interpretation |
| --- | --- | --- |
| Minecraft connection | `src/Link.cpp` creates/maps shared memory and implements heartbeat, state, render, actor and event channels. The package README requires the external SkyCraft Minecraft instance. | It uses an existing Minecraft runtime/bridge rather than shipping a new Fabric implementation. |
| Rendering | `src/Renderer.cpp:771–912` builds sections and consumes render messages; `922–1444` handles geometry, overlays and DX12 drawing. `src/ReShadeBridge.cpp:48–107` binds compositor resources to ReShade. | A mixed geometry/image pipeline: section meshes are rendered on the host, with image/overlay content and depth composition. It is not a drop-in copy of the GTA full-frame pixel bridge. |
| Host terrain into Minecraft | `src/Collision.cpp:409–688` constructs regions and sends them through `WriteCollision`; `src/Link.cpp:121–148` writes the collision channel. | Positive evidence for RDR2-world sampling and Minecraft-side collision representation. This alone does not make placed MC blocks physical obstacles to RDR2 entities. |
| Combat and fire | `src/RedCraft.cpp:521–537` implements `DamageNpc`; its `Tick` at `538–1127` calls event draining, health updates, native explosions, fire and actor publication. | Host interaction has implementation paths beyond a visual model replacement. Runtime damage correctness, attribution and duplicate-event behavior were not tested. |
| Control handover | `SetDriving` at `476–508` manages camera/player state. `Tick` checks mounts, vehicles, pause and player control. | Source paths exist for transferring control; success across all missions or transitions is not established. |
| Native block colliders | The scanned call structure did not contain the conventional `CREATE_OBJECT` / model-request / block-prop path or the known RDR2 object-creation hash. `Collision.cpp` sends sampled regions toward MC. | Native people/horse/wagon obstruction by placed blocks remains unverified. Absence from this scan is not proof that every possible collision mechanism is absent. |
| Tests and distribution | The inner archive has no test directory and its CMake file registers no CTest tests. SDK and ReShade headers are external. | No automated regression suite was supplied in this inspected source bundle. This does not mean the author never tested the game or that external dependencies lack tests. |

The package documents a DX12 path and ReShade with add-on support for host-world occlusion. Its source README names RDR2 `1.0.1491.50` and Script Hook `1.0.1491.17`; vendor instructions name ReShade headers `6.8.0`. These are documented conditions, not a newly verified compatibility matrix. Vulkan support, current game builds, long-session performance and save/restart behavior were not validated.

## Comparison with Blockwater

Blockwater's state is taken from its [pinned README](https://github.com/ranzevor/Blockwater/blob/bdd2e93bcdbb25e5bbffcca3b33e8e5e6c2a9ce6/README.md), not from planned milestones.

| Capability | Inspected RedCraft package | Blockwater baseline |
| --- | --- | --- |
| Downloadable game adapter | `.asi` and shader are distributed; not run in this assessment | Native adapter builds in CI; no playable release |
| Connected Minecraft guest | Package uses SkyCraft; corresponding host protocol paths exist | No Fabric guest integration |
| Real-time game rendering | DX12 geometry/image renderer and ReShade bridge present | CPU reference compositor; no game GPU integration |
| Host world collection | Terrain/collision-region, player and actor code present | Camera/player and bounded asynchronous terrain collection; engineering tests, no live game validation |
| Explosions and NPC damage | Event/native call paths present | Mock/core contracts only; native requests remain rejected |
| Placed blocks obstruct native wagons/NPCs | Not established by this inspection | Not implemented |
| Reusable adapter license | Own-code license expressly not selected | MIT project |
| Engineering evidence | Source package contains no automated test suite | Repository has Linux/Windows CI and lifecycle/collection regression tests |

Documentation, licensing and test infrastructure matter to contributors, but do not by themselves compensate for missing playable functionality. Conversely, the existence of a download is not proof of stable performance or complete physical integration.

## Other project with the same name

The [Melty RedCraft listing](https://melty.gg/m/redcraft-2) is attributed to Tate Blackmore / QQave and advertises building, occlusion, weapon damage, explosions and control handover. No source/version lineage linking that listing to Keel62155's `3.0.0-RedCraft` was established. Its feature claims must not be silently attached to the inspected GitHub artifact. Its downloadable code, license and gameplay were not inspected here.

## Decision options and next evidence

| Option | Assessment |
| --- | --- |
| Use RedCraft to experience the existing crossover | Relevant to playing the idea; not a replacement for owning an open-source project. Installation and gameplay still require verification. |
| Fork RedCraft and improve it immediately | Blocked by the inspected own-code permission notice. A suitable grant must precede code reuse. |
| Continue Blockwater as an independent implementation | Worth considering only with an explicit gameplay or research objective beyond reproducing the pairing. MIT licensing is not enough on its own. |
| Change the target game | Reasonable if novelty is the priority or a meaningful RDR2 difference cannot be demonstrated. No replacement game has been selected or implemented. |

Before treating native construction physics as a reason to resume RDR2-specific work, run a controlled scene with the same RedCraft artifact: place a wall; test a Minecraft-controlled player, the RDR2-controlled player, an NPC and a horse/wagon separately; remove the wall and repeat. Record collision response versus visual occlusion, then repeat after control handover and restart. This distinguishes MC-side movement from native physical obstruction.

The recommendation is to preserve the baseline and avoid the next broad implementation batch until that difference is demonstrated or a different research objective is chosen. Do not narrow the promised full game integration to hide missing core behavior.

## Verification record

The [inventory run](https://github.com/ranzevor/Blockwater/actions/runs/38665955919) and [focused license/call-structure run](https://github.com/ranzevor/Blockwater/actions/runs/38666508382) completed successfully on 2026-10-15. The latter used audit code `15c45d3188c39edf24b7fa719915e259cf1017b8`. The scripts verify SHA-256, bound archive size/nesting and inspect text without running packaged code. The archive, binary and implementation source were not added to Blockwater.

Those runs prove download identity and inspection completion only. RedCraft compilation, binary/source correspondence, real game execution, visual fidelity, stability and physical behavior remain unverified. Findings and inspection tools are retained on `research/redcraft-release-audit`; the application baseline remains separate.
