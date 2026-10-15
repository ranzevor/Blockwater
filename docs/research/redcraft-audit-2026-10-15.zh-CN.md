# RedCraft 发行包与复用评估

[English](redcraft-audit-2026-10-15.md)

**核查日期：** 2026-10-15。**核查对象：** Keel62155/Minecraft-X-Games，`3.0.0-RedCraft`。**Blockwater 基线：** `bdd2e93bcdbb25e5bbffcca3b33e8e5e6c2a9ce6`。

## 结论

RedCraft 已有 Minecraft/RDR2 实现、可下载插件、附带源码、DX12 渲染及宿主交互代码。相同游戏组合不能再作为 Blockwater 的首创依据。但本次检查的 RedCraft **不是可以立即复用的开源基线**：包内明确说明自有源码尚未选择许可证，复用前须取得许可。

保留 Blockwater 已有基线，不导入 RedCraft 实现，也不因为已经投入开发就继续推进整套 RDR2 路线。剩余最值得验证的玩法假设是：Minecraft 建造能够改变 **RDR2 原生人物、马匹和马车的碰撞**，而不只是约束 Minecraft 侧移动。这项能力在 RedCraft 中尚未确认，在 Blockwater 中尚未实现，不能当作已经成立的差异化优势。

## 核查对象与完整性

[发行版本](https://github.com/Keel62155/Minecraft-X-Games/releases/tag/3.0.0-RedCraft)发布于 **2026-10-04 19:15:15 UTC**，即北京时间 **2026-10-05 03:15:15**。附件 `RedCraft.zip` 为 **149,274 字节**，实际下载内容与发行附件登记的 SHA-256 一致：

```text
f003eb89654b7d3ac09c6fd211cf9902c46c7c68d4f18156bcfb9b63038901d1
```

外层压缩包包含四个文件：安装说明、`RedCraft.asi`、`RedCraft.fx` 着色器及 `RedCraft-source.zip`。内层源码包包含 19 个文件，涉及 C++ 源码与头文件、CMake 配置、着色器、文档及第三方许可说明。本包未附 Java/Fabric 源码和外部 SDK。

| 对象 | SHA-256 |
| --- | --- |
| `RedCraft/RedCraft.asi` | `7c0fe25c7e7190c1b628769f5d71fc313865c78803fe095f20285d63bd3b161d` |
| `RedCraft/RedCraft-source.zip` | `6eadf1a436188f9e4c1243419108830575edd6871b7690ecd1d2f87b17d68269` |
| 内层 `RedCraft/THIRD-PARTY-NOTICES.md` | `c1d135aa172a8c5c56e5df7761718ff7c0397adba5dc2b58aeddf8890163fc1f` |

[对应标签的 Git 文件树](https://github.com/Keel62155/Minecraft-X-Games/tree/3.0.0-RedCraft)只有 README 与 Git 属性文件。因此，GitHub 自动生成的源码下载不是本次检查的实现源码包。下载哈希一致仅确认附件身份，不证明附带二进制由该源码可复现构建。

## 许可证范围

内层 `THIRD-PARTY-NOTICES.md` 第 16—17 行明确说明：RedCraft 自有源码尚未选择许可证，允许阅读，复用前须询问作者。这不是仅凭缺少 LICENSE 文件作出的推断。

同一说明将 SkyCraft 的协议/Fabric 依赖列为 MIT，将 ReShade 列为 BSD-3-Clause；Script Hook RDR2 及其 SDK 须另行取得。这些依赖的许可不等于 RedCraft 自有适配器的许可。说明还将深度合成思路归因于 universal-modder，同时明确没有包含该项目代码。

**复用判断：** 在取得适用授权前，不将本次检查的 RedCraft 实现复制、翻译或改编后并入采用 MIT 的 Blockwater。需要复用明确许可的上游组件时，应直接核对该组件自己的源码与版本。源码可阅读或 GitHub 提供 Fork 操作，不构成任意修改并再分发的授权；参见 [GitHub 许可说明](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/licensing-a-repository)。本次未向作者发送授权请求，也没有取得额外授权。

## 源码结构能够确认什么

以下证据来自压缩包清单及去除注释、字符串后的函数与调用索引，结合函数名称、Native 哈希和调用路径判断。检查属于静态结构分析，不等于完整控制流审计、构建成功或游戏内测试。

| 项目 | 实际源码证据 | 可以成立的判断 |
| --- | --- | --- |
| Minecraft 接入 | `src/Link.cpp` 创建和映射共享内存，包含心跳、状态、渲染、角色及事件通道；包内 README 要求外部 SkyCraft Minecraft 实例。 | 使用既有 Minecraft 运行时与桥接，不是在本包内提供全新 Fabric 实现。 |
| 渲染 | `src/Renderer.cpp:771–912` 构建区块网格并消费渲染消息；`922–1444` 处理几何、叠加图像及 DX12 绘制；`src/ReShadeBridge.cpp:48–107` 向 ReShade 绑定合成资源。 | 混合几何与图像管线：宿主绘制区块网格，再结合图像/叠加层与深度合成。不是直接照搬 GTA 全帧像素桥接。 |
| 宿主地形进入 MC | `src/Collision.cpp:409–688` 构建区域并经 `WriteCollision` 发送；`src/Link.cpp:121–148` 写入碰撞通道。 | 确有 RDR2 世界采样及 Minecraft 侧碰撞表示；这不等于玩家放置的方块会阻挡 RDR2 原生实体。 |
| 战斗与火焰 | `src/RedCraft.cpp:521–537` 实现 `DamageNpc`；`538–1127` 的 `Tick` 调用事件读取、生命值更新、原生爆炸、点火和角色发布。 | 不是只有视觉换皮，确有宿主交互实现路径；伤害正确性、归因和重复事件处理未做运行验证。 |
| 控制权切换 | `476–508` 的 `SetDriving` 管理相机和人物状态；`Tick` 检查坐骑、载具、暂停及玩家控制权。 | 有控制权交接实现，不代表所有任务和过场均已兼容。 |
| 方块的原生碰撞体 | 扫描到的调用结构未出现常规 `CREATE_OBJECT`、模型请求、方块实体代理路径或已知 RDR2 对象创建哈希；`Collision.cpp` 将采样区域送往 MC。 | “方块挡住原生 NPC、马匹和马车”仍未确认。扫描未发现不等于排除了所有可能的实现方式。 |
| 测试与发行 | 内层源码包没有测试目录，CMake 未注册 CTest；SDK 与 ReShade 头文件外置。 | 本源码包未交付自动回归测试套件，不代表作者从未测试或外部依赖没有测试。 |

包内文档要求 DX12，并通过支持 Add-on 的 ReShade 实现宿主场景遮挡。源码 README 列出 RDR2 `1.0.1491.50`、Script Hook `1.0.1491.17`；依赖说明列出 ReShade 头文件 `6.8.0`。这些是文档条件，不是本次实测兼容矩阵。Vulkan、当前游戏版本、长时间性能与存档重启行为均未验收。

## 与 Blockwater 的实际对照

Blockwater 状态依据[固定版本 README](https://github.com/ranzevor/Blockwater/blob/bdd2e93bcdbb25e5bbffcca3b33e8e5e6c2a9ce6/README.zh-CN.md)，不以计划中的里程碑冒充已实现能力。

| 能力 | 本次核查的 RedCraft | Blockwater 当前基线 |
| --- | --- | --- |
| 可下载游戏适配器 | 已发行 `.asi` 与着色器；本次未运行 | 原生适配器已有 CI 构建；没有可玩发行版 |
| Minecraft 客体接入 | 依赖 SkyCraft，存在相应宿主协议实现 | 尚未接入 Fabric 客体 |
| 实时游戏画面融合 | 有 DX12 几何/图像渲染器及 ReShade 桥接 | 只有 CPU 参考合成器，未接入游戏 GPU 渲染 |
| 宿主世界采集 | 有地形/碰撞区域、玩家及角色代码 | 相机、玩家及有界异步地形采集；工程测试通过，未实测游戏 |
| 爆炸与 NPC 伤害 | 有事件及 Native 调用路径 | 仅核心/模拟契约；原生请求仍拒绝执行 |
| 方块阻挡原生马车/NPC | 本次检查不足以确认 | 尚未实现 |
| 自有适配器复用许可 | 明确尚未选择许可证 | 项目采用 MIT |
| 工程验证依据 | 本源码包未附自动测试套件 | 仓库有 Linux/Windows CI、生命周期与采集回归 |

文档、许可证和测试体系对贡献者有价值，但不能单独抵消缺少可玩功能的差距；同样，发行包存在也不证明性能稳定或双向物理完整。

## 同名项目分开处理

[Melty 上的 RedCraft 条目](https://melty.gg/m/redcraft-2)署名 Tate Blackmore / QQave，宣称支持建造、遮挡、武器伤害、爆炸及控制权交还。尚未确认该条目与 Keel62155 的 `3.0.0-RedCraft` 存在何种源码或版本继承关系，不能把它的功能宣传直接并入本次 GitHub 附件结论。本次未检查该条目的下载代码、许可证和游戏表现。

## 路线判断与下一项必要证据

| 选项 | 判断 |
| --- | --- |
| 用现有 RedCraft 体验跨游戏玩法 | 对体验目标有价值，但不等于拥有自己的开源项目；安装和运行仍需核验。 |
| 立即 Fork RedCraft 并补强 | 被本包自有代码许可条件阻断，代码复用前须取得适用授权。 |
| 独立继续 Blockwater | 需要明确的玩法或研究目标，不能只重复游戏组合；采用 MIT 本身不足以证明值得继续。 |
| 更换宿主游戏 | 当新鲜感优先或 RDR2 差异无法成立时合理；本次尚未选定或实施其他游戏。 |

在把原生建造物理作为恢复 RDR2 专用开发的依据前，应固定同一个 RedCraft 发行包与测试场景：放置一面墙，分别测试 Minecraft 控制的玩家、RDR2 控制的玩家、NPC、马匹/马车，再拆墙重复。区分真实碰撞响应与视觉遮挡，并检查控制权切换和重启后的行为。这才能判断 MC 侧移动与宿主原生物理的差别。

建议保留基线，不立即恢复下一批大规模实现；先证明上述差异，或明确选择其他研究目标。不能通过缩减完整游戏融合目标来掩盖核心能力尚未交付。

## 核查记录

[发行包清单检查](https://github.com/ranzevor/Blockwater/actions/runs/38665955919)与[许可证/调用结构检查](https://github.com/ranzevor/Blockwater/actions/runs/38666508382)均于 2026-10-15 成功完成。后者使用审查代码 `15c45d3188c39edf24b7fa719915e259cf1017b8`。脚本校验 SHA-256、限制压缩包尺寸和嵌套层级，只检查文本，不运行包内代码。发行包、二进制和实现源码均未加入 Blockwater。

上述运行只证明下载对象一致且检查完成。RedCraft 的编译、源码与二进制对应、真实游戏运行、视觉效果、稳定性及物理行为仍未验证。结论与检查工具保留于 `research/redcraft-release-audit`，与应用主线基线分离。
