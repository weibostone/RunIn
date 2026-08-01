# 🚀 RunIn - Windows 终极终端启动器

RunIn 是一款专为 Windows 设计的轻量级、绿色、高可配置的终端启动工具。
它允许你一键将喜爱的终端、Shell 或命令行工具直接启动到你当前正在浏览的目录中，真正做到开箱即用。

无论你使用的是 PowerShell、Git Bash、MSYS2、Cmder，还是基于 WSL 的 Zellij 和 Claude Code，RunIn 都能将它们整合到一个便捷的快捷菜单中。

![RunIn Menu](https://img.shields.io/badge/Action-Cursor%20Menu-blue) 
![Portable](https://img.shields.io/badge/Mode-Portable-success)
![C++](https://img.shields.io/badge/Built%20with-C++%2B%2B%20Win32-red)

---

## 🎬 演示

下图展示了 RunIn 的典型使用流程（Windows文件资源管理器的地址栏中输入`ri`，弹出菜单并选择Git Bash终端，以及通过输入`ri 1`，直接启动Git Bash终端）：

![RunIn 使用演示](assets/introduce.gif)

> 若动画未能正常显示，请确保 `assets/introduce.gif` 文件存在于项目根目录下的 `assets` 文件夹中，或直接运行程序体验。

---

## ✨ 核心特性

- **光标处弹出菜单**：Windows文件资源管理器的地址栏中输入`ri`或双击`ri.exe`（或绑定鼠标侧键）即可弹出菜单，选择所需终端即可。启动相应终端后，自动切换到当前目录。例如：支持在Windows文件资源管理器的地址栏直接启动PowerShell7终端，进入Claude Code或Codex程序，之后切换到当前目录。
- **一键自动搜索**：内置 20+ 款主流终端和开发工具的自动发现功能。不知道路径？交给 RunIn 自动检测。
- **待设定模板**：如果勾选的终端未安装，RunIn 会生成一个“待设定”模板放入配置列表，允许你之后手动补全路径，而不会中断现有的配置序列。
- **全局 PATH 注入**：一键将 RunIn 所在目录注入系统用户环境变量，让你可以通过 `Win + R` 或 CMD 在任何地方调用它。
- **自动工作目录映射**：通过 `{current_dir}` 动态变量，自动将你当前文件管理器所在的目录映射为所启动终端的工作目录。告别繁琐的 `cd` 命令！
- **100% 绿色便携**：无需安装。所有配置均保存在与执行文件同目录下的单个 `config.txt` 文件中。
- **极速且轻量**：纯 C++ Win32 API 编写，没有后台驻留进程。只有一个极小的可执行文件。

---

## 🛠️ 支持自动搜索的终端

RunIn 能够自动检测并配置以下工具：

| 分类 | 终端 / 工具 |
| :--- | :--- |
| **基础 Shell** | CMD, PowerShell, PowerShell 7, Windows Terminal |
| **Git** | Git Bash, Git CMD |
| **MSYS2** | MinGW x64/x86, MSYS, Clang x64, UCRT x64 |
| **Conda** | Anaconda PowerShell Prompt, Anaconda Prompt |
| **Visual Studio** | x64 Native Tools Cmd, x64_x86 Cross Tools Cmd |
| **终端模拟器** | Cmder, ConEmu, Warp |
| **WSL 与复用器** | WSL (PS7), WSL (CMD), Zellij (WSL), Tmux (WSL) |
| **现代 CLI / AI** | Zellij (原生), Codex (PowerShell/WSL), Claude Code (PowerShell/WSL) |

---

## 📖 使用指南

### 1. 第一次使用
双击`ri.exe`弹出菜单，选择Setting项，进入设置页面，点击Inject System PATH按钮将ri.exe所在路径加入到系统PATH中，以方便在地址栏中调用RunIn，之后在设置界面添加各种所需的终端工具到菜单中，可以点击Auto Search Terminals按钮使用自动搜索功能，找到系统中已安装的各种终端工具。之后点击Save Config Apply按钮退出设置界面。

### 2. 菜单模式
Windows文件资源管理器的地址栏中输入`ri`或双击`ri.exe`（或绑定鼠标侧键）即可弹出菜单，选择所需终端即可。点击任意菜单项即可在当前目录下启动该终端工具。

### 3. 命令行直接启动
你可以通过传入序号参数来跳过菜单弹窗，直接启动对应配置项，例如你记住了PowerShell在菜单中的序号是3，你可以Windows文件资源管理器的地址栏中输入`ri 3`直接启动PowerShell并且切换到当前目录。这非常适合与其他工具或脚本集成。

---

## 🔧 从源码构建

如果你希望自行编译 RunIn，请按以下步骤操作：

1. 在项目根目录下创建 `build` 文件夹。
2. 打开 **“x64 Native Tools Command Prompt for VS”**（Visual Studio 的开发人员命令提示符），并切换到 `build` 目录。
3. 执行以下 CMake 命令生成 NMake 构建文件，编译文件：
   ```bash
   cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release ..
   cmake --build .