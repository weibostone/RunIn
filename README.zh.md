# 🚀 RunIn - 跨平台终极终端启动器

RunIn 是一款轻量级、绿色、高可配置的跨平台终端启动工具。它允许你一键将喜爱的终端、Shell 或命令行工具直接启动到你当前正在浏览的目录中，真正做到开箱即用。

无论你是在 Windows 上使用 PowerShell、Git Bash、MSYS2、Cmder，还是在 Linux/macOS 上使用原生 Bash、Zsh、Fish、Tmux 或 Zellij，RunIn 都能将它们整合到一个便捷的快捷菜单中。

**常用场景：**
- **Windows**: 你用 Git Bash 工具打开了一个目录，从 GitHub 上 pull 了最新代码到本地，之后又用调用 x64 Native Tools Cmd 打开了这个目录进行编译，最后又调用 VSCode 打开了这个目录查看代码内容。
- **Linux/Mac**: 你通过 SSH 远程连接到 Ubuntu 服务器，在默认 Bash 中敲入 `ri` 选 Zsh 开启了一个干净的新 Shell 做试验，试验完成后输入 `exit`，屏幕瞬间恢复为你敲 `ri` 之前的原有 Bash 界面，历史记录完好无损，仿佛什么都没发生过。

![RunIn Menu](https://img.shields.io/badge/Action-Cursor%20Menu-blue)
![Portable](https://img.shields.io/badge/Mode-Portable-success)
![C++](https://img.shields.io/badge/Built%20with-C++%2B%2B%20Cross%20Platform-red)

---

## 🎬 演示

下图展示了 RunIn 在 Windows 下的典型使用流程（文件资源管理器的地址栏中输入`ri`，弹出菜单并选择Git Bash终端；以及通过输入`ri 1`，直接启动Git Bash终端）：

![RunIn 使用演示](assets/introduce.gif)

> 若动画未能正常显示，请确保 `assets/introduce.gif` 文件存在于项目根目录下的 `assets` 文件夹中，或直接运行程序体验。

---

## ✨ 核心特性

### 通用特性
- **光标处/终端内弹出菜单**：Windows文件资源管理器的地址栏输入`ri`回车，或双击`ri.exe`即可弹出菜单。在 Linux/macOS 任意终端中输入`ri`即可弹出 TUI 菜单。选择所需终端即可自动切换到当前目录。
- **一键自动搜索**：内置 20+ 款主流终端和开发工具的自动发现功能（Windows 平台）。Linux/Mac 平台自动检测系统已安装的 Shell 及复用器。
- **待设定模板**：如果勾选的终端未安装，RunIn 会生成一个“待设定”模板放入配置列表，允许你之后手动补全路径，而不会中断现有的配置序列。
- **自动工作目录映射**：通过 `{current_dir}` 动态变量，自动将你当前文件管理器或终端所在的目录映射为所启动终端的工作目录。告别繁琐的 `cd` 命令！
- **100% 绿色便携**：无需安装。所有配置均保存在与执行文件同目录下的单个 `config.ini` 文件中。
- **极速且轻量**：纯 C++ 编写，没有后台驻留进程。只有一个极小的可执行文件。
- **命令行直接启动**：支持 `ri <序号>` 跳过菜单直接启动对应工具，如 `ri 3`，非常适合与其他工具或脚本集成。



## 🛠️ 支持自动搜索的终端

RunIn 能够自动检测并配置以下工具：

| 平台 | 分类 | 终端 / 工具 |
| :--- | :--- | :--- |
| **Windows** | **基础 Shell** | CMD, PowerShell, PowerShell 7, Windows Terminal |
| | **Git** | Git Bash, Git CMD |
| | **MSYS2** | MinGW x64/x86, MSYS, Clang x64, UCRT x64 |
| | **Conda** | Anaconda PowerShell Prompt, Anaconda Prompt |
| | **Visual Studio** | x64 Native Tools Cmd, x64_x86 Cross Tools Cmd |
| | **终端模拟器** | Cmder, ConEmu, Warp |
| | **WSL 与复用器** | WSL (PS7), WSL (CMD), Zellij (WSL), Tmux (WSL) |
| | **现代 CLI / AI** | Zellij (原生), Codex, Claude Code |
| | **其他图形界面工具** | VS Code, Virtual Studio, File Explorer |
| **Linux/macOS** | **终端模拟器 (GUI)** | GNOME Terminal, Konsole, Mac Terminal, iTerm2 |
| | **基础 Shell** | Bash, Zsh, Fish |
| | **复用器 (TUI)** | Tmux, Zellij |

---

## 📖 使用指南

### 1. 第一次使用

- **Windows**：双击 `ri.exe` 弹出菜单，选择 `Setting` 项，进入设置页面。点击 `Add System PATH` 按钮将 `ri.exe` 所在路径加入到系统 PATH 中。之后在设置界面点击 `Auto Search Terminals` 搜索已安装工具，最后 `Save Config` 退出。
- **Linux/macOS**：将 `ri` 可执行文件放入系统 PATH（如 `/usr/local/bin` 或 `~/.local/bin`）。在任意终端输入 `ri`，选择 `Setting` 进入 TUI 配置界面，选择 `Add Tool`，利用内置模板添加你需要的 Shell 或终端。

### 2. 地址栏/文件管理器模式 (Windows)

Windows文件资源管理器的地址栏中输入`ri`或双击`ri.exe`（或绑定鼠标侧键）即可弹出菜单。选择所需终端即可在当前目录下启动该终端工具。

### 3. 终端内嵌模式 (全平台)

在各种终端软件中输入`ri`即可弹出菜单，选择所需终端即可。点击任意菜单项即可在当前目录下启动该终端工具。

**Linux/macOS 典型场景**：在使用 SSH 连接的 Ubuntu 服务器上，你在默认 Bash 里编译完代码，想用一个干净的环境测试运行。输入 `ri`，选 Bash，屏幕一闪显示 `RunIt: Bash started at /home/user/project on 2024-05-24 15:30:45...`，你执行测试程序后输入 `exit`，屏幕瞬间恢复到刚才编译代码的输出状态。

### 4. 命令行直接启动 (全平台)

你可以通过传入序号参数来跳过菜单弹窗，直接启动对应配置项。例如你记住了 PowerShell / Zsh 在菜单中的序号是 2，你可以直接输入 `ri 2` 启动它。这非常适合与其他工具或脚本集成。

---

## 🔧 从源码构建

如果你希望自行编译 RunIn，请按以下步骤操作：

### Windows
1. 在项目根目录下创建 `build` 文件夹。
2. 打开 **“x64 Native Tools Command Prompt for VS”**（Visual Studio 的开发人员命令提示符），并切换到 `build` 目录。
3. 执行以下 CMake 命令生成 NMake 构建文件，编译文件：
   ```bash
   cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release ..
   cmake --build .
   ```

### Linux / macOS
1. 确保系统已安装 `g++` 或 `clang++` 以及 `make` 和 `cmake`。
2. 在项目根目录下创建 `build` 文件夹并进入。
3. 执行cmake和make。

   #### Ubuntu/Debian
   ```bash
   sudo apt update && sudo apt install build-essential cmake
   mkdir build && cd build
   cmake ..
   make
   sudo cp RunIn /usr/local/bin/
   ```

   #### RHEL/CentOS/Fedora
   ```bash
   sudo dnf install gcc gcc-c++ cmake
   mkdir build && cd build
   cmake ..
   make
   sudo cp RunIn /usr/local/bin/
   ```

   #### macOS (Intel & Apple Silicon) 
   ```bash
   xcode-select --install
   brew install cmake
   mkdir build && cd build
   cmake ..
   make
   sudo cp RunIn /usr/local/bin/ 
   # 对于 Apple Silicon (M1/M2)，推荐拷贝到 /opt/homebrew/bin 或 /usr/local/bin 均可
   ```

