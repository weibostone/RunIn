#if defined(_WIN32)

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
// =================================================

#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <windows.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <filesystem>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "comctl32.lib")

namespace fs = std::filesystem;

// ================== Data Structures ==================

struct ToolItem {
    std::wstring name;
    std::wstring path;
    std::wstring args;
    std::wstring hotkey;
};

std::vector<ToolItem> g_tools;
fs::path g_exeDir;

HFONT hGlobalFont = NULL;
HFONT hBoldFont = NULL;
HBRUSH hBkgBrush = NULL;

std::vector<int> g_listBoxMap;

// Forward declarations
HWND CreateCtrl(LPCWSTR cls, LPCWSTR text, DWORD style, DWORD exStyle, int x, int y, int w, int h, HMENU id, HWND parent, HFONT font);
void RunConfigGUI(HINSTANCE hInstance);

// ================== Helper Functions ==================

std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstr(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size);
    return wstr;
}

std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &str[0], size, NULL, NULL);
    return str;
}

fs::path GetExeDir() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    return fs::path(exePath).parent_path();
}

bool FileExistsW(const std::wstring& path) {
    if (path.empty()) return false;
    DWORD attr = GetFileAttributesW(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

bool DirExistsW(const std::wstring& path) {
    if (path.empty()) return false;
    DWORD attr = GetFileAttributesW(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

std::wstring FindExecutablePath(const std::wstring& exeName) {
    wchar_t pathBuffer[MAX_PATH] = { 0 };
    wcscpy_s(pathBuffer, MAX_PATH, exeName.c_str());
    if (PathFindOnPathW(pathBuffer, NULL)) return pathBuffer;
    return L"";
}

std::vector<std::wstring> GetAllDrives() {
    std::vector<std::wstring> drives;
    DWORD mask = GetLogicalDrives();
    for (char i = 0; i < 26; ++i) {
        if (mask & (1 << i)) {
            std::wstring d = std::wstring(1, L'A' + i) + L":";
            drives.push_back(d);
        }
    }
    return drives;
}

// ================== Config File R/W ==================

void LoadConfig() {
    g_tools.clear();
    fs::path configPath = g_exeDir / "config.ini";
    if (!fs::exists(configPath)) return;

    std::ifstream ifs(configPath);
    std::string line;
    ToolItem currentTool;
    bool inItem = false;

    while (std::getline(ifs, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (line.empty() || line[0] == '#') continue;

        if (!line.empty() && line.front() == '[' && line.back() == ']') {
            std::string section = line.substr(1, line.size() - 2);
            if (section.rfind("Item", 0) == 0) {
                if (inItem) g_tools.push_back(currentTool);
                currentTool = ToolItem();
                inItem = true;
            } else {
                if (inItem) { g_tools.push_back(currentTool); inItem = false; }
            }
        } else if (inItem) {
            auto eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                std::string key = line.substr(0, eqPos);
                std::string val = line.substr(eqPos + 1);
                if (key == "Name") currentTool.name = StringToWString(val);
                else if (key == "Path") currentTool.path = StringToWString(val);
                else if (key == "Args") currentTool.args = StringToWString(val);
                else if (key == "Hotkey") currentTool.hotkey = StringToWString(val);
            }
        }
    }
    if (inItem) g_tools.push_back(currentTool);
}

void SaveConfig() {
    fs::path configPath = g_exeDir / "config.ini";
    std::ofstream ofs(configPath);
    if (!ofs.is_open()) return;
    
    ofs << "# RunIt Config File\n";
    ofs << "# Format: [ItemX]\nName=...\nPath=...\nArgs=...\nHotkey=...\n\n";
    
    for (size_t i = 0; i < g_tools.size(); i++) {
        ofs << "[Item" << (i + 1) << "]\n";
        ofs << "Name=" << WStringToString(g_tools[i].name) << "\n";
        ofs << "Path=" << WStringToString(g_tools[i].path) << "\n";
        ofs << "Args=" << WStringToString(g_tools[i].args) << "\n";
        ofs << "Hotkey=" << WStringToString(g_tools[i].hotkey) << "\n\n";
    }
}

// ================== System Functions & Terminal Search ==================

bool AddToSystemPath() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Environment", 0, KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS) return false;

    wchar_t pathVal[32767];
    DWORD bufSize = sizeof(pathVal);
    RegQueryValueExW(hKey, L"Path", NULL, NULL, (LPBYTE)pathVal, &bufSize);

    std::wstring currentPath = pathVal;
    std::wstring exeDirStr = g_exeDir.wstring();

    if (currentPath.find(exeDirStr) != std::wstring::npos) {
        RegCloseKey(hKey); return true;
    }

    std::wstring newPath = currentPath;
    if (!newPath.empty() && newPath.back() != L';') newPath += L";";
    newPath += exeDirStr;

    RegSetValueExW(hKey, L"Path", 0, REG_EXPAND_SZ, (LPBYTE)newPath.c_str(), (newPath.size() + 1) * sizeof(wchar_t));
    RegCloseKey(hKey);

    DWORD_PTR result;
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"Environment", SMTO_ABORTIFHUNG, 5000, &result);
    return true;
}

// ================== System PATH Uninject ==================
bool RemoveFromSystemPath() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Environment", 0, KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS)
        return false;

    wchar_t pathVal[32767];
    DWORD bufSize = sizeof(pathVal);
    DWORD type = 0;
    if (RegQueryValueExW(hKey, L"Path", NULL, &type, (LPBYTE)pathVal, &bufSize) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return false;
    }

    std::wstring currentPath = pathVal;
    std::wstring exeDirStr = g_exeDir.wstring();

    // Remove trailing backslash for consistent matching
    if (!exeDirStr.empty() && exeDirStr.back() == L'\\') {
        exeDirStr.pop_back();
    }

    size_t pos = currentPath.find(exeDirStr);
    if (pos == std::wstring::npos) {
        RegCloseKey(hKey);
        return false; // Not found, nothing to remove
    }

    // Boundary check: Ensure we don't match partial paths (e.g., C:\RunIt inside C:\RunItBeta)
    bool validStart = (pos == 0) || (currentPath[pos - 1] == L';');
    bool validEnd = (pos + exeDirStr.length() == currentPath.length()) || (currentPath[pos + exeDirStr.length()] == L';');

    if (!validStart || !validEnd) {
        RegCloseKey(hKey);
        return false; // Only found a partial match, do not remove
    }

    // Remove the directory and the associated semicolon
    if (pos > 0 && currentPath[pos - 1] == L';') {
        // Remove preceding semicolon and the directory
        currentPath.erase(pos - 1, exeDirStr.length() + 1);
    } else if (pos + exeDirStr.length() < currentPath.length() && currentPath[pos + exeDirStr.length()] == L';') {
        // Remove directory and following semicolon
        currentPath.erase(pos, exeDirStr.length() + 1);
    } else {
        // It's the only element in the PATH
        currentPath.erase(pos, exeDirStr.length());
    }

    // Write back the modified path
    DWORD newSize = (currentPath.size() + 1) * sizeof(wchar_t);
    RegSetValueExW(hKey, L"Path", 0, REG_EXPAND_SZ, (LPBYTE)currentPath.c_str(), newSize);
    RegCloseKey(hKey);

    // Broadcast changes
    DWORD_PTR result;
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"Environment", SMTO_ABORTIFHUNG, 5000, &result);
    return true;
}


bool ToolExists(const std::wstring& name) {
    for (const auto& t : g_tools) if (t.name == name) return true;
    return false;
}

void AddTool(const std::wstring& name, const std::wstring& path, const std::wstring& args, const std::wstring& hotkey = L"") {
    if (ToolExists(name)) return;
    g_tools.push_back({name, path, args, hotkey});
}

std::wstring FindCondaBasePath() {
    std::wstring condaExe = FindExecutablePath(L"conda.exe");
    if (!condaExe.empty()) {
        fs::path p(condaExe);
        if (p.parent_path().filename() == L"Scripts") return p.parent_path().parent_path().wstring();
        return p.parent_path().wstring();
    }

    std::vector<std::wstring> drives = GetAllDrives();
    std::vector<std::wstring> dirs = {L"\\ProgramData\\Anaconda3", L"\\ProgramData\\miniconda3"};
    wchar_t* profile = _wgetenv(L"USERPROFILE");
    if (profile) {
        dirs.push_back(std::wstring(profile) + L"\\anaconda3");
        dirs.push_back(std::wstring(profile) + L"\\miniconda3");
        dirs.push_back(std::wstring(profile) + L"\\AppData\\Local\\anaconda3");
        dirs.push_back(std::wstring(profile) + L"\\AppData\\Local\\miniconda3");
    }

    for (const auto& d : drives) {
        for (const auto& dir : dirs) {
            std::wstring path = d + dir;
            if (DirExistsW(path)) return path;
        }
    }
    return L"";
}

std::wstring FindVsBatch(const std::wstring& batName) {
    std::vector<std::wstring> drives = GetAllDrives();
    std::vector<std::wstring> progs = {L"\\Program Files\\Microsoft Visual Studio", L"\\Program Files (x86)\\Microsoft Visual Studio"};
    std::vector<std::wstring> years = {L"2022", L"2019", L"2017"};
    std::vector<std::wstring> editions = {L"Community", L"Professional", L"Enterprise", L"BuildTools"};

    for (const auto& d : drives) {
        for (const auto& prog : progs) {
            std::wstring basePath = d + prog;
            if (!DirExistsW(basePath)) continue;
            for (const auto& year : years) {
                std::wstring yearPath = basePath + L"\\" + year;
                if (!DirExistsW(yearPath)) continue;
                for (const auto& ed : editions) {
                    std::wstring batPath = yearPath + L"\\" + ed + L"\\VC\\Auxiliary\\Build\\" + batName;
                    if (FileExistsW(batPath)) return batPath;
                }
            }
        }
    }
    return L"";
}

std::wstring FindMSYS2Path() {
    std::vector<std::wstring> drives = GetAllDrives();
    std::vector<std::wstring> dirs = {L"\\msys64", L"\\msys32"};
    for (const auto& d : drives) {
        for (const auto& dir : dirs) {
            std::wstring path = d + dir;
            if (DirExistsW(path)) {
                if (FileExistsW(path + L"\\msys2_shell.cmd")) return path;
            }
        }
    }
    return L"";
}

std::wstring FindCmderPath() {
    std::vector<std::wstring> drives = GetAllDrives();
    std::vector<std::wstring> dirs = {L"\\cmder", L"\\Program Files\\cmder", L"\\Program Files (x86)\\cmder"};
    for (const auto& d : drives) {
        for (const auto& dir : dirs) {
            std::wstring path = d + dir;
            if (DirExistsW(path)) {
                std::wstring exe = path + L"\\Cmder.exe";
                if (FileExistsW(exe)) return exe;
            }
        }
    }
    return L"";
}

std::wstring FindConEmuPath() {
    std::vector<std::wstring> drives = GetAllDrives();
    std::vector<std::wstring> dirs = {L"\\Program Files\\ConEmu", L"\\Program Files (x86)\\ConEmu"};
    for (const auto& d : drives) {
        for (const auto& dir : dirs) {
            std::wstring path = d + dir;
            if (DirExistsW(path)) {
                std::wstring exe = path + L"\\ConEmu64.exe";
                if (FileExistsW(exe)) return exe;
                exe = path + L"\\ConEmu.exe";
                if (FileExistsW(exe)) return exe;
            }
        }
    }
    return L"";
}

std::wstring FindWarpPath() {
    std::vector<std::wstring> drives = GetAllDrives();
    std::vector<std::wstring> dirs = {L"\\Program Files\\Warp", L"\\Program Files (x86)\\Warp"};
    for (const auto& d : drives) {
        for (const auto& dir : dirs) {
            std::wstring path = d + dir;
            if (DirExistsW(path)) {
                std::wstring exe = path + L"\\Warp.exe";
                if (FileExistsW(exe)) return exe;
            }
        }
    }
    return FindExecutablePath(L"Warp.exe");
}

std::wstring FindWSLPath() {
    std::wstring p = FindExecutablePath(L"wsl.exe");
    if (!p.empty()) return p;
    wchar_t sysDir[MAX_PATH];
    if (GetSystemDirectoryW(sysDir, MAX_PATH)) {
        std::wstring p2 = std::wstring(sysDir) + L"\\wsl.exe";
        if (FileExistsW(p2)) return p2;
    }
    return L"";
}

#define IDC_CHK_GIT_BASH       2001
#define IDC_CHK_GIT_CMD        2002
#define IDC_CHK_PS             2003
#define IDC_CHK_PS7            2004
#define IDC_CHK_WT             2005
#define IDC_CHK_CMD            2006
#define IDC_CHK_CONDA_PS       2007
#define IDC_CHK_CONDA_CMD      2008
#define IDC_CHK_VS_X64         2009
#define IDC_CHK_VS_X86         2010
#define IDC_CHK_MSYS2_MINGW64 2011
#define IDC_CHK_MSYS2_MINGW32 2012
#define IDC_CHK_MSYS2_MSYS     2013
#define IDC_CHK_MSYS2_CLANG64 2014
#define IDC_CHK_MSYS2_UCRT64  2015
#define IDC_CHK_CMDER          2016
#define IDC_CHK_CONEMU         2017
#define IDC_CHK_ZELLIJ         2018
#define IDC_CHK_TMUX           2019
#define IDC_CHK_WARP           2020
#define IDC_CHK_WSL_PS7        2021
#define IDC_CHK_WSL_CMD        2022
#define IDC_CHK_ZELLIJ_WIN     2023
#define IDC_CHK_CODEX_PS       2024
#define IDC_CHK_CLAUDE_PS      2025
#define IDC_CHK_CODEX_WSL      2026
#define IDC_CHK_CLAUDE_WSL     2027
#define IDC_BTN_DO_SEARCH      2030

void ExecuteSearch(HWND hParent, std::vector<int> targets) {
    std::wstring sysDir = L"";
    wchar_t sysDirBuf[MAX_PATH];
    if (GetSystemDirectoryW(sysDirBuf, MAX_PATH)) sysDir = sysDirBuf;
    std::wstring sysCmd = sysDir + L"\\cmd.exe";
    std::wstring sysPs = sysDir + L"\\WindowsPowerShell\\v1.0\\powershell.exe";

    std::wstring ps7Path = FindExecutablePath(L"pwsh.exe");
    if (ps7Path.empty()) {
        ps7Path = L"C:\\Program Files\\PowerShell\\7\\pwsh.exe";
        if (!FileExistsW(ps7Path)) ps7Path = L"C:\\Program Files\\PowerShell\\7-preview\\pwsh.exe";
        if (!FileExistsW(ps7Path)) ps7Path = L"";
    }

    for (int id : targets) {
        std::wstring name = L"", path = L"", args = L"";
        
        switch (id) {
        case IDC_CHK_GIT_BASH:
            name = L"Git Bash";
            path = FindExecutablePath(L"git-bash.exe");
            break;
        case IDC_CHK_GIT_CMD:
            name = L"Git CMD";
            {
                std::wstring gitExe = FindExecutablePath(L"git.exe");
                if (!gitExe.empty())
                {
                    path = sysCmd;
                    args = L"/K \"" + fs::path(gitExe).parent_path().parent_path().wstring() + L"\\cmd\\git.cmd\" & cd /D \"{current_dir}\"";
                }
            }
            break;
        case IDC_CHK_PS:
            name = L"PowerShell";
            path = FindExecutablePath(L"powershell.exe");
            if (path.empty() && !sysDir.empty())
                path = sysPs;
            if (!path.empty())
                args = L"-NoExit -Command Set-Location '{current_dir}'";
            break;
        case IDC_CHK_PS7:
            name = L"PowerShell 7";
            path = ps7Path;
            if (!path.empty())
                args = L"-NoExit -Command Set-Location '{current_dir}'";
            break;
        case IDC_CHK_WT:
            name = L"Windows Terminal";
            path = FindExecutablePath(L"wt.exe");
            if (path.empty())
            {
                wchar_t *lad = _wgetenv(L"LOCALAPPDATA");
                if (lad)
                    path = std::wstring(lad) + L"\\Microsoft\\WindowsApps\\wt.exe";
                if (!FileExistsW(path))
                    path = L"";
            }
            if (!path.empty())
                args = L"-d \"{current_dir}\"";
            break;
        case IDC_CHK_CMD:
            name = L"CMD";
            path = sysCmd;
            args = L"/K cd /D \"{current_dir}\"";
            break;
        case IDC_CHK_CONDA_PS:
            name = L"Anaconda PowerShell";
            {
                std::wstring cb = FindCondaBasePath();
                if (!cb.empty())
                {
                    path = sysPs;
                    args = L"-NoExit -Command & '" + cb + L"\\shell\\condabin\\conda-hook.ps1'; conda activate '" + cb + L"'; Set-Location '{current_dir}'";
                }
            }
            break;
        case IDC_CHK_CONDA_CMD:
            name = L"Anaconda Prompt";
            {
                std::wstring cb = FindCondaBasePath();
                if (!cb.empty())
                {
                    path = sysCmd;
                    args = L"/K \"" + cb + L"\\Scripts\\activate.bat\" \"" + cb + L"\" & cd /D \"{current_dir}\"";
                }
            }
            break;
        case IDC_CHK_VS_X64:
            name = L"VS x64 Native";
            {
                std::wstring bp = FindVsBatch(L"vcvars64.bat");
                if (!bp.empty())
                {
                    path = sysCmd;
                    args = L"/K \"" + bp + L"\" & cd /D \"{current_dir}\"";
                }
            }
            break;
        case IDC_CHK_VS_X86:
            name = L"VS x64_x86 Cross";
            {
                std::wstring bp = FindVsBatch(L"vcvarsamd64_x86.bat");
                if (!bp.empty())
                {
                    path = sysCmd;
                    args = L"/K \"" + bp + L"\" & cd /D \"{current_dir}\"";
                }
            }
            break;
        case IDC_CHK_MSYS2_MINGW64:
            name = L"MSYS2 MinGW x64";
            args = L"-mingw64 -here";
            {
                std::wstring mp = FindMSYS2Path();
                if (!mp.empty())
                    path = mp + L"\\msys2_shell.cmd";
            }
            break;
        case IDC_CHK_MSYS2_MINGW32:
            name = L"MSYS2 MinGW x86";
            args = L"-mingw32 -here";
            {
                std::wstring mp = FindMSYS2Path();
                if (!mp.empty())
                    path = mp + L"\\msys2_shell.cmd";
            }
            break;
        case IDC_CHK_MSYS2_MSYS:
            name = L"MSYS2 MSYS";
            args = L"-msys -here";
            {
                std::wstring mp = FindMSYS2Path();
                if (!mp.empty())
                    path = mp + L"\\msys2_shell.cmd";
            }
            break;
        case IDC_CHK_MSYS2_CLANG64:
            name = L"MSYS2 Clang x64";
            args = L"-clang64 -here";
            {
                std::wstring mp = FindMSYS2Path();
                if (!mp.empty())
                    path = mp + L"\\msys2_shell.cmd";
            }
            break;
        case IDC_CHK_MSYS2_UCRT64:
            name = L"MSYS2 UCRT x64";
            args = L"-ucrt64 -here";
            {
                std::wstring mp = FindMSYS2Path();
                if (!mp.empty())
                    path = mp + L"\\msys2_shell.cmd";
            }
            break;
        case IDC_CHK_CMDER:
            name = L"Cmder";
            path = FindCmderPath();
            break;
        case IDC_CHK_CONEMU:
            name = L"ConEmu";
            path = FindConEmuPath();
            break;
        case IDC_CHK_ZELLIJ:
            name = L"Zellij (WSL)";
            path = ps7Path;
            if (!path.empty())
                args = L"-NoExit -Command \"wsl --cd '{current_dir}' zellij\"";
            break;
        case IDC_CHK_TMUX:
            name = L"Tmux (WSL)";
            path = ps7Path;
            if (!path.empty())
                args = L"-NoExit -Command \"wsl --cd '{current_dir}' tmux\"";
            break;
        case IDC_CHK_WARP:
            name = L"Warp";
            path = FindWarpPath();
            break;
        case IDC_CHK_WSL_PS7:
            name = L"WSL (PowerShell 7)";
            path = ps7Path;
            if (!path.empty())
                args = L"-NoExit -Command \"wsl --cd '{current_dir}'\"";
            break;
        case IDC_CHK_WSL_CMD:
            name = L"WSL (CMD)";
            path = sysCmd;
            args = L"/K wsl --cd \"{current_dir}\"";
            break;
        case IDC_CHK_ZELLIJ_WIN:
            name = L"Zellij";
            path = FindExecutablePath(L"zellij.exe");
            break;
        case IDC_CHK_CODEX_PS:
            name = L"Codex (PowerShell)";
            path = ps7Path;
            if (!path.empty())
                args = L"-NoExit -Command \"cd '{current_dir}'; codex\"";
            break;
        case IDC_CHK_CLAUDE_PS:
            name = L"Claude Code (PowerShell)";
            path = ps7Path;
            if (!path.empty())
                args = L"-NoExit -Command \"cd '{current_dir}'; claude\"";
            break;
        case IDC_CHK_CODEX_WSL:
            name = L"Codex (WSL)";
            path = ps7Path;
            if (!path.empty())
                args = L"-NoExit -Command \"wsl --cd '{current_dir}' codex\"";
            break;
        case IDC_CHK_CLAUDE_WSL:
            name = L"Claude Code (WSL)";
            path = ps7Path;
            if (!path.empty())
                args = L"-NoExit -Command \"wsl --cd '{current_dir}' claude\"";
            break;
        }
        AddTool(name, path, args);
    }
}

bool g_bSearchExecuted = false;

LRESULT CALLBACK SearchDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    // Static variables to track scroll state and content size
    static int scrollPos = 0;
    static int contentHeight = 0;
    static int btnY = 0;

    switch (message) {
        case WM_CREATE: {
            CreateCtrl(L"STATIC", L"Please check the terminals to search automatically:", 0, 0, 20, 15, 400, 20, NULL, hWnd, hGlobalFont);
            int y = 45;
            int id = IDC_CHK_GIT_BASH;
            auto AddChk = [&](LPCWSTR text) {
                CreateCtrl(L"BUTTON", text, BS_AUTOCHECKBOX, 0, 20, y, 400, 24, (HMENU)(INT_PTR)id, hWnd, hGlobalFont);
                y += 28;
                id++;
            };
            AddChk(L"Git Bash");
            AddChk(L"Git CMD");
            AddChk(L"PowerShell");
            AddChk(L"PowerShell 7");
            AddChk(L"Windows Terminal");
            AddChk(L"CMD");
            AddChk(L"Anaconda PowerShell Prompt");
            AddChk(L"Anaconda Prompt");
            AddChk(L"VS x64 Native Tools Cmd");
            AddChk(L"VS x64_x86 Cross Tools Cmd");
            AddChk(L"MSYS2 MinGW x64");
            AddChk(L"MSYS2 MinGW x86");
            AddChk(L"MSYS2 MSYS");
            AddChk(L"MSYS2 MinGW Clang x64");
            AddChk(L"MSYS2 MinGW UCRT x64");
            AddChk(L"Cmder");
            AddChk(L"ConEmu");
            AddChk(L"Zellij (WSL)");
            AddChk(L"Tmux (WSL)");
            AddChk(L"Warp");
            AddChk(L"WSL (PowerShell 7)");
            AddChk(L"WSL (CMD)");
            AddChk(L"Zellij (Windows Native)");
            AddChk(L"Codex (PowerShell)");
            AddChk(L"Claude Code (PowerShell)");
            AddChk(L"Codex (WSL)");
            AddChk(L"Claude Code (WSL)");

            btnY = y + 10;
            CreateCtrl(L"BUTTON", L"Start Search", 0, 0, 20, btnY, 400, 35, (HMENU)(INT_PTR)IDC_BTN_DO_SEARCH, hWnd, hBoldFont);
            
            // Calculate total height of all controls (bottom of button + margin)
            contentHeight = btnY + 35 + 20;
            scrollPos = 0;

            // Setup vertical scrollbar if the window has the WS_VSCROLL style
            DWORD style = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
            if (style & WS_VSCROLL) {
                RECT rcClient;
                GetClientRect(hWnd, &rcClient);
                SCROLLINFO sc = {0};
                sc.cbSize = sizeof(sc);
                sc.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
                sc.nMin = 0;
                sc.nMax = contentHeight;
                sc.nPage = rcClient.bottom;
                sc.nPos = 0;
                SetScrollInfo(hWnd, SB_VERT, &sc, TRUE);
            }
            break;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == IDC_BTN_DO_SEARCH) {
                std::vector<int> targets;
                for (int i = IDC_CHK_GIT_BASH; i <= IDC_CHK_CLAUDE_WSL; i++) {
                    HWND hChk = GetDlgItem(hWnd, i);
                    if (IsWindowVisible(hChk) && SendMessageW(hChk, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                        targets.push_back(i);
                    }
                }
                ExecuteSearch(hWnd, targets);
                g_bSearchExecuted = true;
                DestroyWindow(hWnd);
                return 0;
            }
            break;
        }
        case WM_VSCROLL: {
            // Only process scroll messages if the window actually has a scrollbar
            DWORD style = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
            if (!(style & WS_VSCROLL)) break;

            SCROLLINFO scr = {0};
            scr.cbSize = sizeof(scr);
            scr.fMask = SIF_ALL;
            GetScrollInfo(hWnd, SB_VERT, &scr);

            int yPos = scr.nPos;
            switch (LOWORD(wParam)) {
                case SB_TOP:        scr.nPos = scr.nMin; break;
                case SB_BOTTOM:     scr.nPos = scr.nMax; break;
                case SB_LINEUP:     scr.nPos -= 30; break;
                case SB_LINEDOWN:   scr.nPos += 30; break;
                case SB_PAGEUP:     scr.nPos -= scr.nPage; break;
                case SB_PAGEDOWN:   scr.nPos += scr.nPage; break;
                case SB_THUMBTRACK: scr.nPos = scr.nTrackPos; break;
                default: break;
            }
            
            // Constrain scroll position within valid bounds
            scr.nPos = std::max(0, scr.nPos);
            scr.nPos = std::min(scr.nPos, scr.nMax - (int)scr.nPage);
            
            // Apply the new position
            scr.fMask = SIF_POS;
            SetScrollInfo(hWnd, SB_VERT, &scr, TRUE);
            GetScrollInfo(hWnd, SB_VERT, &scr);
            
            // If the position changed, scroll the window contents
            if (scr.nPos != yPos) {
                ScrollWindow(hWnd, 0, yPos - scr.nPos, NULL, NULL);
                UpdateWindow(hWnd);
                scrollPos = scr.nPos;
            }
            return 0;
        }
        case WM_MOUSEWHEEL: {
            // Support mouse wheel scrolling
            DWORD style = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
            if (!(style & WS_VSCROLL)) break;
            
            int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            if (zDelta < 0) SendMessage(hWnd, WM_VSCROLL, SB_PAGEDOWN, 0);
            else SendMessage(hWnd, WM_VSCROLL, SB_PAGEUP, 0);
            return 0;
        }
        case WM_CLOSE: { DestroyWindow(hWnd); return 0; }
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORDLG: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, RGB(30, 30, 30));
            SetBkMode(hdc, TRANSPARENT);
            return (LRESULT)hBkgBrush;
        }
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

HWND CreateCtrl(LPCWSTR cls, LPCWSTR text, DWORD style, DWORD exStyle, int x, int y, int w, int h, HMENU id, HWND parent, HFONT font) {
    HWND hCtrl = CreateWindowExW(exStyle, cls, text, style | WS_CHILD | WS_VISIBLE, x, y, w, h, parent, id, NULL, NULL);
    SendMessageW(hCtrl, WM_SETFONT, (WPARAM)font, TRUE);
    return hCtrl;
}

// ================== Menu Popup & Direct Invoke Mode ==================

#define ID_MENU_SETTING 9999

void LaunchToolByIndex(int index, HINSTANCE hInstance) {
    if (index == ID_MENU_SETTING) {
        RunConfigGUI(hInstance);
    } else if (index > 0 && index <= (int)g_tools.size()) {
        int toolIdx = index - 1;
        if (!g_tools[toolIdx].path.empty()) { 
            const auto& tool = g_tools[toolIdx];
            fs::path currentDir = fs::current_path();
            std::wstring dirStr = currentDir.wstring();

            std::wstring args = tool.args;
            size_t pos = args.find(L"{current_dir}");
            if (pos != std::wstring::npos) args.replace(pos, 13, dirStr);

            std::wstring cmdLine = L"\"" + tool.path + L"\"" + (args.empty() ? L"" : L" " + args);
        STARTUPINFOW sf = { sizeof(sf) };
            PROCESS_INFORMATION pi;
        CreateProcessW(NULL, &cmdLine[0], NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT, NULL, dirStr.c_str(), &sf, &pi);
            if (pi.hProcess) CloseHandle(pi.hProcess);
            if (pi.hThread) CloseHandle(pi.hThread);
        }
    }
}

void RunMenuMode(HINSTANCE hInstance) {
    LoadConfig();

    HMENU hMenu = CreatePopupMenu();
    int menuCount = 0;
    for (size_t i = 0; i < g_tools.size(); i++) {
        if (!g_tools[i].path.empty()) { 
            menuCount++;
            std::wstring menuText = std::to_wstring(menuCount) + L". " + g_tools[i].name;
            if (!g_tools[i].hotkey.empty()) menuText += L"(&" + g_tools[i].hotkey + L")";
            AppendMenuW(hMenu, MF_STRING, (UINT_PTR)(i + 1), menuText.c_str()); 
        }
    }

    if (menuCount > 0) {
        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    }
    std::wstring settingText = std::to_wstring(menuCount + 1) + L". &Setting";
    AppendMenuW(hMenu, MF_STRING, ID_MENU_SETTING, settingText.c_str()); 

    POINT pt;
    GetCursorPos(&pt);
    
    HWND hWnd = CreateWindowExW(0, L"Static", L"", 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    SetForegroundWindow(hWnd); 
    int cmdId = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
    DestroyWindow(hWnd);

    if (cmdId > 0) LaunchToolByIndex(cmdId, hInstance);
    DestroyMenu(hMenu);
}

// ================== Modern GUI Config Interface ==================

#define IDC_LIST_ITEMS 101
#define IDC_EDIT_NAME 102
#define IDC_EDIT_PATH 103
#define IDC_EDIT_ARGS 104
#define IDC_EDIT_HOTKEY 105
#define IDC_BTN_ADD 106
#define IDC_BTN_DEL 107
#define IDC_BTN_SEARCH 108
#define IDC_BTN_INJECT 109
#define IDC_BTN_UNINJECT 110
#define IDC_BTN_SAVE 111
#define IDC_BTN_UP 112
#define IDC_BTN_DOWN 113
#define IDC_BTN_OTHER1 114
#define IDC_BTN_OTHER2 115

HWND hList, hEditName, hEditPath, hEditArgs, hEditHotkey;

void RefreshListBox() {
    int selListBoxIdx = (int)SendMessageW(hList, LB_GETCURSEL, 0, 0);
    int savedToolIdx = -1;
    if (selListBoxIdx != LB_ERR && selListBoxIdx < (int)g_listBoxMap.size()) {
        savedToolIdx = g_listBoxMap[selListBoxIdx];
    }

    SendMessageW(hList, LB_RESETCONTENT, 0, 0);
    g_listBoxMap.clear();

    int number = 1;
    for (size_t i = 0; i < g_tools.size(); i++) {
        if (!g_tools[i].path.empty()) {
            std::wstring itemText = std::to_wstring(number) + L". " + g_tools[i].name;
            SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)itemText.c_str());
            g_listBoxMap.push_back((int)i);
            number++;
        }
    }

    bool hasPending = false;
    for (size_t i = 0; i < g_tools.size(); i++) {
        if (g_tools[i].path.empty()) { hasPending = true; break; }
    }

    if (hasPending) {
        SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)L"---------- Pending (Requires Manual Path) ----------");
        g_listBoxMap.push_back(-1);

        for (size_t i = 0; i < g_tools.size(); i++) {
            if (g_tools[i].path.empty()) {
                std::wstring itemText = L"   " + g_tools[i].name;
                SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)itemText.c_str());
                g_listBoxMap.push_back((int)i);
            }
        }
    }

    if (savedToolIdx != -1) {
        for (size_t i = 0; i < g_listBoxMap.size(); i++) {
            if (g_listBoxMap[i] == savedToolIdx) {
                SendMessageW(hList, LB_SETCURSEL, i, 0);
                break;
            }
        }
    }
}

void LoadItemToEdits(int toolIndex) {
    if (toolIndex < 0 || toolIndex >= (int)g_tools.size()) return;
    SetWindowTextW(hEditName, g_tools[toolIndex].name.c_str());
    SetWindowTextW(hEditPath, g_tools[toolIndex].path.c_str());
    SetWindowTextW(hEditArgs, g_tools[toolIndex].args.c_str());
    SetWindowTextW(hEditHotkey, g_tools[toolIndex].hotkey.c_str());
}

void UpdateItemFromEdits(int toolIndex) {
    if (toolIndex < 0 || toolIndex >= (int)g_tools.size()) return;
    wchar_t buf[2048];
    GetWindowTextW(hEditName, buf, 2048); g_tools[toolIndex].name = buf;
    GetWindowTextW(hEditPath, buf, 2048); g_tools[toolIndex].path = buf;
    GetWindowTextW(hEditArgs, buf, 2048); g_tools[toolIndex].args = buf;
    GetWindowTextW(hEditHotkey, buf, 2048); g_tools[toolIndex].hotkey = buf;
}

LRESULT CALLBACK ConfigWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            hGlobalFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            hBoldFont = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            hBkgBrush = CreateSolidBrush(RGB(245, 245, 245));

            CreateCtrl(L"STATIC", L"RunIt Config Panel", 0, 0, 20, 15, 300, 30, NULL, hWnd, hBoldFont);
            
            CreateCtrl(L"STATIC", L"Menu Item List (Support sorting):", 0, 0, 20, 55, 200, 20, NULL, hWnd, hGlobalFont);
            hList = CreateCtrl(L"LISTBOX", L"", WS_BORDER | LBS_NOTIFY | WS_VSCROLL | LBS_HASSTRINGS, WS_EX_CLIENTEDGE, 20, 80, 340, 310, (HMENU)(INT_PTR)IDC_LIST_ITEMS, hWnd, hGlobalFont);
            
            CreateCtrl(L"BUTTON", L"Up", 0, 0, 20, 400, 160, 35, (HMENU)(INT_PTR)IDC_BTN_UP, hWnd, hGlobalFont);
            CreateCtrl(L"BUTTON", L"Down", 0, 0, 200, 400, 160, 35, (HMENU)(INT_PTR)IDC_BTN_DOWN, hWnd, hGlobalFont);

            CreateCtrl(L"STATIC", L"Name:", 0, 0, 380, 85, 80, 20, NULL, hWnd, hGlobalFont);
            hEditName = CreateCtrl(L"EDIT", L"", WS_BORDER | ES_AUTOVSCROLL, WS_EX_CLIENTEDGE, 470, 80, 290, 28, (HMENU)(INT_PTR)IDC_EDIT_NAME, hWnd, hGlobalFont);
            
            CreateCtrl(L"STATIC", L"Full Path:", 0, 0, 380, 125, 80, 20, NULL, hWnd, hGlobalFont);
            hEditPath = CreateCtrl(L"EDIT", L"", WS_BORDER | ES_AUTOVSCROLL, WS_EX_CLIENTEDGE, 470, 120, 290, 28, (HMENU)(INT_PTR)IDC_EDIT_PATH, hWnd, hGlobalFont);
            
            CreateCtrl(L"STATIC", L"Launch Args:", 0, 0, 380, 165, 80, 20, NULL, hWnd, hGlobalFont);
            hEditArgs = CreateCtrl(L"EDIT", L"", WS_BORDER | ES_AUTOVSCROLL, WS_EX_CLIENTEDGE, 470, 160, 290, 28, (HMENU)(INT_PTR)IDC_EDIT_ARGS, hWnd, hGlobalFont);

            CreateCtrl(L"STATIC", L"Hotkey:", 0, 0, 380, 205, 80, 20, NULL, hWnd, hGlobalFont);
            hEditHotkey = CreateCtrl(L"EDIT", L"", WS_BORDER | ES_AUTOVSCROLL, WS_EX_CLIENTEDGE, 470, 200, 50, 28, (HMENU)(INT_PTR)IDC_EDIT_HOTKEY, hWnd, hGlobalFont);
            CreateCtrl(L"STATIC", L"(Single letter/number, optional)", 0, 0, 530, 205, 200, 20, NULL, hWnd, hGlobalFont);

            CreateCtrl(L"BUTTON", L"Add / Update", 0, 0, 380, 250, 180, 38, (HMENU)(INT_PTR)IDC_BTN_ADD, hWnd, hGlobalFont);
            CreateCtrl(L"BUTTON", L"Delete Selected", 0, 0, 580, 250, 180, 38, (HMENU)(INT_PTR)IDC_BTN_DEL, hWnd, hGlobalFont);

            // New layout for System PATH buttons
            CreateCtrl(L"BUTTON", L"Auto Search Terminals", 0, 0, 380, 290, 380, 35, (HMENU)(INT_PTR)IDC_BTN_SEARCH, hWnd, hGlobalFont);
            CreateCtrl(L"BUTTON", L"Add System PATH", 0, 0, 380, 330, 180, 35, (HMENU)(INT_PTR)IDC_BTN_INJECT, hWnd, hGlobalFont);
            CreateCtrl(L"BUTTON", L"Remove System PATH", 0, 0, 580, 330, 180, 35, (HMENU)(INT_PTR)IDC_BTN_UNINJECT, hWnd, hGlobalFont);
            
            // Adjusted Save button position
            CreateCtrl(L"BUTTON", L"Save Config Apply", 0, 0, 380, 375, 380, 60, (HMENU)(INT_PTR)IDC_BTN_SAVE, hWnd, hBoldFont);

            LoadConfig();
            RefreshListBox();
            break;
        }
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, RGB(30, 30, 30));
            SetBkMode(hdc, TRANSPARENT);
            return (LRESULT)hBkgBrush;
        }
        case WM_CTLCOLORDLG:
        case WM_ERASEBKGND: {
            HDC hdc = (HDC)wParam;
            RECT rc;
            GetClientRect(hWnd, &rc);
            FillRect(hdc, &rc, hBkgBrush);
            return 1;
        }
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            int selListBoxIdx = (int)SendMessageW(hList, LB_GETCURSEL, 0, 0);
            int toolIdx = -1;
            if (selListBoxIdx != LB_ERR && selListBoxIdx < (int)g_listBoxMap.size()) {
                toolIdx = g_listBoxMap[selListBoxIdx];
            }

            switch (wmId) {
                case IDC_LIST_ITEMS: if (HIWORD(wParam) == LBN_SELCHANGE) { if (toolIdx != -1) LoadItemToEdits(toolIdx); else { SetWindowTextW(hEditName, L""); SetWindowTextW(hEditPath, L""); SetWindowTextW(hEditArgs, L""); SetWindowTextW(hEditHotkey, L""); } } break;
                case IDC_BTN_UP: if (toolIdx > 0) { std::swap(g_tools[toolIdx], g_tools[toolIdx - 1]); RefreshListBox(); for (size_t i = 0; i < g_listBoxMap.size(); i++) { if (g_listBoxMap[i] == toolIdx - 1) { SendMessageW(hList, LB_SETCURSEL, i, 0); break; } } } break;
                case IDC_BTN_DOWN: if (toolIdx != -1 && toolIdx < (int)g_tools.size() - 1) { std::swap(g_tools[toolIdx], g_tools[toolIdx + 1]); RefreshListBox(); for (size_t i = 0; i < g_listBoxMap.size(); i++) { if (g_listBoxMap[i] == toolIdx + 1) { SendMessageW(hList, LB_SETCURSEL, i, 0); break; } } } break;
                case IDC_BTN_ADD: if (toolIdx == -1) { wchar_t buf[1024]; GetWindowTextW(hEditName, buf, 1024); if (wcslen(buf) == 0) { MessageBoxW(hWnd, L"Name cannot be empty!", L"Notice", MB_OK | MB_ICONWARNING); break; } g_tools.push_back({buf, L"", L"", L""}); toolIdx = g_tools.size() - 1; UpdateItemFromEdits(toolIdx); } else { UpdateItemFromEdits(toolIdx); } RefreshListBox(); for (size_t i = 0; i < g_listBoxMap.size(); i++) { if (g_listBoxMap[i] == toolIdx) { SendMessageW(hList, LB_SETCURSEL, i, 0); break; } } break;
                case IDC_BTN_DEL: if (toolIdx != -1) { g_tools.erase(g_tools.begin() + toolIdx); RefreshListBox(); SetWindowTextW(hEditName, L""); SetWindowTextW(hEditPath, L""); SetWindowTextW(hEditArgs, L""); SetWindowTextW(hEditHotkey, L""); } break;
                case IDC_BTN_SEARCH: {
                    WNDCLASSW wc = {0};
                    wc.lpfnWndProc = SearchDlgProc;
                    wc.hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
                    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
                    wc.hbrBackground = hBkgBrush;
                    wc.lpszClassName = L"RunItSearchDlg";
                    RegisterClassW(&wc);
                    
                    EnableWindow(hWnd, FALSE);
                    g_bSearchExecuted = false;

                    // Dynamically calculate max window height based on screen work area
                    RECT rcWorkArea;
                    SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0);
                    int maxScreenHeight = rcWorkArea.bottom - rcWorkArea.top - 50;
                    int idealHeight = 900;
                    int finalHeight = idealHeight;
                    DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;

                    // Add vertical scrollbar if screen is too small
                    if (idealHeight > maxScreenHeight) {
                        finalHeight = maxScreenHeight;
                        dwStyle |= WS_VSCROLL;
                    }

                    HWND hDlg = CreateWindowExW(0, L"RunItSearchDlg", L"Auto Search Terminals", dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, 460, finalHeight, hWnd, NULL, wc.hInstance, NULL);
                    ShowWindow(hDlg, SW_SHOW);
                    SetForegroundWindow(hDlg);
                    MSG msg;
                    while (IsWindow(hDlg) && GetMessage(&msg, NULL, 0, 0)) {
                        if (!IsDialogMessageW(hDlg, &msg)) {
                            TranslateMessage(&msg);
                            DispatchMessage(&msg);
                        }
                    }
                    EnableWindow(hWnd, TRUE);

                    // Unbreakable Foreground Hack: AttachThreadInput
                    DWORD dwCurrentThread = GetCurrentThreadId();
                    DWORD dwFGThread = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
                    if (dwFGThread != dwCurrentThread) {
                        AttachThreadInput(dwCurrentThread, dwFGThread, TRUE);
                    }
                    SetForegroundWindow(hWnd);
                    SetActiveWindow(hWnd);
                    SetFocus(hWnd);
                    if (dwFGThread != dwCurrentThread) {
                        AttachThreadInput(dwCurrentThread, dwFGThread, FALSE);
                    }

                    RefreshListBox();
                    
                    if (g_bSearchExecuted) {
                        MessageBoxW(hWnd, L"Search complete! Checked items have been added to the list.\n(Items not found are marked as pending, please supplement the path in the main interface)", L"Notice", MB_OK | MB_ICONINFORMATION);
                    }
                    break;
                }
                case IDC_BTN_INJECT: if (AddToSystemPath()) MessageBoxW(hWnd, L"Successfully add the tool directory into the system PATH environment variable!", L"Success", MB_OK | MB_ICONINFORMATION); else MessageBoxW(hWnd, L"Add to system PATH failed, please check permissions.", L"Error", MB_OK | MB_ICONERROR); break;
                // Handle Remove System PATH
                case IDC_BTN_UNINJECT: if (RemoveFromSystemPath()) MessageBoxW(hWnd, L"Successfully removed the tool directory from the system PATH environment variable!", L"Success", MB_OK | MB_ICONINFORMATION); else MessageBoxW(hWnd, L"Removal failed, or the tool directory was not found in PATH.", L"Notice", MB_OK | MB_ICONWARNING);break;
                case IDC_BTN_SAVE: if (toolIdx != -1) UpdateItemFromEdits(toolIdx); SaveConfig(); MessageBoxW(hWnd, L"Config saved successfully to config.ini!", L"Save Success", MB_OK | MB_ICONINFORMATION); break;
            }
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void RunConfigGUI(HINSTANCE hInstance) {
    const wchar_t* CLASS_NAME = L"RunItConfigWnd";
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = ConfigWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = hBkgBrush;
    wc.lpszClassName = CLASS_NAME;
    RegisterClassW(&wc);

    HWND hWnd = CreateWindowExW(0, CLASS_NAME, L"RunIt Settings", 
                                WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, 
                                CW_USEDEFAULT, CW_USEDEFAULT, 790, 520, NULL, NULL, hInstance, NULL);
    ShowWindow(hWnd, SW_SHOW);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// ================== Win32 Entry Point ==================

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    g_exeDir = GetExeDir();
    
    std::wstring args = pCmdLine;
    while (!args.empty() && (args.front() == L' ' || args.front() == L'\t')) args.erase(args.begin());
    while (!args.empty() && (args.back() == L' ' || args.back() == L'\t')) args.pop_back();

    if (args == L"/config" || args == L"-config") { RunConfigGUI(hInstance); return 0; }

    bool isNumeric = !args.empty();
    for (wchar_t c : args) {
        if (c < L'0' || c > L'9') { isNumeric = false; break; }
    }

    if (isNumeric) {
        LoadConfig();
        int index = std::stoi(args);
        LaunchToolByIndex(index, hInstance);
    } else {
        RunMenuMode(hInstance);
    }
    return 0;
}


#else
// ================== POSIX (Linux/Mac) Includes ==================
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <climits>
#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <termios.h>
#include <cstdio>
#include <cerrno>
#include <ctime>
#include <csignal>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

extern char **environ;

namespace fs = std::filesystem;

// ================== Data Structures ==================
struct ToolItem {
    std::string name;
    std::string path;
    std::string args;
    std::string shell;
    std::string hotkey;
    std::string type;  // "GUI", "SHELL", "MULTIPLEXER"
};

std::vector<ToolItem> g_tools;
fs::path g_exeDir;

// ================== Environment Awareness ==================
bool IsSSHSession() {
    return getenv("SSH_CONNECTION") != nullptr || getenv("SSH_CLIENT") != nullptr;
}

bool HasDisplay() {
#if defined(__APPLE__)
    const char* termProgram = getenv("TERM_PROGRAM");
    if (termProgram) {
        std::string p(termProgram);
        if (p == "Apple_Terminal" || p == "iTerm.app" || p == "VSCode" || p == "Hyper") return true;
    }
    return false;
#else
    return getenv("DISPLAY") != nullptr || getenv("WAYLAND_DISPLAY") != nullptr;
#endif
}

// ================== Helper Functions ==================
fs::path GetExeDir() {
#if defined(__APPLE__)
    char buf[PATH_MAX];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) == 0) {
        return fs::path(buf).parent_path();
    }
#else
    char buf[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len != -1) {
        buf[len] = '\0';
        return fs::path(buf).parent_path();
    }
#endif
    return fs::current_path();
}

fs::path GetConfigPath() {
    fs::path exeConfig = g_exeDir / "config.ini";
    if (fs::exists(exeConfig)) return exeConfig;
    
    const char* home = getenv("HOME");
    if (home) {
        fs::path homeConfig = fs::path(home) / ".config" / "RunIn" / "config.ini";
        if (fs::exists(homeConfig)) return homeConfig;
    }
    return exeConfig;
}

std::vector<std::string> SplitArgs(const std::string& s) {
    std::vector<std::string> args;
    std::string current;
    bool inQuotes = false;
    for (char c : s) {
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (std::isspace(c) && !inQuotes) {
            if (!current.empty()) {
                args.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        args.push_back(current);
    }
    return args;
}

std::string FindExecutablePathPosix(const std::string& name) {
    std::string cmd = "which " + name + " 2>/dev/null";
    FILE* fp = popen(cmd.c_str(), "r");
    if (fp) {
        char buf[PATH_MAX];
        if (fgets(buf, sizeof(buf), fp) != nullptr) {
            std::string path = buf;
            path.erase(path.find_last_not_of(" \n\r\t") + 1);
            pclose(fp);
            return path;
        }
        pclose(fp);
    }
    return "";
}

// ================== Config File R/W ==================
void LoadConfig() {
    g_tools.clear();
    fs::path configPath = GetConfigPath();
    if (!fs::exists(configPath)) return;
    
    std::ifstream ifs(configPath);
    std::string line;
    ToolItem currentTool;
    bool inItem = false;
    
    while (std::getline(ifs, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line.empty() || line[0] == '#') continue;
        
        if (!line.empty() && line.front() == '[' && line.back() == ']') {
            std::string section = line.substr(1, line.size() - 2);
            if (section.rfind("Item", 0) == 0) {
                if (inItem) g_tools.push_back(currentTool);
                currentTool = ToolItem();
                inItem = true;
            } else {
                if (inItem) {
                    g_tools.push_back(currentTool);
                    inItem = false;
                }
            }
        } else if (inItem) {
            auto eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                std::string key = line.substr(0, eqPos);
                std::string val = line.substr(eqPos + 1);
                if (key == "Name") currentTool.name = val;
                else if (key == "Path") currentTool.path = val;
                else if (key == "Args") currentTool.args = val;
                else if (key == "Shell") currentTool.shell = val;
                else if (key == "Type") currentTool.type = val;
                else if (key == "Hotkey") currentTool.hotkey = val;
            }
        }
    }
    if (inItem) g_tools.push_back(currentTool);
}

void SaveConfig() {
    fs::path configPath = GetConfigPath();
    fs::create_directories(configPath.parent_path());
    std::ofstream ofs(configPath);
    if (!ofs.is_open()) return;
    
    ofs << "# RunIt Config File\n";
    ofs << "# Format: [ItemX]\nName=...\nPath=...\nArgs=...\nShell=...\nType=...\nHotkey=...\n\n";
    for (size_t i = 0; i < g_tools.size(); i++) {
        ofs << "[Item" << (i + 1) << "]\n";
        ofs << "Name=" << g_tools[i].name << "\n";
        ofs << "Path=" << g_tools[i].path << "\n";
        ofs << "Args=" << g_tools[i].args << "\n";
        ofs << "Shell=" << g_tools[i].shell << "\n";
        ofs << "Type=" << g_tools[i].type << "\n";
        ofs << "Hotkey=" << g_tools[i].hotkey << "\n\n";
    }
}

// ================== Tool Launch Logic ==================

// ================== TUI Core Logic ==================
struct termios g_oldTermios;

// Global flag to track if we are currently in the alternate screen buffer.
// Used by the signal handler to ensure safe exit.
bool g_inAltScreen = false;

// Signal handler to gracefully exit on interruptions (e.g., Ctrl+C, termination).
void handle_signal(int sig) {
    // If we are in the alternate screen, switch back to the main screen buffer.
    if (g_inAltScreen) {
        std::cout << "\033[?1049l" << std::flush;
    }
    
    // Restore the original terminal attributes (disable raw mode).
    tcsetattr(STDIN_FILENO, TCSANOW, &g_oldTermios);
    
    // Use _exit() for immediate termination to ensure async-signal-safety.
    // Avoids calling non-async-signal-safe functions (like destructors or std::cout internals) inside the handler.
    _exit(sig); 
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &g_oldTermios);
    termios newTermios = g_oldTermios;
    newTermios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newTermios);
}

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &g_oldTermios);
}

void LaunchToolByIndex(int index) {
    if (index > 0 && index <= (int)g_tools.size()) {
        int toolIdx = index - 1;
        const auto& tool = g_tools[toolIdx];
        if (!tool.path.empty()) {
            fs::path currentDir = fs::current_path();
            std::string dirStr = currentDir.string();
            
            std::string cmdPath = tool.path;
            std::string argsStr = tool.args;
            
            size_t pos = argsStr.find("{current_dir}");
            if (pos != std::string::npos) {
                argsStr.replace(pos, 13, "\"" + dirStr + "\"");
            }
            
            chdir(dirStr.c_str());
            
            std::vector<std::string> argStrs;
            argStrs.push_back(cmdPath);
            for (const auto& a : SplitArgs(argsStr)) {
                argStrs.push_back(a);
            }
            
            if (!tool.shell.empty()) {
                std::string termName = fs::path(cmdPath).filename().string();
                if (termName == "gnome-terminal" || termName == "xfce4-terminal" || termName == "mate-terminal") {
                    argStrs.push_back("--");
                    argStrs.push_back(tool.shell);
                } else if (termName == "konsole" || termName == "xterm" || termName == "alacritty" || termName == "kitty") {
                    argStrs.push_back("-e");
                    argStrs.push_back(tool.shell);
                }
            }
            
            std::vector<char*> argv;
            for (auto& s : argStrs) argv.push_back(&s[0]);
            argv.push_back(nullptr);
            
            std::string type = tool.type;
            // Backward compatibility: infer type if not set
            if (type.empty()) {
                std::string n = tool.name;
                std::transform(n.begin(), n.end(), n.begin(), ::tolower);
                if (n.find("bash") != std::string::npos || n.find("zsh") != std::string::npos || n.find("fish") != std::string::npos) {
                    type = "SHELL";
                } else if (n.find("tmux") != std::string::npos || n.find("zellij") != std::string::npos) {
                    type = "MULTIPLEXER";
                } else {
                    type = "GUI";
                }
            }
            
            if (type == "SHELL" || type == "MULTIPLEXER" || type == "TUI") {

                // Register signal handlers in the parent process to prevent crashes
                std::signal(SIGINT, handle_signal);// Ctrl+C
                std::signal(SIGTERM, handle_signal);// kill command
                std::signal(SIGHUP, handle_signal);// kill command
                // Enter alternate screen buffer. This saves the current screen and cursor position,
                // providing a clean empty screen for the new shell. When the shell exits,
                // we will revert to the main buffer, perfectly restoring the original shell's output.
                // position cursor at top-left and clear screen
                std::cout << "\033[?1049h\033[H\033[2J" << std::flush;
                
                // Get current system time for the welcome prompt
                std::time_t now = std::time(nullptr);
                char timeBuf[64];
                std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
                
                // Print welcome message with tool name, time, and directory
                fs::path currentDir = fs::current_path();
                std::cout << "\033[1;36m" 
                          << "RunIt: " << tool.name << " started at " << currentDir.string() 
                          << " on " << timeBuf << ". Type 'exit' to return."
                          << "\033[0m\n\n" << std::flush;
                
                g_inAltScreen = true;
                
                pid_t pid = fork();
                if (pid == 0) {
                    // Child process
                    execvp(cmdPath.c_str(), argv.data());
                    // If exec fails
                    std::cerr << "Failed to exec " << cmdPath << ": " << strerror(errno) << std::endl;
                    exit(1);
                } else if (pid > 0) {
                    // Parent process
                    int status;
                    // Wait for the child shell to exit
                    waitpid(pid, &status, 0);
                    // Exit alternate screen buffer (restores original screen)
                    std::cout << "\033[?1049l" << std::flush;
                } else {
                    std::cerr << "Failed to fork: " << strerror(errno) << std::endl;
                    std::cout << "\033[?1049l" << std::flush;
                }
            } else {
                // GUI apps can be spawned asynchronously
                pid_t pid;
                int spawnResult;
                if (cmdPath.find('/') != std::string::npos) {
                    spawnResult = posix_spawn(&pid, cmdPath.c_str(), NULL, NULL, argv.data(), environ);
                } else {
                    spawnResult = posix_spawnp(&pid, cmdPath.c_str(), NULL, NULL, argv.data(), environ);
                }
                if (spawnResult != 0) {
                    std::cerr << "Failed to launch " << cmdPath << ": " << strerror(spawnResult) << std::endl;
                }
            }
        }
    }
}

int getKey() {
    char c;
    if (read(STDIN_FILENO, &c, 1) <= 0) return -1;
    if (c == '\033') {
        char seq[2];
        if (read(STDIN_FILENO, &seq[0], 1) <= 0) return '\033';
        if (read(STDIN_FILENO, &seq[1], 1) <= 0) return '\033';
        if (seq[0] == '[') {
            if (seq[1] == 'A') return 1;
            if (seq[1] == 'B') return 2;
        }
        return '\033';
    }
    return c;
}

// TUI rendering: in-place re-render, preserves scrollback history
// Uses "# " prefix for non-selectable category separators
int showMenu(const std::string& title, const std::vector<std::string>& options) {
    int selected = 1;
    int numSelectable = 0;
    for(auto& opt : options) if (opt.rfind("# ", 0) != 0) numSelectable++;
    
    enableRawMode();
    int printedLines = 0;
    
    auto render = [&]() {
        if (printedLines > 0) {
            std::cout << "\033[" << printedLines << "A";
            for(int i=0; i<printedLines; i++) std::cout << "\r\033[K\n";
            std::cout << "\033[" << printedLines << "A";
        }
        printedLines = 0;
        
        std::string titleHint = (numSelectable <= 9) ? " (Up/Down, 1-9, Enter, ESC to cancel)" : " (Up/Down, Enter, ESC to cancel)";
        std::cout << title << titleHint << "\n"; printedLines++;
        std::cout << "------------------------------------------------\n"; printedLines++;
        
        int idx = 0;
        for (const auto& opt : options) {
            if (opt.rfind("# ", 0) == 0) {
                std::cout << opt.substr(2) << "\n";
            } else {
                idx++;
                std::string label = std::to_string(idx) + ". " + opt;
                if (idx == selected) {
                    std::cout << "\033[7m> " << label << "\033[0m";
                } else {
                    std::cout << "  " << label;
                }
                std::cout << "\n";
            }
            printedLines++;
        }
        std::cout << "------------------------------------------------\n"; printedLines++;
        std::cout.flush();
    };
    
    render();
    while (true) {
        int key = getKey();
        if (key == 2) { selected++; if (selected > numSelectable) selected = 1; render(); }
        else if (key == 1) { selected--; if (selected < 1) selected = numSelectable; render(); }
        else if (key == '\n' || key == '\r') { disableRawMode(); return selected; }
        else if (key == '\033') { disableRawMode(); return -1; }
        else if (numSelectable <= 9 && key >= '1' && key <= '9') {
            int num = key - '0';
            if (num <= numSelectable) {
                disableRawMode();
                return num;
            }
        }
    }
}

std::string promptString(const std::string& prompt, const std::string& def) {
    std::string input = def;
    int printedLines = 0;
    enableRawMode();
    
    auto render = [&]() {
        if (printedLines > 0) {
            std::cout << "\033[" << printedLines << "A";
            for(int i=0; i<printedLines; i++) std::cout << "\r\033[K\n";
            std::cout << "\033[" << printedLines << "A";
        }
        printedLines = 0;
        std::cout << prompt << " (ESC to keep default)\n"; printedLines++;
        std::cout << "------------------------------------------------\n"; printedLines++;
        std::cout << ">> " << input << "\n"; printedLines++;
        std::cout << "------------------------------------------------\n"; printedLines++;
        std::cout.flush();
    };
    
    render();
    while (true) {
        int key = getKey();
        if (key == '\n' || key == '\r') { disableRawMode(); return input; }
        else if (key == 127 || key == '\b') { if (!input.empty()) input.pop_back(); render(); }
        else if (key == '\033') { disableRawMode(); return def; }
        else if (key >= 32 && key <= 126) { input += (char)key; render(); }
    }
}

// ================== TUI Settings Logic ==================
void AddToolTUI() {
    // Fixed to exactly 9 selectable items to support 1-9 shortcuts!
    std::vector<std::string> templates = {
        "# ----------------- Terminals -----------------",
        "GNOME Terminal (Linux GUI)",
        "Konsole (KDE Linux GUI)",
        "Mac Terminal (macOS GUI)",
        "iTerm2 (macOS GUI)",
        "# ------------------ Shells ------------------",
        "Zsh Shell (Interactive Shell)",
        "Bash Shell (Interactive Shell)",
        "Fish Shell (Interactive Shell)",
        "# --------------- Multiplexers ---------------",
        "Tmux (TUI Multiplexer - SSH OK!)",
        "Zellij (TUI Multiplexer - SSH OK!)"
    };
    int choice = showMenu("Select Tool Template", templates);
    if (choice == -1) return;

    std::string name = "", path = "", args = "", shell = "", hotkey = "";
    std::string type = "GUI";
    
    if (choice == 1) { name = "GNOME Terminal"; path = FindExecutablePathPosix("gnome-terminal"); args = "--working-directory=\"{current_dir}\""; type = "GUI"; }
    else if (choice == 2) { name = "Konsole"; path = FindExecutablePathPosix("konsole"); args = "--workdir \"{current_dir}\""; type = "GUI"; }
    else if (choice == 3) { name = "Mac Terminal"; path = "/System/Applications/Terminal.app/Contents/MacOS/Terminal"; args = "\"{current_dir}\""; type = "GUI"; }
    else if (choice == 4) { name = "iTerm2"; path = "/Applications/iTerm.app/Contents/MacOS/iTerm2"; args = "\"{current_dir}\""; type = "GUI"; }
    else if (choice == 5) { name = "Zsh"; path = FindExecutablePathPosix("zsh"); args = ""; type = "SHELL"; }
    else if (choice == 6) { name = "Bash"; path = FindExecutablePathPosix("bash"); args = ""; type = "SHELL"; }
    else if (choice == 7) { name = "Fish"; path = FindExecutablePathPosix("fish"); args = ""; type = "SHELL"; }
    else if (choice == 8) { name = "Tmux"; path = FindExecutablePathPosix("tmux"); args = "new-session -s work"; type = "MULTIPLEXER"; }
    else if (choice == 9) { name = "Zellij"; path = FindExecutablePathPosix("zellij"); args = ""; type = "MULTIPLEXER"; }

    name = promptString("Confirm/Enter Name", name);
    if (name.empty()) return;
    path = promptString("Confirm/Enter Path", path);
    if (path.empty()) return;
    args = promptString("Confirm/Enter Args", args);
    shell = promptString("Confirm/Enter Inner Shell", shell);
    type = promptString("Confirm/Enter Type (GUI/SHELL/MULTIPLEXER)", type);
    hotkey = promptString("Confirm/Enter Hotkey", hotkey);

    g_tools.push_back({name, path, args, shell, hotkey, type});
    SaveConfig();
}

void ModifyToolTUI() {
    if (g_tools.empty()) {
        showMenu("Notice", {"No tools to modify. Press Enter..."});
        return;
    }
    std::vector<std::string> names;
    for (const auto& t : g_tools) names.push_back(t.name + "  [" + t.path + "]");

    int choice = showMenu("Select Tool to Modify", names);
    if (choice == -1) return;

    int idx = choice - 1;
    std::string name = promptString("Modify Name", g_tools[idx].name);
    std::string path = promptString("Modify Path", g_tools[idx].path);
    std::string args = promptString("Modify Args", g_tools[idx].args);
    std::string shell = promptString("Modify Inner Shell", g_tools[idx].shell);
    std::string type = promptString("Modify Type (GUI/SHELL/MULTIPLEXER)", g_tools[idx].type.empty() ? "GUI" : g_tools[idx].type);
    std::string hotkey = promptString("Modify Hotkey", g_tools[idx].hotkey);

    g_tools[idx] = {name, path, args, shell, hotkey, type};
    SaveConfig();
}

void RunTuiConfig() {
    while (true) {
        int action = showMenu("Setting Menu", {"Add Tool", "Modify Tool", "Back"});
        if (action == 1) AddToolTUI();
        else if (action == 2) ModifyToolTUI();
        else break;
    }
}

// ================== Main Menu & Entry Point ==================
int main(int argc, char* argv[]) {
    g_exeDir = GetExeDir();
    LoadConfig();
    
    int autoSelect = -1;
    if (argc > 1) {
        std::string arg = argv[1];
        bool isNumeric = !arg.empty();
        for (char c : arg) {
            if (c < '0' || c > '9') { isNumeric = false; break; }
        }
        if (arg == "config" || arg == "/config" || arg == "-config" || arg == "setting" || arg == "/setting") {
            RunTuiConfig();
            return 0;
        } else if (isNumeric) {
            autoSelect = std::stoi(arg);
        }
    }
    
    bool isSSH = IsSSHSession();

    while (true) {
        std::vector<int> terminalTools, shellTools;
        
        for (size_t i = 0; i < g_tools.size(); i++) {
            if (!g_tools[i].path.empty()) {
                std::string type = g_tools[i].type;
                // Infer type for backward compatibility
                if (type.empty()) {
                    std::string n = g_tools[i].name;
                    std::transform(n.begin(), n.end(), n.begin(), ::tolower);
                    if (n.find("bash") != std::string::npos || n.find("zsh") != std::string::npos || n.find("fish") != std::string::npos) {
                        type = "SHELL";
                    } else if (n.find("tmux") != std::string::npos || n.find("zellij") != std::string::npos) {
                        type = "MULTIPLEXER";
                    } else {
                        type = "GUI";
                    }
                }
                
                // Environment filtering: Hide GUI terminals in ANY SSH session
                if (isSSH && type == "GUI") continue;
                
                // Grouping: Shells and Multiplexers go to the available list
                if (type == "SHELL" || type == "MULTIPLEXER" || type == "TUI") {
                    shellTools.push_back(i);
                } else {
                    terminalTools.push_back(i);
                }
            }
        }
        
        // Build validIndices with separator markers
        std::vector<int> validIndices;
        if (!terminalTools.empty()) {
            validIndices.push_back(-1); // Terminal separator
            for(int i : terminalTools) validIndices.push_back(i);
        }
        if (!shellTools.empty()) {
            validIndices.push_back(-2); // Shell separator
            for(int i : shellTools) validIndices.push_back(i);
        }
        
        int numSelectable = terminalTools.size() + shellTools.size();
        
        // Handle command line autoSelect
        if (autoSelect > 0) {
            if (autoSelect <= numSelectable) {
                int actualIdx = -1;
                int count = 0;
                for(int idx : validIndices) {
                    if (idx >= 0) {
                        count++;
                        if (count == autoSelect) {
                            actualIdx = idx;
                            break;
                        }
                    }
                }
                if (actualIdx != -1) {
                    LaunchToolByIndex(actualIdx + 1);
                    return 0;
                }
            } else if (autoSelect == numSelectable + 1) {
                RunTuiConfig();
                return 0;
            }
            autoSelect = -1; // Invalid, fallthrough to interactive
        }
        
        int selected = 1;
        int printedLines = 0;
        
        auto renderMenu = [&]() {
            if (printedLines > 0) {
                std::cout << "\033[" << printedLines << "A";
                for(int i=0; i<printedLines; i++) std::cout << "\r\033[K\n";
                std::cout << "\033[" << printedLines << "A";
            }
            printedLines = 0;
            
            std::string titleHint = (numSelectable + 1 <= 9) ? " (Up/Down, 1-9, Enter, ESC to cancel)" : " (Up/Down, Enter, ESC to cancel)";
            std::cout << "RunIt - Select Environment" << titleHint << ":\n"; printedLines++;
            std::cout << "------------------------------------------------\n"; printedLines++;
            
            int idx = 0;
            for (size_t i = 0; i < validIndices.size(); i++) {
                int toolIdx = validIndices[i];
                if (toolIdx == -1) {
                    std::cout << "------------------- Terminals ------------------\n";
                } else if (toolIdx == -2) {
                    std::cout << "------------------- Shells ---------------------\n";
                } else {
                    idx++;
                    std::string label = std::to_string(idx) + ". " + g_tools[toolIdx].name;
                    if (!g_tools[toolIdx].shell.empty()) label += " [Shell: " + g_tools[toolIdx].shell + "]";
                    else if (!g_tools[toolIdx].hotkey.empty()) label += " (" + g_tools[toolIdx].hotkey + ")";
                    
                    if (idx == selected) std::cout << "\033[7m> " << label << "\033[0m";
                    else std::cout << "  " << label;
                    std::cout << "\n";
                }
                printedLines++;
            }
            
            // System separator
            std::cout << "------------------- System ---------------------\n"; printedLines++;
            
            int settingIdx = numSelectable + 1;
            std::string settingLabel = std::to_string(settingIdx) + ". Setting";
            if (settingIdx == selected) std::cout << "\033[7m> " << settingLabel << "\033[0m";
            else std::cout << "  " << settingLabel;
            std::cout << "\n"; printedLines++;
            
            std::cout << "------------------------------------------------\n"; printedLines++;
            std::cout.flush();
        };
        
        enableRawMode();
        renderMenu();
        int finalChoice = -1;
        while (true) {
            int key = getKey();
            if (key == 2) { selected++; if (selected > numSelectable + 1) selected = 1; renderMenu(); }
            else if (key == 1) { selected--; if (selected < 1) selected = numSelectable + 1; renderMenu(); }
            else if (key == '\n' || key == '\r') { finalChoice = selected; break; }
            else if (key == '\033') { finalChoice = -1; break; }
            else if ((numSelectable + 1) <= 9 && key >= '1' && key <= '9') {
                int num = key - '0';
                if (num <= numSelectable + 1) {
                    finalChoice = num;
                    break;
                }
            }
        }
        disableRawMode();
        
        // Clear the TUI menu lines before taking action, to seamlessly restore the underlying terminal state.
        if (printedLines > 0) {
            std::cout << "\033[" << printedLines << "A";
            for(int i=0; i<printedLines; i++) std::cout << "\r\033[K\n";
            std::cout << "\033[" << printedLines << "A";
            std::cout.flush();
            printedLines = 0;
        }
        
        if (finalChoice == -1) break;
        
        if (finalChoice == numSelectable + 1) {
            RunTuiConfig();
            // Loop continues, will renderMenu() again
        } else {
            int actualIdx = -1;
            int count = 0;
            for(int idx : validIndices) {
                if (idx >= 0) {
                    count++;
                    if (count == finalChoice) {
                        actualIdx = idx;
                        break;
                    }
                }
            }
            if (actualIdx != -1) {
                LaunchToolByIndex(actualIdx + 1);
                break;
            }
        }
    }
    
    return 0;
}


#endif
