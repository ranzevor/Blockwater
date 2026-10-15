# 兼容目标评估 — 2026-10-15

[English](host-target-review-2026-10-15.md)

## 当前决定与评估范围

项目负责人倾向于更换 RDR2 兼容目标。本评估比较替代方案，不代表已经选定新游戏或修改现行应用契约。本轮不增加 RDR2 玩法实现，原有源码与测试保留于 main `bdd2e93bcdbb25e5bbffcca3b33e8e5e6c2a9ce6`。

更换目标不需要先证明 RedCraft 不够好。原始目标是有趣、可玩的开源跨游戏融合，具有场景遮挡和有意义的双向交互。原生碰撞属于这个目标，不是必须继续 RDR2 的理由。

## RedCraft 碰撞链路复核

[第三次检查](https://github.com/ranzevor/Blockwater/actions/runs/38668864600)已通过，审查代码为 `afc1b4df422ab0cdcdf74d91ad66c306ec4fc96a`。核查对象仍是[3.0.0 发行附件](https://github.com/Keel62155/Minecraft-X-Games/releases/tag/3.0.0-RedCraft)，SHA-256 为 `f003eb89654b7d3ac09c6fd211cf9902c46c7c68d4f18156bcfb9b63038901d1`。

词法索引覆盖内层全部 14 个 C++、头文件与着色器文件，发现 46 处 `invoke<T>` 绑定，全部使用字面量哈希，该调用形式没有动态哈希参数；检查器自测通过。这是静态检查，不是编译器生成的完整调用图，也不是游戏实测。

| 路径 | 实际结果 |
| --- | --- |
| `Collision.cpp:409–688` | `BuildAndSend` 构建区域；`SendRegion` 在第 620、687 行调用 `WriteCollision`，包含 `kColRegion`。 |
| `Link.cpp:121–148` | `WriteCollision` 将碰撞数据发布到共享内存；结合此前协议核查，方向为宿主世界到 Minecraft。 |
| `Renderer.cpp:691–770,1312` | `kWeBlock` 由 `BuildWorldEntities` 引用，`ReadWorldEntities` 由 `RenderFrame` 消费；这些路径生成和绘制几何，不创建宿主原生障碍物。 |
| `RedCraft.cpp:1015,1019,1079` | 存在爆炸、命中角色及 `CopyHazards` 路径，不能因此推导出刚体碰撞注册。 |
| 原生方块障碍物 | 索引中没有发现放置方块对应的常规对象创建、模型加载和碰撞启用链路；此前 Native 哈希清单也没有已知的对象创建哈希。 |

**能够成立的结论：** 本次源码证据没有展示“MC 方块进入 RDR2 原生碰撞世界”的实现；能够识别的碰撞导出方向相反。**仍未查实：** 发行二进制在真实游戏中是否能阻挡 NPC、马匹或马车。本轮没有运行游戏，也没有验证源码与二进制对应。自有代码复用许可限制见[此前评估](redcraft-audit-2026-10-15.zh-CN.md)。

## 候选 A：消逝的光芒 1

指第一代，不是《消逝的光芒 2》或《困兽》。已有 [GModLight](https://github.com/GooseMcGee/DyingLight-GMod) 将它与 Garry's Mod 桥接；本轮直接读取其 [MIT 许可证](https://github.com/GooseMcGee/DyingLight-GMod/blob/main/LICENSE)。README 说明相机、输入、原生僵尸代理、伤害调用、重投影及模拟宿主测试入口。这个参考比单纯的物品生成器覆盖更多接入环节，但它不是 Minecraft 适配器。

直接读取[渲染源码](https://github.com/GooseMcGee/DyingLight-GMod/blob/main/dl_plugin/src/overlay.cpp)确认：着色器采样客体画面，利用其中编码的距离做重投影，深度测试关闭；这不是宿主场景深度遮挡。[引擎分析](https://github.com/GooseMcGee/DyingLight-GMod/blob/main/docs/ENGINE_ANALYSIS.md)也记录了未完成的物理接入，分析目标是 Steam 可执行文件 1.55.0.0。当前游戏版本兼容性尚未实测。

**玩法提案：** 在消光世界使用 MC 建造、工具和 TNT，让方块影响原生敌人，让原生攻击反馈到 MC 玩家。这是设计目标，不是已有能力。保留原生跑酷、让 AI 对新建筑重新寻路需要独立设计和验收；物理阻挡本身不会更新导航。

**首要技术关口：** 宿主深度获取、原生方块碰撞体创建/删除、原生伤害语义。相机和通信基础可以在版本及许可核对后复用。不能直接把 GMod 客体换成 MC 就当作完整实现，也不应先重写全部桥接基础设施。

## 候选 B：正当防卫 3

[Console Thingy](https://github.com/aaronkirkham/jc3-console-thingy)提供车辆、人物、动物及道具生成入口，本轮核对了它的 [MIT 许可证](https://github.com/aaronkirkham/jc3-console-thingy/blob/master/LICENSE)。直接读取 [spawn_system.h](https://github.com/aaronkirkham/jc3-console-thingy/blob/master/src/game/spawn_system.h)可见基于回调的生成请求，但通过硬编码地址调用游戏；[graphics.h](https://github.com/aaronkirkham/jc3-console-thingy/blob/master/src/graphics.h)接入 D3D11 绘制控制台，不是完整场景深度或任意几何接入。

**玩法提案：** MC 建筑与车辆、爆炸产生交互，更贴近 GTA 示例中的车辆玩法。但其已核实的接入覆盖弱于 GModLight；相机、地形查询、可控碰撞几何、清理、伤害及当前版本地址兼容仍需建立依据。已有实体生成器不等于解决了任意方块碰撞。

## 同类检查与排序

《巫师 3》已有直接同类 [WitchCraft](https://github.com/Henvag/WitchCraft)。README 描述基于 Minecraft/SkyCraft 的移动、背包和方块放置原型，也列出未完成的战斗和完整场景遮挡。不能作为尚无同类的替代目标。

本轮使用 Minecraft 与 Dying Light、Just Cause 3 的精确组合，结合 passthrough、bridge 等关键词检索，也检查了 MineLight、LightCraft 名称，尚未识别到直接等价实现。只在 MC 内添加主题内容的 Mod 不能视为双运行时融合。GitHub 仓库搜索发生超时，因此仓库覆盖不完整；两个候选均不能宣称首创。

**建议排序：** 优先将消逝的光芒 1 作为下一项小范围技术可行性研究对象，因为已检查的参考项目覆盖更多必要宿主接口；正当防卫 3 保留为更偏车辆玩法的备选。两者均未获选定，也均未验证能完整达到 GTA 示例效果。选定目标并检查关键技术缺口前，不改仓库身份或重写路线图。

## 成果保留

保留共享内存和核心契约、生命周期测试、校验工具、国际化、文档与 CI 结构；RDR2 Native 绑定和宿主特有的相机、地形假设不能自动迁移。现有宿主实现独立保留；本评估没有新增其他游戏适配器或可玩发行版。

## 本轮读取的源码身份

| 文件 | Git blob SHA |
| --- | --- |
| GModLight `README.md` | `1de5ac0d1cf0239cebef03b9b894ce7c936d251a` |
| GModLight `LICENSE` | `febb14127c189b8067849e766e46b33f5d19cd9a` |
| GModLight `dl_plugin/src/overlay.cpp` | `16fdde25b5da6ad9c169b07a50b686fed1406beb` |
| GModLight `docs/ENGINE_ANALYSIS.md` | `b73f279076d9c22e76270e9306a070019f922c4a` |
| JC3 Console Thingy `src/game/spawn_system.h` | `51ad1c5f7aed96a3d6fb815bd6ac2703b39384ef` |
| JC3 Console Thingy `src/graphics.h` | `605a0d1905b3f147cb01536fb0b2adb72afa4cbd` |
| JC3 Console Thingy `LICENSE` | `af58cec86597a2b572aa8dfa23497aa2f427e709` |
| WitchCraft `README.md` | `18ef8256ed661937eb273e4c7c41d805cc5bbebc` |
