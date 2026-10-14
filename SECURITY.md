# Security Policy

[简体中文](docs/zh-CN/security.md)

## Supported scope

Security fixes are accepted against the current `main` branch and any separately identified maintained release. There is no stable or playable release at present. The bridge includes shared-memory IPC and a game-side adapter contract; game process hooks and the renderer are not yet implemented.

## Reporting

Do not disclose a working exploit or private information in a public issue. If GitHub private vulnerability reporting is enabled for this repository, use **Security → Report a vulnerability**. Otherwise, contact the repository owner privately using a channel the owner has publicly provided. If no private channel is available, do not post exploit details publicly.

Include affected commit/version, impact, reproducible steps and a minimal demonstration without proprietary game assets or personal data. Maintainers will determine the affected scope and coordinate a fix through the repository's normal review process.

## Boundaries

The integration is Story Mode only. Reports involving attempts to bypass anti-cheat, modify Red Dead Online or gain unauthorized access to other players' systems are outside the supported use of Blockwater. The runtime must not silently fall back from a rejected multiplayer context into a write-capable mode.
