<p align="center">
  <img src="ios/Assets.xcassets/AppIcon.appiconset/Icon-180.png" width="128" alt="TH08 iOS Port icon">
</p>

# TH08 iOS Port

《东方永夜抄 ～ Imperishable Night》1.00d 的非官方 iOS 移植工程。

本仓库以社区重构源码为基础，把原本面向 Windows / DirectX 8 的游戏逻辑迁移到 arm64 iPhone 和 iPad。项目不是模拟器，也没有把 Windows 可执行文件装进 App；游戏 C++ 逻辑会直接编译为 iOS 原生程序，平台相关部分由 SDL2、OpenGL ES 兼容层和移动端输入系统承接。

> [!IMPORTANT]
> 仓库不包含 `th08.dat`、`thbgm.dat`、`th08.exe`、商业字体或其他原版游戏数据。构建者必须使用自己合法持有的《东方永夜抄》日文 1.00d 数据。

## 当前状态

| 项目 | 状态 |
| --- | --- |
| 目标平台 | arm64 iPhone / iPad |
| 最低系统 | iOS 14.0 |
| 当前验证节点 | `0.4.0 (103)` |
| 主线流程 | 模拟器自动测试已完成 Normal 六面路线、结局和返回标题页 |
| 输入 | 触摸、键盘、SDL 控制器 |
| 画面 | 横屏和竖屏；iPhone / iPad 自适应布局 |
| 安装包 | 可生成 ad-hoc 签名 IPA；仓库不发布含原作数据的 IPA |

目前版本已经可以进入标题页、选择角色并进行完整战斗。已完成的移动端功能包括：

- 跟手拖拽、虚拟摇杆、可移动和缩放的动作键；
- 菜单直接点击、滑动选项、二指取消，以及键盘 / 手柄兼容；
- 自动射击、Z/S 状态切换、仅在持有 Bomb 时触发的自动放 B；
- iPhone / iPad 横竖屏布局和竖屏战斗区重排；
- 性能选项、开发者菜单、无敌与调试功能；
- 性能菜单中的 iOS 原生作弊码输入框；输入 `ymgjdsh` 可持久化解锁全部内容；
- App 进入后台时的生命周期处理；
- 键盘 Esc、手柄 Start 和触摸菜单均进入游戏菜单，不触发 iOS 应用退出；
- 启动、渲染、关卡切换、Bomb 和崩溃诊断日志；
- Replay、Music Room、结算和跨关卡资源重建的针对性修复。

这仍是测试移植，不代表所有角色、路线、难度、符卡、Replay 和设备组合都已经无缺陷。真机日志和可重复操作步骤仍然是修复问题的重要依据。

## 技术实现

### 渲染

原作逻辑保留了 D3D8 形状的接口。iOS 端通过项目内的兼容层，把固定功能管线状态、纹理、顶点、混合、雾和 Render Target 操作转换为 OpenGL ES 2 调用，再交给 iOS GPU。当前实现不是 DXVK、MoltenVK 或 Metal 重写。

为了兼容原作的 32 位对象布局，arm64 版本还使用四字节指针句柄和外部存储表，避免 64 位指针改变 Enemy、Player、ANM、Replay 等结构体中后续字段的偏移。

### 输入和界面

SDL2 负责窗口、事件、键盘和控制器。`src/modern/ios/ios_touch.cpp` 在此之上实现触摸命中、拖拽移动、摇杆、动作键、菜单手势、自动射击、自动放 B、开发者菜单和设置持久化。横竖屏合成由 iOS 渲染兼容层根据设备方向和安全区域动态计算。

### 音频、文字和数据

- SDL2_mixer 负责 BGM 和 WAV 音效；当前不提供 MIDI 输出。
- SDL2_ttf 与构建者提供的 `msgothic.ttc` 用于日文菜单、对话和 Music Room 文字。
- 游戏继续使用原作 PBG/LZSS 数据读取逻辑，从 `th08.dat` 和 `thbgm.dat` 读取资源。
- Shift-JIS 文本在构建阶段通过 `scripts/generate_i18n.py` 和 `config/i18n.csv` 生成对应头文件。

### 稳定性和测试

工程包含可通过环境变量启用的模拟器 Smoke Test，覆盖标题菜单、Music Room、Replay 返回、Bomb、自动放 B、关卡结算、跨关卡重建和完整六面流程。App 还会在 Documents 中持续写入运行日志，便于定位只在真机出现的问题。

验证节点 `0.4.0 (103)` 已通过：

- 第六面直达和实际 Bomb 计数验证；
- Music Room 进入与加载遮罩关闭；
- 第一面到第二面的完整资源重建；
- arm64、iOS 14.0 最低版本、签名、资源和 IPA 压缩完整性检查；
- 第六面停在战斗区原点的附属使魔/光效抑制与诊断日志。
- iOS 原生作弊码弹窗及全开存档写入路径的构建检查；
- 键盘 Esc 的 `SDL_QUIT` 防退出兼容路径；

## 构建 IPA

### 环境

- Intel 或 Apple Silicon Mac；
- Xcode 和 iPhoneOS SDK；
- CMake 3.20 或更高版本；
- Python 3；
- 合法取得的 `th08.dat`、`thbgm.dat` 和 `msgothic.ttc`。

把三个数据文件放在同一目录，例如 `~/TH08-data`：

```text
~/TH08-data/
  th08.dat
  thbgm.dat
  msgothic.ttc
```

首次构建需要联网下载固定版本的 SDL 依赖：

```sh
git clone https://github.com/Ymgjdsh/th08-ios-port.git
cd th08-ios-port
chmod +x ios/*.sh

TH08_IOS_DATA_DIR="$HOME/TH08-data" \
IOS_VERSION=0.3.9 \
IOS_BUILD=94 \
./ios/build_ios.sh
```

默认输出位于 `build-ios/`。也可以指定输出路径：

```sh
TH08_IOS_DATA_DIR="$HOME/TH08-data" \
OUTPUT_IPA="$HOME/Desktop/th08-ios.ipa" \
./ios/build_ios.sh
```

重复离线构建时，可通过 `TH08_IOS_DEPS_DIR` 指向下列已经下载的源码目录：

```text
SDL-release-2.30.9
SDL_image-release-2.8.2
SDL_mixer-release-2.8.0
SDL_ttf-release-2.22.0
```

脚本会检查 App 版本、两个 DAT、日文字体、图标资源和可执行文件，随后执行 ad-hoc 签名并打包 IPA。ad-hoc 签名不等于 Apple 开发者签名；实际安装方式取决于设备和签名环境。

## 日志

发生黑屏、卡死或闪退时，请同时提供设备型号、iOS 版本、发生时间和完整操作路径。可从 App 的共享文件区域导出：

```text
Documents/startup.log
Documents/th08-startup.log
Documents/startup.log.crash
```

日志中的 `stage-transition`、`smoke-*`、`player-bomb`、`stage6-corner-familiar` 和 `frame/profile` 可用于区分资源加载、脚本状态、输入、渲染和性能问题。

## 仓库结构

```text
config/i18n.csv            构建期文本表
ios/                       App 元数据、图标、启动页和 IPA 脚本
resources/modern-icon.png  现代桌面后端保留的项目图标
scripts/generate_i18n.py   文本头文件生成器
src/                       游戏重构源码与平台兼容层
src/modern/ios/            iOS 运行时、渲染和触摸实现
src/modern/linux/include/  iOS 仍使用的 Win32 / D3D 类型兼容头
CMakeLists.txt             现代平台构建入口
```

仓库已移除不参与 iOS 构建的 Agent 技能、Docker、Linux 发布 CI、VC7/reccmp 数据库、IDA/Ghidra 脚本、演示 GIF 和历史分析笔记。上游提交历史仍被保留，以维持原作者署名和来源可追溯性。

## 参考项目与致谢

本移植建立在社区多年工作的基础上，而不是从零独立完成：

- [N0zoM1z0/th08](https://github.com/N0zoM1z0/th08)：当前主要 TH08 重构和可运行 Linux 基线；
- [GensokyoClub/th08](https://github.com/GensokyoClub/th08)：更早的 TH08 反编译/重构基础及历史贡献；
- [N0zoM1z0/th08-web](https://github.com/N0zoM1z0/th08-web)：WebAssembly、数据边界和跨平台渲染实现的重要参考；
- [N0zoM1z0/th07](https://github.com/N0zoM1z0/th07)：相邻世代引擎结构、重构命名和行为参考；
- [CNTianQi233/th06-sdl2](https://github.com/CNTianQi233/th06-sdl2)：TH06 SDL2 移植和相邻引擎行为参考；
- 本地 TH06 / TH07 iOS 移植：触摸、按键布局、旋转、性能菜单和真机诊断经验来源；
- [SDL2](https://github.com/libsdl-org/SDL)、[SDL_image](https://github.com/libsdl-org/SDL_image)、[SDL_mixer](https://github.com/libsdl-org/SDL_mixer)、[SDL_ttf](https://github.com/libsdl-org/SDL_ttf)：跨平台运行基础。

感谢所有上游作者、测试者以及提供真机日志和复现步骤的玩家。Git 历史保留了导入仓库中的原始作者信息；本仓库不把上游重构成果重新声明为 iOS 移植维护者的个人作品。

## AI / Vibe Coding 声明

本项目的 iOS 专用兼容层、触摸系统、菜单与性能选项、诊断日志、回归脚本、部分稳定性修复和文档中，有大量内容通过 AI 辅助的迭代式 Vibe Coding 完成，主要使用 OpenAI Codex。

人类参与者负责需求定义、原作资料准备、设备操作、真机测试、日志反馈、结果验收和发布决策；AI 负责代码分析、实现候选、构建自动化和问题定位。提交显示的维护者身份用于仓库管理，不表示每一行代码都由该账号逐字手写。

所有 AI 生成或修改的内容都可能存在误判。已经编译或通过自动测试也不等于完全正确，尤其是反编译结构、64 位布局、Replay、符卡脚本和 GPU 状态。后续贡献应优先附带可重复测试、真机日志和清晰的行为证据。

上游 TH08 重构工作有其独立作者和历史，不应被归类为本项目的 AI 产物。

## 版权与许可证

《东方Project》《东方永夜抄》及其角色、音乐、美术和原版数据的权利属于上海爱丽丝幻乐团 / ZUN。此项目与 ZUN、上海爱丽丝幻乐团或 Apple 无官方关系。

仓库中的代码和文档按 [MIT License](LICENSE) 提供。MIT License 不授予任何原版游戏、商业字体、音乐、图像或数据文件的再分发权利。
