# RDR2 原生宿主

[English](../rdr2-host.md)

## 当前实现

`Blockwater.asi` 是 B1 的 Windows x64/MSVC 生命周期组件：向 ScriptHookRDR2 注册脚本，在脚本线程检查游戏状态，并连接既有 ABI v1 桥接层。相机、地形、碰撞对象与游戏效果属于后续 B1 工作。本组件不发布有效世界帧；对当前会话的玩法请求明确返回拒绝，不以模拟结果代替游戏执行。

## 构建与测试

在装有 C++ 桌面工具链的 Windows 环境，从仓库根目录执行：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
ctest --test-dir build -C Release -R "^rdr2_" --output-on-failure
cmake --install build --config Release --prefix build/package
```

插件位于 `build/rdr2/plugin/Release/Blockwater.asi`。Windows 默认启用 `BLOCKWATER_BUILD_RDR2`；使用 `-DBLOCKWATER_BUILD_RDR2=OFF` 仅构建可移植核心。原生插件要求 MSVC x64 ABI。CMake 根据 `rdr2/scripthook.def` 中的六个接口符号生成导入库，不下载或提交专有 SDK。

安装暂存目录只包含 `Blockwater.asi` 和两份许可文件。测试用 `ScriptHookRDR2.dll` 位于 `build/rdr2/harness/Release/`，不是真实游戏运行库，不得复制进游戏目录；`rdr2_install_manifest` 检查暂存文件集合。游戏执行需另行取得 [ScriptHookRDR2 运行库和 ASI 加载器](https://www.dev-c.com/rdr2/scripthookrdr2/)。加载器测试不能确定支持哪些游戏版本。

## 生命周期

| 状态或变化 | 宿主行为 |
| --- | --- |
| DLL 加载 | 仅注册脚本入口，不调用游戏 Native、不创建共享内存 |
| 首次进入脚本 | 取得不等待的独占宿主存在句柄，打开有效残留映射或新建映射，并使旧世界帧失效 |
| 主菜单、玩家不可用、玩家死亡或 Native 未返回结果 | 保持连接关闭 |
| 非网络环境下，存活玩家具有控制权 | 发布非零会话代次与心跳，等待对端心跳实际推进 |
| 暂停或失去玩家控制权 | 关闭对端记录，使世界数据失效，清除本地待发送响应 |
| 客体关闭、超时或切换代次 | 使旧会话失效，增加宿主代次，并重新确认心跳 |
| 任一网络检查为真 | 返回多人模式拒绝，在插件卸载前不恢复连接 |
| 显式卸载 DLL | 先注销脚本，再关闭通信、释放映射和宿主存在句柄；不调用游戏 Native |
| 进程退出 | 由操作系统回收进程资源，不调用正在退出的其他 DLL 完成清理 |

共享内存名称为 `Local\Blockwater_rdr2`，宿主存在句柄名称为 `Local\Blockwater_rdr2_host`。第二个插件实例不能覆盖已有实例。存在句柄只创建并持有，不执行等待。客体仍持有映射时允许插件重新打开，但不重新初始化协议不符的现有映射。重载后的代次必须大于原宿主代次，数值耗尽时停止启动而不回绕。

`scriptWait(0)` 将 ScriptHook 协程让出至下一帧。逐帧路径没有 IPC 等待、工作线程、磁盘操作或资源创建。共享内存清理不调用游戏函数。本批不持有游戏相机、玩家可见性状态或游戏对象；后续玩法适配器须在卸载前通过脚本线程清理其游戏资源。

## 通信行为

通信就绪不代表玩法就绪。客体必须同时验证有效且属于当前会话的 `HostFrame`；真实世界采集接入前，本组件维持无效的零标记帧。

每帧最多取出 32 条原始消息，旧消息也计入预算。非法数据、发送或接收代次不符的消息，以及仅用于响应的事件类型会被丢弃。当前代次中的放置、移除、爆炸与伤害请求返回 `Rejected`，保留请求标识、原类型和方块坐标。重复的不支持请求不会产生游戏副作用，可再次得到相同拒绝。

反向队列满时保留一条拒绝响应，并停止继续消耗请求；代次切换会丢弃这一旧响应。旧事件不能在新会话中转化为动作，不重置任一共享队列的游标，ABI 布局与版本保持不变。

## 接口来源与验证

九个只读 Native 的名称、哈希和签名见[接口登记表](../rdr2-host.md#native-interface-register)，已与固定版本 Garry's Redemption 的登记核对。三个网络检查分别执行：任一个为真即拒绝；没有真值但存在缺失结果时，保持不可用。IPC 不接受任意 Native 哈希或控制台命令。六个 ScriptHook 声明保留对应的 MSVC 导入符号，并保留[第三方来源](../references.md)及[原始 MIT 许可](../../LICENSES/Garrys-Redemption.txt)。

可移植测试覆盖生命周期、消息预算、反向队列满载、线程归属、代次耗尽和帧失效。Windows 测试加载实际构建的 `.asi`，使用隔离的测试运行库核对 Native 签名和脚本协程归属。未知 Native、错误线程和非零逐帧等待均使测试失败。覆盖三个独立网络标记、部分 Native 失败、暂停、玩家不可用/死亡/无控制权、断连、客体重启、残留映射重载、损坏映射、重复插件和首次脚本执行前卸载；另有运行库缺失与安装文件集合检查。

各提交的实际执行结果见 [CI](https://github.com/ranzevor/Blockwater/actions/workflows/ci.yml)。真实 ScriptHook 加载、游戏状态与故事模式内重载仍需[固定版本游戏验收](../milestones.md#required-game-validation)。B1 尚需完成相机/地形采集以及真实碰撞对象的创建和移除。
