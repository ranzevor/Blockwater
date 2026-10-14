# Contributing to Blockwater

[简体中文](docs/zh-CN/contributing.md)

## Before opening a pull request

Check existing issues and the [implementation milestones](docs/milestones.md). Describe the behavior you want to change and its observable acceptance condition. Keep the patch focused: bridge protocol, game adapter, renderer, documentation or localization.

1. Create a branch from the current default branch. Avoid unrelated formatting or file moves in behavioral changes.
2. Run the core build and tests from [README](README.md); run `python3 tools/validate_repository.py` for documentation and locale consistency.
3. Include regression tests when modifying protocol fields, lifecycle, event ordering, collision proxies or the compositor. A protocol layout change must increment `kVersion` and update both peers' compatibility behavior.
4. In the PR description, provide the exact environments tested, test results and behavior not tested. Fake-host tests do not establish RDR2 or Minecraft in-game compatibility.
5. Identify newly introduced dependencies and their licenses. Do not contribute proprietary game resources, extracted assets, binaries, saves, SDK binaries, copyrighted source from an unlicensed release, credentials or private device paths.

## Game integration requirements

All native game operations must stay on the verified game script thread and must not block a game frame for IPC or disk activity. RDR2 runtime integration must refuse multiplayer before any world-changing action. Cleanup must release all objects owned by a disconnected guest. Native hashes and signatures must come from documented, checkable sources and be verified against the intended RDR2 version before compatibility is claimed.

For bug reports, include reproducible steps, actual and expected behavior, compiler or game version, OS, and relevant sanitized diagnostics. Report security vulnerabilities [privately](SECURITY.md), not in a public issue.

## Localization

Edit both `locales/en-US.lang` and `locales/zh-CN.lang` when introducing a new key. Keep keys stable and translate user-visible meaning, not internal variable names. Details: [localization guide](docs/localization.md).

## Review and merging

Maintainers review functionality, protocol compatibility, source/asset rights, regression evidence and user-facing text. Automated checks are part of the review, not a substitute for the corresponding in-game or rights review. Do not publish game-compatible release claims until target versions and live-game acceptance are recorded.
