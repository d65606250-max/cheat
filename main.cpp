
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <string>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")

constexpr wchar_t APP_TITLE[] = L"REX MENU";
constexpr wchar_t STEAM_URI[] = L"steam://run/4262460";

constexpr int ID_LAUNCH = 1001;
constexpr int ID_VISUAL = 1002;
constexpr int ID_AIM = 1003;
constexpr int ID_LOOT = 1004;
constexpr int ID_TOPMOST = 1005;
constexpr int HOTKEY_INSERT = 1;

HWND g_status = nullptr;
HWND g_main = nullptr;

HBRUSH g_background = nullptr;
HBRUSH g_controlBackground = nullptr;

COLORREF BG_COLOR = RGB(12, 12, 12);
COLORREF PANEL_COLOR = RGB(27, 27, 27);
COLORREF TEXT_COLOR = RGB(235, 235, 235);
COLORREF MUTED_COLOR = RGB(145, 145, 145);

void SetStatus(const wchar_t* text)
{
    SetWindowTextW(g_status, text);
}

HWND AddControl(
    const wchar_t* type,
    const wchar_t* text,
    DWORD style,
    int x, int y, int w, int h,
    int id,
    HWND parent)
{
    return CreateWindowW(
        type,
        text,
        WS_CHILD | WS_VISIBLE | style,
        x, y, w, h,
        parent,
        reinterpret_cast<HMENU>(
            static_cast<INT_PTR>(id)),
        GetModuleHandleW(nullptr),
        nullptr
    );
}

void LaunchGame()
{
    SetStatus(L"Connecting to Steam...");

    HINSTANCE result = ShellExecuteW(
        g_main,
        L"open",
        STEAM_URI,
        nullptr,
        nullptr,
        SW_SHOWNORMAL
    );

    if (reinterpret_cast<INT_PTR>(result) <= 32)
    {
        SetStatus(L"Steam launch failed.");
        return;
    }

    SetStatus(L"ROB RUN launch request sent.");
}

void CreateMenu(HWND hwnd)
{
    AddControl(
        L"STATIC", L"REX MENU",
        0, 25, 20, 400, 45, 0, hwnd
    );

    AddControl(
        L"STATIC", L"ROB RUN  |  GAME PANEL",
        0, 27, 65, 400, 22, 0, hwnd
    );

    AddControl(
        L"STATIC", L"VISUAL SETTINGS",
        0, 27, 110, 400, 25, 0, hwnd
    );

    AddControl(
        L"BUTTON", L"Show interface indicators",
        BS_AUTOCHECKBOX,
        30, 140, 400, 28, ID_VISUAL, hwnd
    );

    AddControl(
        L"STATIC", L"AIM SETTINGS",
        0, 27, 185, 400, 25, 0, hwnd
    );

    AddControl(
        L"BUTTON", L"Enable sensitivity settings panel",
        BS_AUTOCHECKBOX,
        30, 215, 400, 28, ID_AIM, hwnd
    );

    AddControl(
        L"STATIC", L"LOOT SETTINGS",
        0, 27, 260, 400, 25, 0, hwnd
    );

    AddControl(
        L"BUTTON", L"Show loot notifications",
        BS_AUTOCHECKBOX,
        30, 290, 400, 28, ID_LOOT, hwnd
    );

   AddControl(
    L"BUTTON",
    L"Keep menu above other windows",
    BS_AUTOCHECKBOX,
    30, 325, 420, 28,
    ID_TOPMOST,
    hwnd
);

    AddControl(
        L"BUTTON", L"START ROB RUN",
        BS_PUSHBUTTON,
        30, 370, 420, 42, ID_LAUNCH, hwnd
    );

    g_status = AddControl(
        L"STATIC", L"Ready.",
        0, 30, 425, 420, 25, 0, hwnd
    );
}

LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        CreateMenu(hwnd);
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_LAUNCH:
            LaunchGame();
            return 0;

        case ID_TOPMOST:
        {
            bool enabled =
                SendMessageW(
                    GetDlgItem(hwnd, ID_TOPMOST),
                    BM_GETCHECK, 0, 0
                ) == BST_CHECKED;

            SetWindowPos(
                hwnd,
                enabled ? HWND_TOPMOST : HWND_NOTOPMOST,
                0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE
            );

            return 0;
        }
        }
        return 0;

    case WM_HOTKEY:
        if (wParam == HOTKEY_INSERT)
        {
            if (IsWindowVisible(hwnd))
                ShowWindow(hwnd, SW_HIDE);
            else
            {
                ShowWindow(hwnd, SW_SHOW);
                SetWindowPos(
                    hwnd, HWND_TOPMOST,
                    0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE
                );
            }
        }
        return 0;

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT:
    {
        HDC dc = reinterpret_cast<HDC>(wParam);

        SetTextColor(dc, TEXT_COLOR);
        SetBkColor(dc, BG_COLOR);

        return reinterpret_cast<LRESULT>(g_background);
    }

    case WM_DESTROY:
        UnregisterHotKey(hwnd, HOTKEY_INSERT);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    PWSTR,
    int nCmdShow)
{
    g_background = CreateSolidBrush(BG_COLOR);
    g_controlBackground = CreateSolidBrush(PANEL_COLOR);

    const wchar_t CLASS_NAME[] = L"REX_MENU_WINDOW";

    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursorW(
    nullptr,
    MAKEINTRESOURCEW(32512)
);
    wc.hbrBackground = g_background;

    if (!RegisterClassW(&wc))
        return 1;

    g_main = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CLASS_NAME,
        APP_TITLE,
        WS_OVERLAPPED | WS_CAPTION |
        WS_SYSMENU | WS_MINIMIZEBOX,
        100, 100,
        500, 510,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!g_main)
        return 1;

    RegisterHotKey(
        g_main,
        HOTKEY_INSERT,
        0,
        VK_INSERT
    );

    ShowWindow(g_main, nCmdShow);
    UpdateWindow(g_main);

    MSG msg{};

    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    DeleteObject(g_background);
    DeleteObject(g_controlBackground);

    return static_cast<int>(msg.wParam);
}
