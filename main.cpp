#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <fstream>
#include <filesystem>
#include <cstdlib>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")

constexpr wchar_t APP_TITLE[] = L"ROB RUN Launcher";
constexpr wchar_t STEAM_APP_ID[] = L"4262460";
constexpr wchar_t STEAM_URI[] = L"steam://run/4262460";

HWND g_status = nullptr;
HWND g_log = nullptr;
HWND g_launchButton = nullptr;

std::filesystem::path GetLogPath()
{
wchar_t* appData = nullptr;
size_t size = 0;

if (_wdupenv_s(&appData, &size, L"APPDATA") != 0 || !appData)
    return L"RobRunLauncher.log";

std::filesystem::path folder =
    std::filesystem::path(appData) / L"RobRunLauncher";

free(appData);

std::error_code ec;
std::filesystem::create_directories(folder, ec);

return folder / L"launcher.log";
}

void AppendLog(const std::wstring& message)
{
std::wofstream file(GetLogPath(), std::ios::app);

if (file.is_open())
    file << message << L"\n";

if (g_log)
{
    int length = GetWindowTextLengthW(g_log);
    SendMessageW(g_log, EM_SETSEL, length, length);
    SendMessageW(g_log, EM_REPLACESEL, FALSE,
                 reinterpret_cast<LPARAM>((message + L"\r\n").c_str()));
}
}

void SetStatus(const wchar_t* message)
{
if (g_status)
SetWindowTextW(g_status, message);
}

void LaunchGame(HWND owner)
{
SetStatus(L"Opening Steam...");
AppendLog(L"Requesting official Steam launch for ROB RUN.");

HINSTANCE result = ShellExecuteW(
    owner,
    L"open",
    STEAM_URI,
    nullptr,
    nullptr,
    SW_SHOWNORMAL
);

INT_PTR code = reinterpret_cast<INT_PTR>(result);

if (code <= 32)
{
    wchar_t message[256]{};

    swprintf_s(
        message,
        _countof(message),
        L"Could not open Steam. ShellExecute error: %lld",
        static_cast<long long>(code)
    );

    SetStatus(L"Steam launch failed.");
    AppendLog(message);

    MessageBoxW(
        owner,
        message,
        APP_TITLE,
        MB_ICONERROR | MB_OK
    );

    return;
}

SetStatus(L"Steam launch request sent.");
AppendLog(L"Steam accepted the launch request.");

MessageBoxW(
    owner,
    L"Launch request sent to Steam.\n"
    L"Make sure Steam is running and ROB RUN is installed.",
    APP_TITLE,
    MB_ICONINFORMATION | MB_OK
);
}

LRESULT CALLBACK WindowProc(
HWND hwnd,
UINT msg,
WPARAM wParam,
LPARAM lParam
)
{
switch (msg)
{
case WM_CREATE:
{
CreateWindowW(
L"STATIC",
L"ROB RUN",
WS_CHILD | WS_VISIBLE,
30, 25, 400, 45,
hwnd, nullptr, nullptr, nullptr
);

    CreateWindowW(
        L"STATIC",
        L"Steam App ID: 4262460",
        WS_CHILD | WS_VISIBLE,
        32, 75, 400, 25,
        hwnd, nullptr, nullptr, nullptr
    );

    CreateWindowW(
        L"STATIC",
        L"Launch mode: Official Steam protocol",
        WS_CHILD | WS_VISIBLE,
        32, 105, 400, 25,
        hwnd, nullptr, nullptr, nullptr
    );

    g_launchButton = CreateWindowW(
        L"BUTTON",
        L"START GAME THROUGH STEAM",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        30, 150, 330, 42,
        hwnd,
        reinterpret_cast<HMENU>(1001),
        GetModuleHandleW(nullptr),
        nullptr
    );

    g_status = CreateWindowW(
        L"STATIC",
        L"Ready.",
        WS_CHILD | WS_VISIBLE,
        32, 210, 450, 25,
        hwnd, nullptr, nullptr, nullptr
    );

    CreateWindowW(
        L"STATIC",
        L"Activity log:",
        WS_CHILD | WS_VISIBLE,
        32, 245, 400, 22,
        hwnd, nullptr, nullptr, nullptr
    );

    g_log = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE |
        ES_MULTILINE | ES_READONLY |
        ES_AUTOVSCROLL | WS_VSCROLL,
        30, 270, 450, 130,
        hwnd, nullptr, GetModuleHandleW(nullptr), nullptr
    );

    AppendLog(L"ROB RUN Launcher initialized.");
    AppendLog(L"Steam App ID: 4262460");

    return 0;
}

case WM_COMMAND:
    if (LOWORD(wParam) == 1001)
    {
        LaunchGame(hwnd);
        return 0;
    }
    break;

case WM_DESTROY:
    PostQuitMessage(0);return 0;
}

return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(
HINSTANCE hInstance,
HINSTANCE,
PWSTR,
int nCmdShow
)
{
const wchar_t CLASS_NAME[] = L"RobRunLauncherWindow";

WNDCLASSW wc{};
wc.lpfnWndProc = WindowProc;
wc.hInstance = hInstance;
wc.lpszClassName = CLASS_NAME;
wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

if (!RegisterClassW(&wc))
{
    MessageBoxW(
        nullptr,
        L"Could not register window class.",
        APP_TITLE,
        MB_ICONERROR
    );
    return 1;
}

HWND hwnd = CreateWindowExW(
    0,
    CLASS_NAME,
    APP_TITLE,
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    540,
    470,
    nullptr,
    nullptr,
    hInstance,
    nullptr
);

if (!hwnd)
{
    MessageBoxW(
        nullptr,
        L"Could not create launcher window.",
        APP_TITLE,
        MB_ICONERROR
    );
    return 1;
}

ShowWindow(hwnd, nCmdShow);
UpdateWindow(hwnd);

MSG msg{};

while (GetMessageW(&msg, nullptr, 0, 0) > 0)
{
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
}

return static_cast<int>(msg.wParam);
}
