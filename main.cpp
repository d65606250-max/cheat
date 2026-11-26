#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <fstream>
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

constexpr wchar_t STEAM_APP_ID[] = L"4262460";
constexpr wchar_t STEAM_URI[] = L"steam://run/4262460";

static HWND g_status = nullptr;
static HWND g_log = nullptr;
static std::wstring g_configDir;
static std::wstring g_logPath;
static HBRUSH g_backgroundBrush = nullptr;
static HBRUSH g_controlBrush = nullptr;

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
    file << L"["
         << st.wYear << L"-"
         << st.wMonth << L"-"
         << st.wDay << L" "
         << st.wHour << L":"
         << st.wMinute << L":"
         << st.wSecond << L"] "
         << message << L"\n";
}

if (g_log) {
    int n = GetWindowTextLengthW(g_log);
    SendMessageW(g_log, EM_SETSEL, n, n);

    std::wstring line = message + L"\r\n";
    SendMessageW(
        g_log,
        EM_REPLACESEL,
        FALSE,
        reinterpret_cast<LPARAM>(line.c_str())
    );
}
}

void SetStatus(const std::wstring& message) {
if (g_status) {
SetWindowTextW(g_status, message.c_str());
}
}

void LaunchGame(HWND owner) {
AppendLog(L"Requesting Steam launch for App ID: " +
std::wstring(STEAM_APP_ID));

HINSTANCE result = ShellExecuteW(
    owner,
    L"open",
    STEAM_URI,
    nullptr,
    nullptr,
    SW_SHOWNORMAL
);

if (reinterpret_cast<INT_PTR>(result) <= 32) {
    wchar_t message[256]{};

    swprintf_s(
        message,
        L"Could not open Steam. Windows error: %ld",
        reinterpret_cast<INT_PTR>(result)
    );

    SetStatus(L"Steam launch failed.");
    AppendLog(message);

    MessageBoxW(
        owner,
        message,
        L"ROB RUN Launcher",
        MB_ICONERROR
    );

    return;
}

SetStatus(L"Steam launch request sent.");
AppendLog(L"Steam launch request sent successfully.");

MessageBoxW(
    owner,
    L"Steam has received the launch request.\n"
    L"The game will start through its official Steam configuration.",
    L"ROB RUN Launcher",
    MB_ICONINFORMATION
);
}

LRESULT CALLBACK WndProc(
HWND hwnd,
UINT msg,
WPARAM wParam,
LPARAM lParam
) {
switch (msg) {
case WM_CREATE: {
HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    auto make = [&](const wchar_t* cls,
                    const wchar_t* text,
                    DWORD style,
                    int x, int y, int w, int h,
                    int id) -> HWND {

        HWND control = CreateWindowExW(
            0,
            cls,
            text,
            WS_CHILD | WS_VISIBLE | style,
            x, y, w, h,
            hwnd,
            (HMENU)(INT_PTR)id,
            GetModuleHandleW(nullptr),
            nullptr
        );

        SendMessageW(
            control,
            WM_SETFONT,
            (WPARAM)font,
            TRUE
        );

        return control;
    };

    make(
        L"STATIC",
        L"ROB RUN",
        SS_LEFT,
        24, 20, 300, 36, 0
    );

    make(
        L"STATIC",
        L"STEAM GAME LAUNCHER",
        SS_LEFT,
        26, 50, 300, 22, 0
    );

    make(
        L"STATIC",
        L"Steam App ID:",
        SS_LEFT,
        24, 100, 150, 22, 0
    );

    make(
        L"STATIC",
        STEAM_APP_ID,
        SS_LEFT,
        160, 100, 200, 22, 0
    );

    make(
        L"STATIC",
        L"Launch method: Official Steam protocol",
        SS_LEFT,
        24, 135, 500, 22, 0
    );

    make(
        L"BUTTON",
        L"START GAME THROUGH STEAM",
        BS_PUSHBUTTON,
        24, 175, 550, 44, 103
    );

    g_status = make(
        L"STATIC",
        L"Ready.",
        SS_LEFT,
        24, 235, 550, 22, 104
    );

    make(
        L"STATIC",
        L"Activity log",
        SS_LEFT,24, 270, 200, 20, 0
    );

    g_log = make(
        L"EDIT",
        L"",
        ES_MULTILINE |
        ES_AUTOVSCROLL |
        ES_READONLY |
        WS_VSCROLL,
        24, 295, 550, 115, 105
    );

    AppendLog(L"Launcher initialized.");
    AppendLog(L"Steam App ID: 4262460");
    AppendLog(L"Launch mode: official Steam protocol.");

    return 0;
}

case WM_COMMAND:
    if (LOWORD(wParam) == 103) {
        LaunchGame(hwnd);
        return 0;
    }
    break;

case WM_CTLCOLORSTATIC:
case WM_CTLCOLOREDIT: {
    HDC hdc = (HDC)wParam;

    SetTextColor(hdc, RGB(232, 236, 242));
    SetBkColor(hdc, RGB(30, 34, 42));

    return (LRESULT)g_controlBrush;
}

case WM_ERASEBKGND: {
    RECT rect{};
    GetClientRect(hwnd, &rect);

    FillRect(
        (HDC)wParam,
        &rect,
        g_backgroundBrush
    );

    return 1;
}

case WM_DESTROY:
    DeleteObject(g_backgroundBrush);
    DeleteObject(g_controlBrush);
    PostQuitMessage(0);
    return 0;
}

return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(
HINSTANCE instance,
HINSTANCE,
PWSTR,
int show
) {
g_configDir = GetAppDataDir();

std::error_code ec;
fs::create_directories(g_configDir, ec);

g_logPath = g_configDir + L"\\launcher.log";

g_backgroundBrush = CreateSolidBrush(RGB(22, 25, 32));
g_controlBrush = CreateSolidBrush(RGB(30, 34, 42));

const wchar_t CLASS_NAME[] = L"RobRunSteamLauncher";

WNDCLASSW wc{};
wc.lpfnWndProc = WndProc;
wc.hInstance = instance;
wc.lpszClassName = CLASS_NAME;
wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
wc.hbrBackground = g_backgroundBrush;

if (!RegisterClassW(&wc)) {
    return 1;
}

HWND hwnd = CreateWindowExW(
    0,
    CLASS_NAME,
    L"ROB RUN | Steam Launcher",
    WS_OVERLAPPED |
    WS_CAPTION |
    WS_SYSMENU |
    WS_MINIMIZEBOX,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    620,
    480,
    nullptr,
    nullptr,
    instance,
    nullptr
);

if (!hwnd) {
    DeleteObject(g_backgroundBrush);
    DeleteObject(g_controlBrush);
    return 1;
}

ShowWindow(hwnd, show);
UpdateWindow(hwnd);

MSG msg{};

while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
}

return (int)msg.wParam;
}
