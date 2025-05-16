#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <fstream>
#include <string>
#include <CommCtrl.h>
#include "resource.h"

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAYICON 1
#define DEFAULT_TIMEOUT 300
#define CONFIG_FILENAME "insomnia_config.ini"

// Global variables
HWND g_hwnd = NULL;
HWND g_dialogHwnd = NULL;
NOTIFYICONDATA g_nid = { 0 };
UINT_PTR g_timerId = 1;
int g_inactivityTimeout = DEFAULT_TIMEOUT;
POINT g_lastMousePos = { 0 };
DWORD g_lastMouseMoveTime = 0;

// Function declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void LoadConfiguration();
void SaveConfiguration();
void SimulateMouseMovement();
void CreateTrayIcon(HWND hwnd);
void RemoveTrayIcon();
void ShowConfigDialog();
void ShowTrayMenu(HWND hwnd);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Register window class
    const wchar_t CLASS_NAME[] = L"InsomniaWindowClass";
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    
    RegisterClass(&wc);

    // Create the hidden window
    g_hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Insomnia",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        400, 300,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (g_hwnd == NULL) {
        return 0;
    }

    // Load configuration
    LoadConfiguration();

    // Create tray icon
    CreateTrayIcon(g_hwnd);

    // Start the inactivity timer
    SetTimer(g_hwnd, g_timerId, 1000, NULL); // Check every second

    // Message loop
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    RemoveTrayIcon();
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_TIMER:
            if (wParam == g_timerId) {
                POINT currentPos;
                GetCursorPos(&currentPos);
                
                DWORD currentTime = GetTickCount();
                if (currentPos.x != g_lastMousePos.x || currentPos.y != g_lastMousePos.y) {
                    g_lastMousePos = currentPos;
                    g_lastMouseMoveTime = currentTime;
                }
                else if (currentTime - g_lastMouseMoveTime >= (DWORD)(g_inactivityTimeout * 1000)) {
                    SimulateMouseMovement();
                    g_lastMouseMoveTime = currentTime;
                }
            }
            return 0;

        case WM_TRAYICON:
            if (lParam == WM_LBUTTONUP) {
                ShowConfigDialog();
            }
            else if (lParam == WM_RBUTTONUP) {
                ShowTrayMenu(hwnd);
            }
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_TRAY_CONFIG:
                    ShowConfigDialog();
                    break;
                case ID_TRAY_EXIT:
                    DestroyWindow(hwnd);
                    break;
            }
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void LoadConfiguration() {
    std::ifstream config(CONFIG_FILENAME);
    if (config.is_open()) {
        std::string line;
        if (std::getline(config, line)) {
            try {
                int timeout = std::stoi(line);
                if (timeout > 0) {
                    g_inactivityTimeout = timeout;
                }
            }
            catch (...) {
                // Use default timeout if parsing fails
            }
        }
        config.close();
    }
}

void SaveConfiguration() {
    std::ofstream config(CONFIG_FILENAME);
    if (config.is_open()) {
        config << g_inactivityTimeout;
        config.close();
    }
}

void SimulateMouseMovement() {
    POINT currentPos;
    GetCursorPos(&currentPos);
    
    // Store original position
    g_lastMousePos = currentPos;
    
    // Move cursor slightly (10 pixels) in the most appropriate direction
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    POINT newPos = currentPos;
    if (currentPos.x < screenWidth / 2) {
        newPos.x += 10;
    }
    else {
        newPos.x -= 10;
    }
    
    // Simulate mouse movement
    SetCursorPos(newPos.x, newPos.y);
    Sleep(100); // Small delay to ensure Windows detects the movement
    SetCursorPos(currentPos.x, currentPos.y); // Return to original position
}

void CreateTrayIcon(HWND hwnd) {
    g_nid.cbSize = sizeof(NOTIFYICONDATA);
    g_nid.hWnd = hwnd;
    g_nid.uID = ID_TRAYICON;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_INSOMNIA));
    wcscpy_s(g_nid.szTip, sizeof(g_nid.szTip)/sizeof(WCHAR), L"Insomnia");
    Shell_NotifyIcon(NIM_ADD, &g_nid);
}

void RemoveTrayIcon() {
    Shell_NotifyIcon(NIM_DELETE, &g_nid);
}

void ShowConfigDialog() {
    if (g_dialogHwnd == NULL) {
        g_dialogHwnd = CreateDialog(
            GetModuleHandle(NULL),
            L"CONFIG_DIALOG",
            g_hwnd,
            DialogProc
        );
        if (g_dialogHwnd) {
            ShowWindow(g_dialogHwnd, SW_SHOW);
        }
    }
    else {
        SetForegroundWindow(g_dialogHwnd);
    }
}

INT_PTR CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG: {
            // Set current timeout value
            SetDlgItemInt(hwnd, IDC_TIMEOUT, g_inactivityTimeout, FALSE);
            return TRUE;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                BOOL success;
                int newTimeout = GetDlgItemInt(hwnd, IDC_TIMEOUT, &success, FALSE);
                if (success && newTimeout > 0) {
                    g_inactivityTimeout = newTimeout;
                    SaveConfiguration();
                }
                EndDialog(hwnd, IDOK);
                g_dialogHwnd = NULL;
                return TRUE;
            }
            else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hwnd, IDCANCEL);
                g_dialogHwnd = NULL;
                return TRUE;
            }
            break;

        case WM_CLOSE:
            EndDialog(hwnd, IDCANCEL);
            g_dialogHwnd = NULL;
            return TRUE;
    }
    return FALSE;
}

void ShowTrayMenu(HWND hwnd) {
    POINT pt;
    GetCursorPos(&pt);
    
    HMENU hMenu = CreatePopupMenu();
    InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, ID_TRAY_CONFIG, L"Config");
    InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, ID_TRAY_EXIT, L"Exit");
    
    // Required to make menu work with keyboard
    SetForegroundWindow(hwnd);
    
    TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON,
        pt.x, pt.y, 0, hwnd, NULL);
    
    DestroyMenu(hMenu);
}
