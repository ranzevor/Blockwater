# 消逝的光芒 1 启动核查 — 2026-10-15

[English](dying-light-1-startup-audit-2026-10-15.md)

## 实施判断

**正式宿主迁移的启动条件尚未满足。** 消光 1 仍是拟采用目标；现有证据没有证明完整融合不可行，但参考工程尚不能提供所需的单人模式门禁、安全修改阶段、原生方块碰撞生命周期和经过验证的双向伤害契约。下列实验均是未完成事项，不是已通过前提。

本次变更仅记录研究。Blockwater 的既有 RDR2 实现、当前宿主契约、ABI、构建和测试保持不变。本次没有交付消光适配器、Minecraft 客体、安装器或可玩发行版，也不继续扩充 RDR2 玩法或改选其他目标。

## 固定来源与发行身份

| 来源 | 实际读取版本 | 核查结果 |
|---|---|---|
| Blockwater main | `bdd2e93bcdbb25e5bbffcca3b33e8e5e6c2a9ce6` | 与接手检查点一致；首次读取时没有开放 PR |
| 原研究分支 | `6bbc1a23d8d674b0a2df7fd0826835d0df3d09f1` | 领先主线十个提交；八项研究文档、工作流及工具新增，没有应用源码变化 |
| GModLight main | `e53209d25c295550e40b8fa977cc36147567b7de` | 下文上游代码结论统一依据此版本 |
| GModLight v0.1.0 标签 | `e3ad71bc2e38141321a9804192d7864ec6c96bde` | 不同于审查的 main；main 领先两个提交，涉及十四个文件 |

作者的 [v0.1.0 发行页][release] 发布时间为 2026-10-04 20:12:15 UTC。标签至 main 的变化包括运行时代码、配置、桥接协议及客体代码，并非只有文档。ZIP 的发行元数据不能证明本次审查的 main 已经发行，更不能证明 Blockwater 已经通过测试。本轮未执行发行包，也未将其二进制与重新构建结果比较。[版本比较][release-diff]

引擎研究针对 Steam `DyingLightGame.exe` 1.55.0.0、build 24076292。此信息只标识上游研究目标，不代表已核实当前零售版本，也不构成 Blockwater 的兼容承诺。[引擎研究][history]

[已有目标评估][previous] 保留在原研究版本。本轮不重跑 RedCraft 发行附件与静态结构扫描，不引入原研究分支中的临时工作流及审查工具。

## 同类项目补查

检索日期：**2026-10-15**。已查看结果没有建立消光 1 × Minecraft 直接等价双运行时项目的证据。覆盖不完整，不能据此声称唯一或首个。

| 检索面与实际检索词 | 实际结果 | 未覆盖部分 |
|---|---|---|
| GitHub 仓库检索：`"dying light" minecraft`；`DyingLight Minecraft` | 两次调用均返回 `ReadTimeout` | 仓库检索覆盖仍未解决；超时不等于零结果 |
| Web：`"Dying Light" "Minecraft" "passthrough"`；`"Dying Light" "Minecraft" "fusion"`；`Dying Light Minecraft mod mashup 2026` | 出现 GModLight 和 Minecraft 主题项目，已查看结果未建立所需的双运行时组合 | 仅覆盖公开索引结果 |
| 名称核查：`"MineLight" "Dying Light"`；`"LightCraft" "Dying Light"` | 未建立相应作者、代码及发行身份 | 不能仅凭相似名称认定项目对应 |
| 视频与社区：`site:youtube.com "Dying Light" "Minecraft" "mod"`；`site:youtube.com "Dying Light" "Minecraft inside"`；`site:reddit.com/r/dyinglight "Minecraft" "mod"`；`GModLight GooseMcGee video` | 未独立验证到消光 1 × Minecraft 的视频—源码—发行对应链 | 检索元数据不等于观看视频或实机测试；未覆盖私有社区 |
| 中文：`消逝的光芒 我的世界 双游戏 融合`；`"消逝的光芒" "我的世界" "融合"` | 找到创作者发布的 Minecraft 整合包视频，未建立消光原生宿主接入证据 | 仅查看检索摘要；页面直接读取被 robots.txt 阻止，未检查视频画面 |

**身份区分。** GooseMcGee 的 GModLight 仓库与作者发行身份一致，但客体为 Garry's Mod。[Last One Alive][modpack] 创作者页面描述的是受消光启发的 Forge Minecraft 整合包；小小伊布酱于 2023-01-13 发布的[光芒消逝之日视频][video]也描述 Minecraft 整合包。这些描述没有提供两个原版游戏同时运行的证据。主题地图、武器、怪物和消光里的 Minecraft 彩蛋均不作为等价融合证据。[GModLight README][up-readme]

目前已识别的重合不足以否定本项目价值。后续若发现直接等价项目，应比较真实交互、交付和维护能力，而不是自动否决目标。

## 宿主接口证据表

以下均针对上述固定 GModLight main。上游实机观察属于作者记录；**Blockwater 尚未对表中任何接口执行消光 1 实机测试**。导出符号与源码实现提供候选接口证据，不证明其已在当前可用零售程序中成功调用。

| 接口 | 源码位置与静态事实 | 上游记录及仍需解决的条件 |
|---|---|---|
| 加载与版本身份 | [初始化][init]、[入口][detach]、[CMake][cmake]：ASI DLL、引擎修饰导出名及固定游戏 DLL 偏移 | 文档给出研究版本，但所查初始化路径没有在挂钩和修改前校验完整的受支持二进制指纹 |
| 帧阶段与线程 | [消息泵][tick]、[世界调用分发][world] | 当前使用 PeekMessageA/W、窗口线程筛选和 3 毫秒节流；最近 tick 超过 250 毫秒后回退到 Present 修改世界。节流和线程编号相同均不能证明引擎安全阶段 |
| 相机与投影 | [相机实现][camera] | 读取相机向量、view、combined、projection，按合理性检查回退并从投影推导视场角；上游记录米制、Y-up 及前向轴修正。Blockwater 尚未验证帧深度配对、抖动及窗口变化 |
| 世界查询 | [TraceProbes][events] | 对有界客体线段执行宿主射线查询；可作为查询线索，不能替代原生障碍物创建或完整世界几何接口 |
| 对象／碰撞体生命周期 | [模型创建删除][models]、[SyncVisuals][world] | 用已有宿主 mesh 创建 CModelObject；该路径没有任意 MC 方块碰撞几何构造和注册。创建失败可能丢失已分配对象，删除失败没有向调用方反馈结果 |
| 伤害与死亡 | [TakeDamage][damage]、[ApplyEvents][events]、[协议][abi] | 固定偏移构造描述符并调用虚表槽；槽位与导出不匹配只记录日志，仍继续调用。普通伤害、首次命中、归因与反向结算尚未验证 |
| 单人模式 | [计划 1.2 节][plan]、[初始化][init]、[世界分发][world] | 上游明确说明合作模式只有私密可见性操作提示；SessionCooperative／SessionCooperativeDI 是研究线索，不是已实现分类器 |
| 退出与恢复 | [关卡变化处理][world]、[入口][detach]、[模型删除][models] | 依赖关卡指针变化清空状态；DllMain 处理 attach，没有 detach 恢复分支。空关卡传播、地址复用、资源回滚及受控卸载仍需实验 |

导出表另有 `IsLoading`、`IsLevelShutdown`、对象销毁状态查询及原生帧处理器相关控制入口。这些线索缩小了下一步核查范围，但尚未给出可调用的回调 ABI、安全阶段或单人模式状态契约。`IsNetPlayerPlaying`、在线服务初始化、玩家数为一、私密可见性均不能替代经验证的会话模式判定。[生命周期导出][exports-life] [帧处理器导出][exports-frame]

## 三个核心关口的新增证据

### 渲染

实际着色器只声明客体帧纹理，alpha 距离用于重投影；其深度模板状态关闭深度测试。这直接说明当前合成路径没有将客体深度与宿主场景深度比较。[合成代码][compositor]

PLAN 中的场景深度捕获和猜测的深度编码仍是未实现假设。原生模型替代路径绘制的是已有宿主 mesh，不是任意 Minecraft 几何。两条路线均未提供经过测量的消光深度资源身份、编码、相机与帧配对、透明层及 HUD 顺序结果。[计划][plan] [模型路径][models]

D3D11 支持满足资源创建及绑定条件的深度资源和兼容类型化视图。因此可以设计具体图形实验，但不能把在消光中找到任意 DSV 当作正确场景深度。必须先读取真实资源格式、视图、采样数及所属渲染阶段，再确定复制或着色器访问方式。[Microsoft 深度文档][d3d-depth] [资源视图契约][d3d-view]

### 碰撞

`CreateModelRaw` 分配游戏对象后，类型转换失败或后续异常会直接返回空值，没有撤销已创建对象。`DeleteModel` 不向 `SyncVisuals` 返回删除结果，后者仍移除其登记项；多次创建失败后的提前返回也会跳过正常删除扫描。这些是参考路径的静态所有权和回滚缺口，不是已观察到的 Blockwater 崩溃。[模型路径][models] [SyncVisuals][world]

创建 mesh、射线命中、移动已有角色或猜测碰撞标志均不能证明 MC 放置事件增加了宿主原生障碍物。需要串联几何构造、绑定、过滤、激活、停用和回收；玩家、原生敌人与投射物分别验收，导航结果单独记录。

### 伤害

当前伤害代码仍使用游戏 DLL 虚表偏移和描述符常量。虚表槽位比较发现不匹配只写日志，调用仍继续；返回成功仅表示调用返回，不代表生命值按预期减少。[TakeDamage][damage]

桥接协议只有客体到宿主的伤害和死亡事件，没有反向伤害结算事件契约。事件记录没有会话代次、事件身份或逐事件攻击者身份。调用方选择宿主玩家作为攻击者，死亡事件另行调用 TakeDamageToKill 或 Kill。这不足以建立 Blockwater 所需的状态权威、跨重连一次生效、攻击者归因和双向 TNT 联动。[协议][abi] [ApplyEvents][events]

历史首次命中异常需要通过普通非致死伤害重现并修复。强制死亡属于另一项操作，不能计作修复。[引擎研究第 5—7 轮][history] [计划][plan]

## 启动及核心链路的实验条件

以下是实验规格，**均未实现或通过**。探测原型不要求完整玩法已经成立，但修改宿主之前必须验证版本、模式和执行阶段门禁；门禁尚未成立时，隔离探测只能执行不改写世界的观察。

| 编号 | 实验 | 验收证据 |
|---|---|---|
| DL1-S0 | 确定 EXE、引擎 DLL、游戏 DLL 指纹；不改写世界地观察会话状态及引擎回调节奏 | 精确程序身份、单人模式正向证据；未知、加载、关闭及禁止模式拒绝全部修改；不接受配置强开、玩家数捷径、定时节流冒充帧入口或 Present 修改回退 |
| DL1-G0 | 找到同一渲染阶段的场景深度与相机；或追踪原生几何提交 | 实际资源／视图／采样描述和帧配对；实测近远端及深度编码；前后景对照能区分遮挡正确与仅对齐；覆盖窗口变化、抖动、透明层和 HUD 顺序 |
| DL1-P0 | 创建、注册、移除一个自有方块碰撞体，并覆盖重复及关卡／会话变化 | 存在时分别阻挡原生玩家、敌人和投射物，移除后不再阻挡；各分配和注册阶段失败均回滚；资源登记回到基线，导航单独报告 |
| DL1-D0 | 先普通非致死伤害，再反向伤害及有界爆炸 | 权威生命值前后对照、受害者与攻击者身份、事件及代次；每事件每目标只生效一次，无反馈循环；首击不依赖 Kill；正确处理重复、迟到、重连和死亡目标 |
| DL1-R0 | 客体停止／重连、离开／重新进入关卡及受控退出 | 清理前停止修改；旧队列及身份不能影响新会话；通过已验证的生命周期路径恢复或释放输入、HUD、自有对象、钩子及资源 |

S0 必须验证状态转换，而不只验证首次进入。真实禁止模式观察需要明确获准的隔离测试条件；本轮不发起联机玩法、邀请或入侵。协议负例测试不能单独证明游戏状态分类器正确。

G0 不能仅按 DSV 最大尺寸认定世界深度；P0 不能用客体碰撞或已有原生道具代替任意方块碰撞；D0 必须先测普通伤害，不能用死亡捷径替代；R0 必须跟踪会话／关卡代次，不能仅比较原始指针。逐帧修改回调不得执行阻塞 IPC、等待、文件 I/O 或同步 GPU 回读。

S0 获得经验证的路径、其余可行性风险有具体实验支持后，再从独立实施分支接入真正的消光适配器，同步修改项目特有宿主、构建与文档契约，保留现有 ABI 权威和有效回归。随后实现真实 Fabric 客体，在同一场景完成 G0／P0／D0，再扩展外围玩法。Minecraft／Fabric 版本仍需客体源码与依赖验证后选择；Garry's Mod 不作为最终运行依赖。

## 可用验证条件与实际结果

本轮检查的执行环境为 Linux x86_64，具备 CMake、Ninja、C++ 编译器和 Java；工具清单中未发现 Windows 运行时、Wine、Steam、MinGW 交叉编译器或暴露的 GPU 设备；检查的工作根目录中没有 Blockwater 工作副本或消光程序。此检查不描述当前可访问环境之外的机器或账号。

Blockwater 既有 [CI][blockwater-ci] 提供 Ubuntu 24.04 与 Windows 2025 编译和测试。已并入所查 main 的 PR #3 源码头 `23c52e6dbe16c5c2ca1202ae06ee2e87e4662167`，其[既有运行][base-ci]两平台均成功。这是复用的工程证据，不是本轮重跑结果或消光兼容结果。合并 SHA 未返回 push 工作流记录；该工作流按 PR／手动触发。

GModLight 的 CMake fakehost 目标及[源码][fakehost]用模拟宿主驱动真实 Garry's Mod；其 [CI][up-ci]在 Windows 构建打包，发行流程没有提供消光玩法验收。这两者都不能替代缺失的真实宿主运行条件。

**尚缺的运行前提：**可访问且获授权的 Windows x64 图形会话、合法可用的消光 1 副本、精确 EXE／DLL 身份，以及隔离探测执行和一次性测试数据的使用许可。后续真实客体验证还需要合法 Minecraft 运行条件。

**本次结果：**静态源码、发行、检索、环境证据和明确的启动判断。消光进程集成、GPU 合成、原生碰撞、普通伤害、模式拒绝与退出恢复均未实测，不能用单元测试、Windows 编译或上游观察填充为通过。

## 复用范围

GModLight 自有 [LICENSE][license] 为 MIT，版权归 GModLight contributors；[第三方清单][notices]另列 MinHook、Ultimate ASI Loader 和 gmod-module-base。后者是 GMod 专用依赖，不是计划中的 Minecraft 依赖。本次没有复制上游实现或二进制；后续实际采用的组件需分别固定来源并保留适用许可。不引入游戏资产、专有 SDK、存档或测试替身。

[up-readme]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/README.md
[plan]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/docs/PLAN.md
[history]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/docs/ENGINE_ANALYSIS.md
[init]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/dl_plugin/src/engine.cpp#L1-L145
[camera]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/dl_plugin/src/engine.cpp#L260-L360
[tick]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/dl_plugin/src/main.cpp#L572-L616
[world]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/dl_plugin/src/main.cpp#L1250-L1499
[detach]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/dl_plugin/src/main.cpp#L1499-L1518
[compositor]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/dl_plugin/src/overlay.cpp#L80-L210
[models]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/dl_plugin/src/engine.cpp#L1371-L1420
[damage]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/dl_plugin/src/engine.cpp#L590-L680
[events]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/dl_plugin/src/main.cpp#L906-L1035
[abi]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/shared/bridge.h
[exports-frame]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/docs/re-data/engine_x64_rwdi.dll.exports.txt#L2153-L2225
[exports-life]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/docs/re-data/engine_x64_rwdi.dll.exports.txt#L3932-L4077
[fakehost]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/tools/fakehost.cpp#L1-L120
[cmake]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/CMakeLists.txt
[up-ci]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/.github/workflows/build.yml
[license]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/LICENSE
[notices]: https://github.com/GooseMcGee/DyingLight-GMod/blob/e53209d25c295550e40b8fa977cc36147567b7de/THIRD_PARTY_NOTICES.md
[release]: https://github.com/GooseMcGee/DyingLight-GMod/releases/tag/v0.1.0
[release-diff]: https://github.com/GooseMcGee/DyingLight-GMod/compare/e3ad71bc2e38141321a9804192d7864ec6c96bde...e53209d25c295550e40b8fa977cc36147567b7de
[previous]: https://github.com/ranzevor/Blockwater/blob/6bbc1a23d8d674b0a2df7fd0826835d0df3d09f1/docs/research/host-target-review-2026-10-15.zh-CN.md
[base-ci]: https://github.com/ranzevor/Blockwater/actions/runs/38657624738
[blockwater-ci]: https://github.com/ranzevor/Blockwater/blob/bdd2e93bcdbb25e5bbffcca3b33e8e5e6c2a9ce6/.github/workflows/ci.yml
[modpack]: https://modrinth.com/modpack/last-one-alive
[video]: https://www.bilibili.com/video/BV1bM411F7o4/
[d3d-depth]: https://learn.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-depth-stencil
[d3d-view]: https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_shader_resource_view_desc
