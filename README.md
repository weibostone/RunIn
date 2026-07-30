# 🚀 RunIn - The Ultimate Windows Terminal Launcher

RunIn is a lightweight, portable, and highly configurable terminal launcher for Windows. 
It allows you to instantly launch your favorite terminals, shells, or command-line tools directly into the directory you are currently working in, with zero configuration required.

Whether you are using PowerShell, Git Bash, MSYS2, Cmder, or even WSL-based tools like Zellij and Claude Code, RunIn brings them all together into a single, accessible context menu.

![RunIn Menu](https://img.shields.io/badge/Action-Cursor%20Menu-blue) 
![Portable](https://img.shields.io/badge/Mode-Portable-success)
![C++](https://img.shields.io/badge/Built%20with-C++%2B%2B%20Win32-red)

---

## ✨ Key Features

- **Context Menu Popup**: Double-click `RunIn.exe` or `ri.exe` (or bind it to your mouse's side button) to instantly pop up a menu at your cursor. Selecting a terminal launches it right where you are.
- **Auto Working Directory Mapping**: Automatically maps your current File Explorer directory to the launched terminal using the `{current_dir}` dynamic variable. No more `cd` typing!
- **One-Click Auto Search**: Built-in auto-discovery for over 20+ popular terminals and dev tools. Don't know the exact path? Let RunIn find them for you.
- **Pending Templates**: If a selected terminal isn't installed, RunIn creates a "Pending" template in your config, allowing you to manually fill in the path later without breaking your list.
- **Global PATH Injection**: One click to inject RunIn's directory into your System User PATH, making it accessible from anywhere via `Win + R` or CMD.
- **100% Portable**: No installation required. All configurations are saved in a single `config.txt` file alongside the executable.
- **Blazing Fast & Lightweight**: Pure C++ Win32 API. No Electron, no .NET runtime, no background processes. Just a tiny executable.

---

## 🛠️ Supported Auto-Search Terminals

RunIn can automatically detect and configure the following tools:

| Category | Terminals / Tools |
| :--- | :--- |
| **Shells & Basics** | CMD, PowerShell, PowerShell 7, Windows Terminal |
| **Git** | Git Bash, Git CMD |
| **MSYS2** | MinGW x64/x86, MSYS, Clang x64, UCRT x64 |
| **Conda** | Anaconda PowerShell Prompt, Anaconda Prompt |
| **Visual Studio** | x64 Native Tools Cmd, x64_x86 Cross Tools Cmd |
| **Emulators** | Cmder, ConEmu, Warp |
| **WSL & Multiplexers**| WSL (PS7), WSL (CMD), Zellij (WSL), Tmux (WSL) |
| **Modern CLI / AI** | Zellij (Native), Codex (PowerShell/WSL), Claude Code (PowerShell/WSL) |

---

## 📖 Usage Guide

### 1. Context Menu Mode
Simply double-click `RunIn.exe`. A menu will appear at your mouse cursor. Click any item to launch it in the current directory.

### 2. Direct Command Line Launch
You can bypass the menu by passing the index number as an argument. This is perfect for integration with other tools or scripts.
