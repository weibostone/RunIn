# 🚀 RunIn - The Ultimate Windows Terminal Launcher

RunIn is a lightweight, portable, and highly configurable terminal launcher for Windows. 
It allows you to instantly launch your favorite terminals, shells, or command-line tools directly into the directory you are currently working in, with zero configuration required.

Whether you are using PowerShell, Git Bash, MSYS2, Cmder, or even WSL-based tools like Zellij and Claude Code, RunIn brings them all together into a single, accessible context menu.

![RunIn Menu](https://img.shields.io/badge/Action-Cursor%20Menu-blue) 
![Portable](https://img.shields.io/badge/Mode-Portable-success)
![C++](https://img.shields.io/badge/Built%20with-C++%2B%2B%20Win32-red)

---

## 🎬 Demo

The following GIF demonstrates the typical usage of RunIn (type `ri` in the File Explorer address bar, select Git Bash from the popup menu; and type `ri 1` to directly launch Git Bash):

![RunIn Demo](assets/introduce.gif)

> If the animation does not display, ensure `assets/introduce.gif` is placed in the `assets` folder under the project root, or simply try the program yourself.

---

## ✨ Key Features

- **Popup menu at cursor**: Enter `ri` in the File Explorer address bar/terminals, or double-click `ri.exe` (or bind it to your mouse side button) to pop up the menu. Select any terminal to launch it and automatically switch to the current directory. For example, you can launch PowerShell 7 directly from the File Explorer address bar, enter Claude Code or Codex, and switch to the current directory seamlessly.
- **One-Click Auto Search**: Built-in auto-discovery for over 20+ popular terminals and dev tools. Don't know the exact path? Let RunIn find them for you.
- **Pending Templates**: If a selected terminal isn't installed, RunIn creates a "Pending" template in your config, allowing you to manually fill in the path later without breaking your list.
- **Auto Working Directory Mapping**: Automatically maps your current File Explorer directory to the launched terminal using the `{current_dir}` dynamic variable. No more `cd` typing!
- **100% Portable**: No installation required. All configurations are saved in a single `config.txt` file alongside the executable.
- **Blazing Fast & Lightweight**: Pure C++ Win32 API, no background processes. Just a tiny executable.

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

### 1. First-time Setup
Double-click `ri.exe` to pop up the menu, select the `Setting` item to enter the configuration interface. Click the `Add to System PATH` button to add `ri.exe`'s directory to the system PATH, making it convenient to launch RunIn from the address bar. Then, add your desired terminal tools to the menu. You can use the `Auto Search Terminals` button to automatically detect installed terminals on your system. After that, click `Save Config Apply` to exit the settings interface.

### 2. Address Bar Mode
Enter `ri` in the File Explorer address bar, or double-click `ri.exe` (or bind it to your mouse side button) to pop up the menu. Select any terminal to launch it in the current directory.

### 3. Terminal Mode
Simply enter `ri` in various terminal applications to bring up a menu and select the desired terminal. Selecting any menu item will launch that terminal tool in the current directory. For example, when using CMD, if you need to open the current directory in Git Bash, simply enter `ri` and select Git Bash from the menu.

### 4. Direct Command Line Launch
You can bypass the menu by passing the index number as an argument. For example, if you remember that PowerShell is item #3 in the menu, you can type `ri 3` in the File Explorer address bar to directly launch PowerShell and switch to the current directory. This is perfect for integration with other tools or scripts.

---

## 🔧 Building from Source

If you prefer to compile RunIn yourself, follow these steps:

1. Create a `build` folder in the project root.
2. Open the **“x64 Native Tools Command Prompt for VS”** (Visual Studio developer command prompt) and change to the `build` directory.
3. Run the following CMake commands to generate NMake build files and compile:
   ```bash
   cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release ..
   cmake --build .