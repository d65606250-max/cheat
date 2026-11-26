#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <string>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

static HWND g_pathEdit = nullptr;
static HWND g_status = nullptr;
static HWND g_log = nullptr;
static std::wstring g_configDir;
static std::wstring g_logPath;

std::wstring GetAppDataDir() {
    wchar_t* appData = nullptr;
    size_t len = 0;
    _wdupenv_s(&appData, &len, L"APPDATA");
    std::wstring result = appData ? appData : L".";
    free(appData);
    result += L"\\RobRunLauncher";
    return result;
}

void AppendLog(const std::wstring& message) {
    SYSTEMTIME st{};
    GetLocalTime(&st);
    std::wofstream file(g_logPath, std::ios::app);
    if (file) {
        file << L"[" << st.wYear << L"-" << st.wMonth << L"-" << st.wDay
             << L" " << st.wHour << L":" << st.wMinute << L":" << st.wSecond
             << L"] " << message << L"\n";
    }
    if (g_log) {
        int n = GetWindowTextLengthW(g_log);
        SendMessageW(g_log, EM_SETSEL, n, n);
        std::wstring line = message + L"\r\n";
        SendMessageW(g_log, EM_REPLACESEL, FALSE, (LPARAM)line.c_str());
    }
}

void SetStatus(const std::wstring& s) {
    SetWindowTextW(g_status, s.c_str());
}

void SaveConfig(const std::wstring& exePath) {
    std::wofstream f(g_configDir + L"\\config.ini");
    if (f) f << exePath;
}

std::wstring LoadConfig() {
    std::wifstream f(g_configDir + L"\\config.ini");
    std::wstring value;
    if (f) std::getline(f, value);
    return value;
}

void BrowseForGame(HWND owner) {
    wchar_t file[MAX_PATH] = L"";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = L"Executable files (*.exe)\0*.exe\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    ofn.lpstrTitle = L"Select the ROB RUN executable";
    if (GetOpenFileNameW(&ofn)) {
        SetWindowTextW(g_pathEdit, file);
        SaveConfig(file);
        SetStatus(L"Executable selected.");
        AppendLog(L"Selected: " + std::wstring(file));
    }
}

void LaunchGame(HWND owner) {
    wchar_t pathBuf[32768]{};
    GetWindowTextW(g_pathEdit, pathBuf, 32768);
    std::wstring path = pathBuf;

    if (path.empty() || !fs::exists(path) || fs::is_directory(path)) {
        MessageBoxW(owner, L"Select a valid game .exe file first.", L"ROB RUN Launcher", MB_ICONWARNING);
        SetStatus(L"Please select a valid executable.");
        return;
    }

    if (_wcsicmp(fs::path(path).extension().c_str(), L".exe") != 0) {
        MessageBoxW(owner, L"The selected file is not an .exe file.", L"ROB RUN Launcher", MB_ICONWARNING);
        return;
    }

    SaveConfig(path);
    AppendLog(L"Starting game: " + path);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::wstring command = L"\"" + path + L"\"";
    std::wstring workingDir = fs::path(path).parent_path().wstring();

    BOOL ok = CreateProcessW(
        path.c_str(),
        command.data(),
        nullptr, nullptr, FALSE,
        0, nullptr,
        workingDir.empty() ? nullptr : workingDir.c_str(),
        &si, &pi
    );

    if (!ok) {
        DWORD err = GetLastError();
        wchar_t msg[256];
        swprintf_s(msg, L"Could not start the game. Windows error: %lu", err);
        SetStatus(L"Launch failed.");
        AppendLog(msg);
        MessageBoxW(owner, msg, L"ROB RUN Launcher", MB_ICONERROR);
        return;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    SetStatus(L"Game process started.");
    AppendLog(L"Game process started successfully.");
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        auto make = [&](DWORD ex, const wchar_t* cls, const wchar_t* text, DWORD style,
                        int x, int y, int w, int h, int id) -> HWND {
            HWND c = CreateWindowExW(ex, cls, text, WS_CHILD | WS_VISIBLE | style,
                                     x, y, w, h, hwnd, (HMENU)(INT_PTR)id,
                                     GetModuleHandleW(nullptr), nullptr);
            SendMessageW(c, WM_SETFONT, (WPARAM)font, TRUE);
            return c;
        };

        make(0, L"STATIC", L"ROB RUN", SS_LEFT, 24, 18, 250, 36, 0);
        make(0, L"STATIC", L"CPP GAME LAUNCHER", SS_LEFT, 26, 48, 260, 20, 0);
        make(0, L"STATIC", L"Game executable:", SS_LEFT, 24, 88, 140, 22, 0);
        g_pathEdit = make(WS_EX_CLIENTEDGE, L"EDIT", L"", ES_AUTOHSCROLL, 24, 112, 440, 28, 101);
        make(0, L"BUTTON", L"Browse...", BS_PUSHBUTTON, 474, 112, 100, 28, 102);
        make(0, L"BUTTON", L"START GAME", BS_PUSHBUTTON, 24, 154, 550, 42, 103);
        g_status = make(0, L"STATIC", L"Choose the game executable to begin.", SS_LEFT, 24, 207, 550, 22, 104);
        make(0, L"STATIC", L"Activity log", SS_LEFT, 24, 240, 200, 20, 0);
        g_log = make(WS_EX_CLIENTEDGE, L"EDIT", L"", ES_MULTILINE | ES_AUTOVSCROLL |
                    ES_READONLY | WS_VSCROLL, 24, 264, 550, 150, 105);

        std::wstring saved = LoadConfig();
        if (!saved.empty()) SetWindowTextW(g_pathEdit, saved.c_str());
        AppendLog(L"Launcher ready. No game files are modified.");
        return 0;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 102: BrowseForGame(hwnd); return 0;
        case 103: LaunchGame(hwnd); return 0;
        }
        break;
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(232, 236, 242));
        SetBkColor(hdc, RGB(30, 34, 42));
        static HBRUSH brush = CreateSolidBrush(RGB(30, 34, 42));
        return (LRESULT)brush;
    }
    case WM_ERASEBKGND: {
        RECT r; GetClientRect(hwnd, &r);
        FillRect((HDC)wParam, &r, CreateSolidBrush(RGB(22, 25, 32)));
        return 1;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    g_configDir = GetAppDataDir();
    std::error_code ec;
    fs::create_directories(g_configDir, ec);
    g_logPath = g_configDir + L"\\launcher.log";

    const wchar_t CLASS_NAME[] = L"RobRunCppLauncherWindow";
    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = instance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(22, 25, 32));
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"ROB RUN | C++ Launcher",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 620, 480,
        nullptr, nullptr, instance, nullptr);

    if (!hwnd) return 1;
    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
