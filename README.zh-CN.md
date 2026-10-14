# Blockwater

**Minecraft × 荒野大镖客 2** —— 通过双运行时桥接，让玩家在《荒野大镖客 2》故事模式的世界中使用 Minecraft 的建造、交互与战斗机制。

[English](README.md) · [贡献指南](docs/zh-CN/contributing.md) · [安全报告](docs/zh-CN/security.md) · [开源许可](LICENSE)

## 当前进度

**B0：原生桥接核心。** 已实现并测试跨进程共享内存、容量受限的游戏事件、坐标转换、模拟 RDR2 宿主、CPU 图像合成参考实现和启动时加载的国际化资源。**目前尚不是能够安装或游玩的游戏 Mod。** ScriptHookRDR2 插件、Minecraft Fabric 客户端与 GPU 图像合成器尚待接入。

## 工作原理

- **RDR2** 负责场景、NPC、地形、车辆、任务及原生特效。
- **Minecraft** 负责方块规则、物品栏、工具与 MC 实体。
- **Blockwater** 通过版本化的本地共享内存协议交换状态与事件，由宿主适配器调用经验证的游戏接口，并拒绝不受支持的操作。
- 后续图形融合将使用 RDR2 相机与深度缓冲区合成 Minecraft 的颜色、深度及 HUD。当前只有 CPU 参考实现。

参见[技术架构](docs/architecture.md)、[开发里程碑](docs/milestones.md)和[兼容性记录](docs/compatibility.md)。

## 构建与测试

需要 CMake 3.20+ 和支持 C++20 的编译器。桥接核心不依赖两款游戏的安装文件。

```sh
cmake -S . -B build
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

构建后运行 `blockwater_fakes`，可以测试模拟宿主与 Minecraft 侧的事件路径。Visual Studio 多配置构建的程序一般位于 `build/Release/` 目录。

## 国际化

[`locales/`](locales/) 包含 `en-US` 英文和 `zh-CN` 简体中文资源，使用一致的消息键。`Catalog` 在启动时读取资源，缺失翻译回退到英文。语言选择尚未接入游戏内 UI。翻译贡献和验收要求参见[国际化文档](docs/zh-CN/localization.md)。

## 开源协作与边界

欢迎贡献，提交前请阅读[贡献指南](docs/zh-CN/contributing.md)与[社区行为准则](CODE_OF_CONDUCT.md)。PR 将执行 Linux、Windows 桥接核心 CI；CI 不以真实游戏运行作为已完成验证。

本项目不分发游戏资产、执行文件、存档或专有 SDK 二进制文件。使用者须自行取得合法授权的游戏副本。项目**仅针对 RDR2 故事模式**，不面向 Red Dead Online。

## 文档

- [文档索引](docs/README.md)
- [架构与通信协议](docs/architecture.md)
- [实现与验收阶段](docs/milestones.md)
- [兼容性与验证环境](docs/compatibility.md)
- [中文贡献指南](docs/zh-CN/contributing.md)
- [中文安全报告说明](docs/zh-CN/security.md)
- [第三方来源和许可边界](docs/references.md)
- [变更记录](CHANGELOG.md)

## 许可

Blockwater 源码使用 [MIT 许可](LICENSE)。引用的第三方项目保留各自的著作权与许可；Minecraft、《荒野大镖客 2》及其商标归各自权利人所有。Blockwater 是独立的社区项目。
