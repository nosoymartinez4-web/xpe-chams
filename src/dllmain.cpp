/*
 * XPE CHAMS v2 - DLL Main Entry Point
 * OpenGL Proxy + D3D11 Overlay with KeyAuth
 * Author: xpe.nettt / Stealth Proyects
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>

#include "config.h"
#include "keyauth.h"
#include "overlay.h"
#include "glchams.h"
#include "streammode.h"

// ========================
// DLL ENTRY POINT
// ========================
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
        {
            DisableThreadLibraryCalls(hModule);

            // Initialize overlay (hooks D3D11)
            g_Overlay.Initialize();

            // Initialize GL Chams
            g_GLChams.Initialize();

            // Initialize Stream Mode
            g_StreamMode.Initialize();

            break;
        }

        case DLL_PROCESS_DETACH:
        {
            g_Overlay.Shutdown();
            g_GLChams.Shutdown();
            g_StreamMode.Shutdown();
            break;
        }

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;
}