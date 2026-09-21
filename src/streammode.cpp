// ============================================================
// XPE CHAMS v2 - StreamMode (anti-grabacion / Streamer Mode)
// Copyright (c) 2026 xpe.nettt / Stealth Proyects. All rights reserved.
// ============================================================
// Port completo de StreamMode.hpp (bypass NVIDIA ShadowPlay +
// ocultamiento ante apps de grabacion).
//
// Concepto (heredado del StreamMode.hpp original):
//   - Sin el patch, ShadowPlay detecta WDA_EXCLUDEFROMCAPTURE en
//     nuestro overlay y DETIENE la grabacion por completo.
//   - Con el patch a nvd3dumx.dll (dentro de nvcontainer.exe), la
//     comprobacion de DRM Widevine L1 queda neutralizada: ShadowPlay
//     sigue grabando y Windows sigue excluyendo el overlay.
//   - NO hookeamos GetWindowDisplayAffinity: queremos que ShadowPlay
//     vea WDA_EXCLUDEFROMCAPTURE para que el overlay quede fuera de
//     la captura, no adentro.
// ============================================================
#include "streammode.h"
#include "config.h"
#include <tlhelp32.h>
#include <psapi.h>
#include <vector>
#include <cstring>

#pragma comment(lib, "psapi")

namespace {

// ------------------------------------------------------------------
// Estado del modulo
// ------------------------------------------------------------------
volatile bool s_enabled     = false;   // Streamer Mode activado por el usuario
volatile bool s_monitoring  = false;   // hilo monitor corriendo
volatile bool s_patched     = false;   // nvcontainer patcheado
DWORD         s_patchedPID  = 0;

std::vector<BYTE*> s_patchAddrs1;      // direcciones del patron 1
std::vector<BYTE*> s_patchAddrs2;      // direcciones del patron 2

// ------------------------------------------------------------------
// Patrones Widevine L1 en nvd3dumx.dll (bytes originales -> parche)
// ------------------------------------------------------------------
const BYTE s_original1[] = { 0x44, 0x8B, 0x82, 0x70, 0x01, 0x00, 0x00, 0x45, 0x85, 0xC0 };
const BYTE s_patched1[]  = { 0x45, 0x31, 0xC0, 0x90, 0x90, 0x90, 0x90, 0x45, 0x85, 0xC0 };

const BYTE s_original2[] = { 0x8B, 0x88, 0x70, 0x01, 0x00, 0x00, 0x85, 0xC9 };
const BYTE s_patched2[]  = { 0x31, 0xC9, 0x90, 0x90, 0x90, 0x90, 0x85, 0xC9 };

// ------------------------------------------------------------------
// Apps de grabacion / streaming (deteccion por nombre de proceso)
// ------------------------------------------------------------------
bool IsCaptureAppRunning() {
    static const char* apps[] = {
        // OBS Studio
        "obs64.exe", "obs32.exe", "obs.exe",
        // Discord (manda DDA pero por las dudas)
        "discord.exe",
        // XSplit / Twitch Studio
        "xsplit.core.exe", "twitchstudio.exe",
        // Streamlabs / StreamElements
        "streamlabs.exe", "streamelements.exe",
        // Vmix / ffmpeg (copiado de v1)
        "vmix.exe", "ffmpeg.exe",
        // Bandicam
        "bandicam.exe", "bdcam.exe",
        // NVIDIA ShadowPlay / GeForce Experience
        "nvidia share.exe", "nvcontainer.exe", "shadowplay.exe",
        // Otros capturadores conocidos
        "fraps.exe", "dxtory.exe", "action.exe", "mirillis action.exe",
        "playclaw.exe", "ocam.exe", "camtasia.exe", "xboxapp.exe",
        "gamebar.exe", "gcgameservices.exe", "gamebarpresencewriter.exe",
        nullptr
    };
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;

    bool found = false;
    PROCESSENTRY32 pe = { sizeof(pe) };
    if (Process32First(snap, &pe)) {
        do {
            // Normaliza a minusculas
            char exe[MAX_PATH];
            strcpy_s(exe, pe.szExeFile);
            for (char* p = exe; *p; ++p)
                *p = (char)tolower((unsigned char)*p);
            for (int i = 0; apps[i]; ++i) {
                if (strstr(exe, apps[i])) { found = true; break; }
            }
        } while (!found && Process32Next(snap, &pe));
    }
    CloseHandle(snap);
    return found;
}

// ------------------------------------------------------------------
// Helpers de proceso remoto
// ------------------------------------------------------------------
std::vector<DWORD> FindNvContainerPIDs() {
    std::vector<DWORD> pids;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return pids;

    PROCESSENTRY32W pe = {};
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, L"nvcontainer.exe") == 0)
                pids.push_back(pe.th32ProcessID);
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);
    return pids;
}

bool FindRemoteModule(HANDLE hProcess, const wchar_t* moduleName,
                      BYTE*& outBase, SIZE_T& outSize) {
    HMODULE hMods[1024];
    DWORD cbNeeded = 0;
    if (!EnumProcessModulesEx(hProcess, hMods, sizeof(hMods), &cbNeeded,
                              LIST_MODULES_ALL))
        return false;

    DWORD numModules = cbNeeded / sizeof(HMODULE);
    for (DWORD i = 0; i < numModules; i++) {
        wchar_t modName[MAX_PATH] = { 0 };
        if (GetModuleBaseNameW(hProcess, hMods[i], modName, MAX_PATH)) {
            if (_wcsicmp(modName, moduleName) == 0) {
                MODULEINFO modInfo = {};
                if (GetModuleInformation(hProcess, hMods[i], &modInfo,
                                         sizeof(modInfo))) {
                    outBase = (BYTE*)modInfo.lpBaseOfDll;
                    outSize = modInfo.SizeOfImage;
                    return true;
                }
            }
        }
    }
    return false;
}

bool RemoteScanAndPatch(HANDLE hProcess, BYTE* moduleBase, SIZE_T moduleSize,
                        const BYTE* search, const BYTE* replace,
                        size_t patternSize, std::vector<BYTE*>& patchedAddrs) {
    std::vector<BYTE> buffer(moduleSize);
    SIZE_T bytesRead = 0;
    if (!ReadProcessMemory(hProcess, moduleBase, buffer.data(), moduleSize,
                           &bytesRead))
        return false;
    if (bytesRead < patternSize) return false;

    bool found = false;
    for (SIZE_T i = 0; i <= bytesRead - patternSize; i++) {
        if (memcmp(buffer.data() + i, search, patternSize) == 0) {
            BYTE* remoteAddr = moduleBase + i;
            DWORD oldProtect = 0;
            if (VirtualProtectEx(hProcess, remoteAddr, patternSize,
                                 PAGE_EXECUTE_READWRITE, &oldProtect)) {
                SIZE_T written = 0;
                if (WriteProcessMemory(hProcess, remoteAddr, replace,
                                       patternSize, &written)) {
                    patchedAddrs.push_back(remoteAddr);
                    found = true;
                }
                VirtualProtectEx(hProcess, remoteAddr, patternSize,
                                 oldProtect, &oldProtect);
            }
        }
    }
    return found;
}

bool RemoteRestore(HANDLE hProcess, BYTE* remoteAddr, const BYTE* original,
                   size_t size) {
    DWORD oldProtect = 0;
    if (VirtualProtectEx(hProcess, remoteAddr, size,
                         PAGE_EXECUTE_READWRITE, &oldProtect)) {
        SIZE_T written = 0;
        WriteProcessMemory(hProcess, remoteAddr, original, size, &written);
        VirtualProtectEx(hProcess, remoteAddr, size, oldProtect, &oldProtect);
        return written == size;
    }
    return false;
}

// ------------------------------------------------------------------
// Patch / unpatch de nvcontainer.exe
// ------------------------------------------------------------------
void PatchNvContainer() {
    auto pids = FindNvContainerPIDs();
    for (DWORD pid : pids) {
        HANDLE hProcess = OpenProcess(
            PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION |
            PROCESS_QUERY_INFORMATION,
            FALSE, pid);
        if (!hProcess) continue;

        BYTE* nvdBase = nullptr;
        SIZE_T nvdSize = 0;
        if (FindRemoteModule(hProcess, L"nvd3dumx.dll", nvdBase, nvdSize)) {
            s_patchAddrs1.clear();
            s_patchAddrs2.clear();

            RemoteScanAndPatch(hProcess, nvdBase, nvdSize,
                               s_original1, s_patched1,
                               sizeof(s_original1), s_patchAddrs1);
            RemoteScanAndPatch(hProcess, nvdBase, nvdSize,
                               s_original2, s_patched2,
                               sizeof(s_original2), s_patchAddrs2);

            s_patchedPID = pid;
            s_patched = true;
            CloseHandle(hProcess);
            return;
        }
        CloseHandle(hProcess);
    }
}

void UnpatchNvContainer() {
    if (!s_patched || s_patchedPID == 0) return;

    HANDLE hProcess = OpenProcess(
        PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION |
        PROCESS_QUERY_INFORMATION,
        FALSE, s_patchedPID);
    if (!hProcess) {
        s_patched = false; s_patchedPID = 0;
        s_patchAddrs1.clear(); s_patchAddrs2.clear();
        return;
    }

    for (auto addr : s_patchAddrs1)
        RemoteRestore(hProcess, addr, s_original1, sizeof(s_original1));
    for (auto addr : s_patchAddrs2)
        RemoteRestore(hProcess, addr, s_original2, sizeof(s_original2));

    CloseHandle(hProcess);
    s_patched = false; s_patchedPID = 0;
    s_patchAddrs1.clear(); s_patchAddrs2.clear();
}

// ------------------------------------------------------------------
// Hilo monitor (3 s): re-patch automatico si ShadowPlay se reinicia
// ------------------------------------------------------------------
DWORD WINAPI MonitorThread(LPVOID) {
    while (s_monitoring) {
        Sleep(3000);
        if (!s_monitoring) break;

        if (!s_patched) {
            PatchNvContainer();          // intenta patch de nuevo
        } else {
            // Verifica que el proceso/DLL sigan vivos
            HANDLE hProcess = OpenProcess(
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                FALSE, s_patchedPID);
            if (hProcess) {
                BYTE* nvdBase = nullptr;
                SIZE_T nvdSize = 0;
                bool stillLoaded = FindRemoteModule(hProcess, L"nvd3dumx.dll",
                                                    nvdBase, nvdSize);
                CloseHandle(hProcess);
                if (!stillLoaded) {
                    // DLL descargada (ShadowPlay apagado): se re-patchea
                    s_patched = false; s_patchedPID = 0;
                    s_patchAddrs1.clear(); s_patchAddrs2.clear();
                }
            } else {
                // Proceso muerto: se re-patchea
                s_patched = false; s_patchedPID = 0;
                s_patchAddrs1.clear(); s_patchAddrs2.clear();
            }
        }
    }
    return 0;
}

} // namespace

// ==================================================================
// API publica
// ==================================================================
void StreamMode_Init() {
    s_enabled = false;
    s_monitoring = false;
    s_patched = false;
    s_patchedPID = 0;
    s_patchAddrs1.clear();
    s_patchAddrs2.clear();
}

void StreamMode_SetEnabled(bool on) {
    if (on == s_enabled) return;
    s_enabled = on;
    if (on) {
        PatchNvContainer();
        s_monitoring = true;
        CreateThread(nullptr, 0, MonitorThread, nullptr, 0, nullptr);
    } else {
        s_monitoring = false;
        UnpatchNvContainer();
    }
}

void StreamMode_Toggle() {
    StreamMode_SetEnabled(!s_enabled);
}

bool StreamMode_IsEnabled() {
    return s_enabled;
}

bool StreamMode_IsHiding() {
    // Oculto todo si: modo activo Y hay app de captura corriendo
    if (!s_enabled) return false;
    return IsCaptureAppRunning();
}

bool StreamMode_IsCaptureAppRunning() {
    return IsCaptureAppRunning();
}

void StreamMode_UpdateWindow(HWND hWnd) {
    if (!hWnd) return;
    if (s_enabled) {
        // Windows excluye esta ventana de toda captura (grabacion,
        // captura de ventana, Discord DDA, etc.)
        SetWindowDisplayAffinity(hWnd, WDA_EXCLUDEFROMCAPTURE);
    } else {
        SetWindowDisplayAffinity(hWnd, WDA_NONE);
    }
}

