# Blockwater

**Minecraft × 荒野大镖客 2 —— 将方块建造带入西部世界。**

[English](README.md) · [简体中文](README.zh-CN.md) · [文档](docs/README.md) · [贡献指南](docs/zh-CN/contributing.md) · [CI](https://github.com/ranzevor/Blockwater/actions/workflows/ci.yml) · [MIT 许可](LICENSE)

Blockwater 是一个开源的双运行时桥接项目，目标是在**《荒野大镖客 2》故事模式**中使用 Minecraft 的建造、工具与生物系统。两款游戏同时运行：Minecraft 提供沙盒规则，RDR2 提供原生世界，本地桥接层连接两边的交互。它不是 Minecraft 主题材质包，也不是对其中一款游戏的重新制作。

目标玩法包括：搭建能挡住 RDR2 角色或马车的方块墙，让 TNT 爆炸影响周围世界，以及让 Minecraft 生物与 RDR2 NPC 互动。

**当前阶段：RDR2 适配器开发。** 桥接核心和首批 ScriptHookRDR2 生命周期组件已实现。Windows x64/MSVC 可构建带有游戏状态检查及会话处理的 `Blockwater.asi`。真实相机/地形采集、玩法效果、Fabric 客户端和 GPU 合成器仍需接入，目前没有可游玩的游戏发行版。

[当前能力](#当前能力) · [工作原理](#工作原理) · [构建与运行](#构建与运行) · [兼容性](#兼容性) · [路线图](#路线图) · [参与贡献](#参与贡献)

## 当前能力

| 组件 | 已实现内容 |
| --- | --- |
| 本地桥接 | Windows 命名映射与 POSIX 共享内存、版本和大小校验、原子帧快照，以及双向有界事件队列 |
| 连接生命周期 | 心跳、会话代次、旧事件拒绝、重复抑制，以及宿主契约中的多人模式拒绝逻辑 |
| 世界映射 | RDR2 米制/Z 轴向上与 Minecraft 方块/Y 轴向上之间的位置、比例转换；增量地形采样与附近实体快照 |
| 宿主适配器 | 模拟方块碰撞代理的创建与移除、爆炸和实体伤害回调、输入边界，以及客体重启时的所属对象清理 |
| RDR2 原生生命周期 | ScriptHook 注册、只读游戏状态检查、暂停/断连/重载处理，以及未接入玩法请求的明确拒绝；详见[原生宿主指南](docs/zh-CN/rdr2-host.md) |
| 图像合成 | 世界图层深度测试与 HUD 混合的 CPU 参考实现；输入是准备好的图像和深度，不是实时游戏画面 |
| 国际化 | 启动时加载英文与简体中文消息资源，缺失译文键时回退英文 |
| 开发测试 | 协议、子进程 IPC、宿主模拟、CPU 合成和国际化测试套件，以及独立运行的模拟双端示例 |

[技术架构](docs/architecture.md)定义协议和当前资源限制；[验证记录](docs/verification-2026-10-14.md)记录 Linux 基线，[CI 运行记录](https://github.com/ranzevor/Blockwater/actions/workflows/ci.yml)提供后续各被测提交的自动检查结果。

## 工作原理

目标架构通过分工连接两款游戏，而不是在 RDR2 中重写一套 Minecraft：

| 运行时 | 负责内容 | 桥接交互 |
| --- | --- | --- |
| RDR2 宿主 | 地貌、原生 NPC、马匹、马车、任务与世界特效 | 发布相机、地形和实体状态；通过游戏原生回调执行映射后的方块碰撞、爆炸和伤害 |
| Minecraft 客体 | 方块、物品栏、工具、生物与 Minecraft 规则 | 接收宿主状态和碰撞数据；发布 Minecraft 交互事件与渲染图层 |
| Blockwater | 坐标转换、状态交换、事件校验和渲染接入 | 连接两个运行时，保持状态归属和会话边界明确 |

**状态链路**使用版本化的本地共享内存。计划中的**图形链路**导出 Minecraft 世界颜色、深度和 HUD，再依据 RDR2 的相机与深度缓冲区合成画面。当前共享内存 ABI 传输状态与事件；GPU 帧传输、深度提取与重投影仍需独立接入。

游戏原生操作在 RDR2 脚本线程中执行。桥接核心本身不会启动或附加到游戏进程。生命周期、坐标与回调契约见[运行时归属及 ABI 设计](docs/architecture.md)。

## 构建与运行

### 环境要求

构建桥接核心**不需要安装游戏、登录游戏账号、准备 Mod SDK 或引入第三方 C++ 库**。

| 要求 | 说明 |
| --- | --- |
| 平台 | 支持无锁 64 位原子操作的 64 位小端桌面环境；当前开发目标为 Linux x86-64 和 Windows x64 |
| C++ 工具链 | 支持 C++20，包括 `std::atomic_ref`；Linux 使用 GCC/Clang，Windows 使用 MSVC |
| CMake | 核心要求 3.20+；下方使用的 [Visual Studio 2022 生成器](https://cmake.org/cmake/help/latest/generator/Visual%20Studio%2017%202022.html)要求 **3.21+** |
| 构建工具 | Linux 使用 Make 或 Ninja；Windows 示例使用已安装 C++ 桌面开发工具和 Windows SDK 的 Visual Studio 2022 |
| Python | 3.9+，仅用于消息资源和文档校验；构建 C++ 核心不需要 Python |
| Git | 用于执行下方的源码获取命令 |

获取源码，后续命令均从仓库根目录执行：

```sh
git clone https://github.com/ranzevor/Blockwater.git
cd Blockwater
```

### Linux

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
./build/blockwater_fakes
python3 tools/validate_repository.py
```

### Windows — PowerShell 与 Visual Studio 2022

安装 C++ 工具链，并在能够使用 CMake、Git 和 Python 的 PowerShell 会话中执行：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
.\build\Release\blockwater_fakes.exe
python tools/validate_repository.py
```

Visual Studio 使用多配置生成器：构建时保留 `--config Release`，CTest 保留 `-C Release`。Ninja 等单配置生成器会将程序直接放在 `build/` 中。切换生成器时应使用新的构建目录。

### 示例与预期结果

两组命令均运行 [`blockwater_fakes`](examples/fake_pair.cpp)。示例创建共享内存映射，在**同一个进程内**模拟宿主和客体，发布地形快照、放置一个方块代理，并传递一次爆炸事件：

```text
Host frame=1 ground samples=16
Explosion propagated: radius=4
Host block proxies=1
```

成功时程序退出码为 `0`。输出来自模拟回调，不会打开游戏窗口或修改 RDR2。独立的子进程共享内存场景由 [`tests/test_ipc.cpp`](tests/test_ipc.cpp) 验证。

可移植构建产物为静态库 `blockwater_core`、测试程序 `blockwater_tests` 和 `blockwater_session_tests`，以及示例 `blockwater_fakes`。Windows x64/MSVC 默认额外构建 `build/rdr2/plugin/Release/Blockwater.asi`；使用 `-DBLOCKWATER_BUILD_RDR2=OFF` 仅开发核心。[原生宿主指南](docs/zh-CN/rdr2-host.md)说明加载测试与隔离暂存方式；当前不生成 Fabric `.jar` 或可玩安装器。

### 测试与常见问题

CTest 注册了 `blockwater_tests`、`blockwater_fake_pair` 和 `blockwater_host_session`，Windows 原生构建还注册 `rdr2_` 加载与暂存测试。前者覆盖帧一致性、非法输入、队列满载、对端重启、重复事件、多人模式拒绝、坐标转换、宿主回调、深度遮挡与语言查询。Python 校验器检查 UTF-8 消息资源、消息键和占位符的一致性，以及 Markdown 本地文件链接。

| 问题 | 处理方式 |
| --- | --- |
| CMake 找不到编译器，或不支持 C++20/所需原子操作 | 选择支持 C++20 的 64 位工具链；Windows 还需检查 C++ 桌面开发工具与 Windows SDK |
| Visual Studio 构建成功，但找不到程序或测试 | 使用一致的 `Release` 配置，并检查 `build/Release/` 下的程序路径 |
| 同时启动两次示例时出现映射创建错误 | 同一时间只运行一个 `blockwater_fakes` 实例；示例使用固定的映射名称 |
| 消息资源或文档校验失败 | 从仓库根目录执行，检查报错指向的资源或链接；`locales/` 下的两份文件均须存在 |

[CI 工作流](.github/workflows/ci.yml)在 `ubuntu-24.04` 和 `windows-2025` 上构建并运行 CTest，消息资源与文档检查在 Linux 任务中执行。工作流由 PR 或手动操作触发，不自动发布发行版。

## 兼容性

**两款游戏的接入目标为 Windows x64。** Linux 用于桥接核心开发和测试，不代表已经建立 Linux 游戏接入方案。已记录的编译器验证结果及游戏组件状态统一维护在[兼容矩阵](docs/compatibility.md)。

Minecraft 客体以 **Java Edition / Fabric** 为目标。Minecraft/Fabric、RDR2、ScriptHookRDR2、图形 API 和驱动版本须在对应游戏测试完成后才能列为受支持版本。参考项目使用的版本不等于 Blockwater 的兼容矩阵。

项目只面向 RDR2 **故事模式**，不支持 Red Dead Online。游戏接入需要分别取得两款游戏的合法授权副本，并准备适用的 Mod 依赖。提交兼容性结果前，请核对[游戏实测验收项](docs/milestones.md#required-game-validation)。

## 路线图

| 阶段 | 交付目标 |
| --- | --- |
| B0 — 桥接核心 | 已实现：原生协议、宿主契约、模拟运行、CPU 合成参考实现和国际化；各平台验证状态单独记录 |
| B1 — RDR2 适配器 | ScriptHookRDR2 插件，接入相机与地形、真实方块碰撞对象、故事模式检查及安全卸载 |
| B2 — Minecraft 客体 | Fabric 客户端，接收地形、产生方块和爆炸事件、导出渲染图层并处理重连 |
| B3 — 渲染 | DX12/ReShade 接入，完成真实宿主深度获取、相机校准、深度合成与重投影 |
| B4 — 首个可玩场景 | 在可复现的场景中放置、移除能阻挡 RDR2 角色或车辆的 MC 方块，触发对应的 TNT 爆炸并验证世界遮挡 |
| B5 — 扩展玩法 | 跨游戏伤害与生物代理、装备交互，以及跨游戏重启的持久化 |

各阶段验收条件及依赖见[接入里程碑](docs/milestones.md)。发行检查见[发布流程](docs/releasing.md)，实际变更记录在 [CHANGELOG](CHANGELOG.md) 中。

## 仓库结构

| 路径 | 用途 |
| --- | --- |
| [`include/blockwater/`](include/blockwater/) | 核心公开头文件；`wire.hpp` 定义共享内存 ABI，`link.hpp` 定义对端生命周期，`coordinates.hpp` 定义世界坐标转换 |
| [`src/`](src/) | 共享内存映射、宿主适配器、CPU 合成器与语言资源加载 |
| [`rdr2/`](rdr2/) | Windows 原生插件、ScriptHook 导入契约与隔离加载测试 |
| [`examples/`](examples/) | 可运行的模拟宿主与客体示例 |
| [`tests/`](tests/) | 原生回归测试，包括子进程 IPC |
| [`locales/`](locales/) | 英文与简体中文消息资源 |
| [`tools/`](tools/) | 消息资源与文档校验 |
| [`docs/`](docs/README.md) | 架构、兼容性、里程碑、验证记录与贡献文档 |
| [`.github/`](.github/) | CI 工作流、Issue/PR 模板与代码维护归属 |

## 国际化

README 提供英文和简体中文版本。运行时消息与通信协议分离：[`en-US.lang`](locales/en-US.lang) 是必需的英文回退资源，[`zh-CN.lang`](locales/zh-CN.lang) 提供简体中文。

`Catalog(directory, requested_locale)` 一次性加载消息资源。`zh` 和 `zh-CN` 选择简体中文，其他语言标记选择英文。缺失译文键时回退英文，缺少必需的资源文件则报错。目前没有游戏内界面或命令行语言切换参数。翻译格式与验收步骤见[国际化指南](docs/zh-CN/localization.md)。

## 文档

| 任务 | 阅读入口 |
| --- | --- |
| 构建或测试原生插件 | [RDR2 原生宿主](docs/zh-CN/rdr2-host.md) |
| 理解协议与渲染设计 | [技术架构](docs/architecture.md) |
| 核对平台证据或接入进度 | [兼容性](docs/compatibility.md)、[里程碑](docs/milestones.md)、[基线验证](docs/verification-2026-10-14.md) |
| 提交代码或翻译 | [贡献指南](docs/zh-CN/contributing.md)、[国际化](docs/zh-CN/localization.md) |
| 报告安全问题 | [安全报告说明](docs/zh-CN/security.md) |
| 审查依赖或准备发行 | [第三方来源](docs/references.md)、[发布流程](docs/releasing.md)、[变更记录](CHANGELOG.md) |

完整入口及已有中文指南见[文档索引](docs/README.md)。

## 参与贡献

欢迎参与桥接核心、RDR2/Fabric 适配器、渲染、回归测试、文档与翻译。开始前请阅读[贡献指南](docs/zh-CN/contributing.md)、[工程规则](AGENTS.md)和[社区行为准则](CODE_OF_CONDUCT.md)，并查看[已有 Issue](https://github.com/ranzevor/Blockwater/issues)，避免重复处理同一项工作。

使用 [Issue 模板](https://github.com/ranzevor/Blockwater/issues/new/choose)提交可复现的问题或功能建议，提供受影响提交、运行环境、预期行为、实际结果和必要的脱敏诊断。协议与玩法改动需要回归证据，游戏兼容性报告需要对应的实机运行记录。漏洞请通过[安全报告说明](docs/zh-CN/security.md)中的渠道反馈，不在公开 Issue 中披露利用细节。

## 参考项目与许可

项目参考了 [Minecraft × GTA V passthrough](https://github.com/rehan-remade/universal-modder/tree/main/examples/minecraft-gta5-passthrough) 的 Minecraft 接入与画面合成思路，以及 [Garry's Redemption](https://github.com/codeByAlexff/garrys-redemption) 的 RDR2 接入方式。[来源清单](docs/references.md)记录了这些项目和后续适配涉及的原生接口、Mod 工具。它们是技术参考，不是当前桥接核心构建的运行依赖。

Blockwater 源码采用 [MIT 许可](LICENSE)。引入的第三方源码须保留适用的许可和署名；项目不分发游戏执行文件、提取的资产、存档或专有 SDK 二进制文件。Minecraft、《荒野大镖客 2》及其商标、游戏资产归各自权利人所有，Blockwater 是独立的社区项目。
