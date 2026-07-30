# 🚀 RunIn - Windows 终极终端启动器

RunIn 是一款专为 Windows 设计的轻量级、绿色、高可配置的终端启动工具。
它允许你一键将喜爱的终端、Shell 或命令行工具直接启动到你当前正在浏览的目录中，真正做到开箱即用。

无论你使用的是 PowerShell、Git Bash、MSYS2、Cmder，还是基于 WSL 的 Zellij 和 Claude Code，RunIn 都能将它们整合到一个便捷的快捷菜单中。

![RunIn Menu](https://img.shields.io/badge/Action-Cursor%20Menu-blue) 
![Portable](https://img.shields.io/badge/Mode-Portable-success)
![C++](https://img.shields.io/badge/Built%20with-C++%2B%2B%20Win32-red)

---

## ✨ 核心特性

- **光标处弹出菜单**：双击 `RunIn.exe` 或`ri.exe`（或绑定鼠标侧键）即可在光标位置弹出菜单。选择终端即可在当前位置启动。
- **自动工作目录映射**：通过 `{current_dir}` 动态变量，自动将你当前文件管理器所在的目录映射为所启动终端的工作目录。告别繁琐的 `cd` 命令！
- **一键自动搜索**：内置 20+ 款主流终端和开发工具的自动发现功能。不知道路径？交给 RunIn 自动检测。
- **待设定模板**：如果勾选的终端未安装，RunIn 会生成一个“待设定”模板放入配置列表，允许你之后手动补全路径，而不会中断现有的配置序列。
- **全局 PATH 注入**：一键将 RunIn 所在目录注入系统用户环境变量，让你可以通过 `Win + R` 或 CMD 在任何地方调用它。
- **100% 绿色便携**：无需安装。所有配置均保存在与执行文件同目录下的单个 `config.txt` 文件中。
- **极速且轻量**：纯 C++ Win32 API 编写。没有 Electron，没有 .NET 运行时，没有后台驻留进程。只有一个极小的可执行文件。

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

### 1. 菜单模式
直接双击 `RunIn.exe`，鼠标光标处将弹出菜单。点击任意项即可在当前目录下启动该工具。

### 2. 命令行直接启动
你可以通过传入序号参数来跳过菜单弹窗，直接启动对应配置项。这非常适合与其他工具或脚本集成。
