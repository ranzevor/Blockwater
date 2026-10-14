# Release procedure

1. Record the exact source commit, protocol `kVersion`, supported OS/compiler or game versions, and any new dependency or asset licenses.
2. Run the native build and tests on supported CI targets; inspect the actual build artifact and verify that no private paths, secrets, third-party game binaries, extracted assets or development data are packaged.
3. For any **game-playable** release, separately exercise the [game acceptance scenarios](milestones.md) on the target RDR2 Story Mode installation and Minecraft client. A native core build is not a playable mod.
4. Update the [compatibility matrix](compatibility.md), release notes and [changelog](../CHANGELOG.md) with only tested functionality. Keep protocol ABI incompatibilities explicit.
5. Publish a source tag and any approved artifacts from the verified commit. Do not enable automatic game binaries or release publishing without separately verifying the redistributable content and production rights.

The first release remains untagged. The current CMake project version identifies source/API development, not a claim of supported game compatibility.
