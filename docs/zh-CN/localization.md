# 国际化规范

[English](../localization.md)

- `locales/en-US.lang` 是必需的英文回退语言资源。
- `locales/zh-CN.lang` 为简体中文资源，消息键应与英文完全一致。
- 文件采用 UTF-8，每条有效记录使用 `stable.message.key = 展示给用户的文字`。空行以及以 `#` 开头的行为注释；禁止重复键、空值或错误格式。
- `Catalog(directory, requested_locale)` 只在启动时载入。`zh`、`zh-CN` 选择简体中文，其余当前回退到 `en-US`；缺失译文也回退到英文。游戏每帧执行过程中不读取文件。
- 跨进程协议必须使用稳定的事件类型、数字和枚举，不能把翻译后的文本作为机器协议字段。

修改翻译时同步修改两份资源，并运行 `python3 tools/validate_repository.py` 及 C++ 测试。校验会检查 UTF-8 编码、键名和各语言的消息集合。语言资源读取功能已实现，但尚未接入游戏中的语言选择界面。
