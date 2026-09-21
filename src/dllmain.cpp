/*
 * XPE CHAMS v2 - Reconstructed from CHAMSMENU.dll
 * Author/Copyright: xpe.nettt
 * Title: XPE CHAMS
 * 
 * DllMain - Entry point, initializes overlay + chams
 * Supports: BlueStacks, LDPlayer (all versions)
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <thread>
#include "config.h"
#include "overlay.h"
#include "glchams.h"
#include "keyauth.h"
#include "streammode.h"
#include "hooks.h"

// ============================================================
// Global state
// ============================================================
HINSTANCE g_hInstance = NULL;
bool g_bRunning = false;
bool g_bInitialized = false;
HWND g_hGameWindow = NULL;
HMODULE g_hOpenGL = NULL;

// ============================================================
// Forward declarations
// ============================================================
void OverlayThread();
void AuthThread();
bool FindGameWindow();

// ============================================================
// Window detection - works for BlueStacks and LDPlayer
// ============================================================
bool FindGameWindow()
{
    const char* classes[] = {
        "LDPlayerMainFrame",
        "LDPlayer9MainFrame",
        "BlueStacksApp",
        "AWindow",
        NULL
    };

    for (int i = 0; classes[i]; i++)
    {
        g_hGameWindow = FindWindowA(classes[i], NULL);
        if (g_hGameWindow)
            return true;
    }

    // Fallback: find any LDPlayer/BlueStacks window
    char title[256];
    HWND hWnd = FindWindowA(NULL, NULL);
    if (hWnd)
    {
        GetWindowTextA(hWnd, title, sizeof(title));
        if (strstr(title, "LDPlayer") || strstr(title, "BlueStacks"))
        {
            g_hGameWindow = hWnd;
            return true;
        }
    }

    return false;
}

// ============================================================
// Authentication thread
// ============================================================
void AuthThread()
{
    Sleep(500);
    g_KeyAuth.init(KEYAUTH_OWNERID, KEYAUTH_APPNAME, KEYAUTH_VERSION, KEYAUTH_API_URL);

    // Check for saved key in registry
    char savedKey[256] = { 0 };
    DWORD keySize = sizeof(savedKey);
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\XPE CHAMS", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        RegQueryValueExA(hKey, "LicenseKey", NULL, NULL, (LPBYTE)savedKey, &keySize);
        RegCloseKey(hKey);
    }

    if (savedKey[0])
    {
        g_KeyAuth.login(savedKey);
        g_Config.bAuthenticated = g_KeyAuth.isLoggedIn();
    }

    g_Config.bShowLogin = !g_Config.bAuthenticated;
    g_bInitialized = true;
}

// ============================================================
// Overlay thread
// ============================================================
void OverlayThread()
{
    while (!g_bInitialized) Sleep(10);

    Overlay overlay;
    if (!overlay.Initialize(g_hGameWindow))
        return;

    g_bRunning = true;
    MSG msg = { 0 };
    while (g_bRunning)
    {
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        overlay.Render();
        Sleep(1);
    }

    overlay.Shutdown();
}

// ============================================================
// Main initialization
// ============================================================
void Initialize()
{
    if (!FindGameWindow())
    {
        MessageBoxA(NULL, "No emulator window found!\nRun LDPlayer or BlueStacks first.",
            "XPE CHAMS", MB_ICONERROR);
        return;
    }

    // Start auth thread
    std::thread(AuthThread).detach();

    // Start overlay thread
    std::thread(OverlayThread).detach();

    // Initialize OpenGL chams
    g_hOpenGL = LoadLibraryA("opengl32.dll");
    if (g_hOpenGL)
        GLChams::Initialize();
}

// ============================================================
// DllMain
// ============================================================
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
    {
        g_hInstance = hinstDLL;
        DisableThreadLibraryCalls(hinstDLL);

        HANDLE hThread = CreateThread(NULL, 0,
            [](LPVOID) -> DWORD {
                Initialize();
                return 0;
            }, NULL, 0, NULL);
        if (hThread) CloseHandle(hThread);

        return TRUE;
    }

    case DLL_PROCESS_DETACH:
    {
        g_bRunning = false;
        break;
    }
    }
    return TRUE;
}