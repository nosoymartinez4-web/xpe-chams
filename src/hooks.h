/*
 * XPE CHAMS v2 - x64 Detour Hooks (14-byte JMP)
 * Reconstructed from CHAMSMENU.dll
 * Author: xpe.nettt
 */

#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstring>
#include <cstdio>

// ============================================================
// x64 detour hook - 14 byte JMP [RIP+offset]
// ============================================================
#pragma pack(push, 1)
struct DetourHook_x64
{
    BYTE  jmp_opcode;    // 0xFF
    BYTE  jmp_modrm;     // 0x25
    DWORD jmp_offset;    // 0x00000000 (JMP [RIP+0])
    UINT64 target_addr;  // Absolute target address
};
#pragma pack(pop)

class Hook
{
public:
    // Install a 14-byte detour hook on x64
    static bool Install(void* target, void* detour, void** original)
    {
        if (!target || !detour) return false;

        // Allocate trampoline (executable memory)
        BYTE* trampoline = (BYTE*)VirtualAlloc(NULL, 32,
            MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!trampoline) return false;

        // Read original bytes
        BYTE originalBytes[14];
        memcpy(originalBytes, target, 14);

        // Build trampoline: original bytes + JMP back to target+14
        memcpy(trampoline, originalBytes, 14);

        // Add JMP [RIP+0] at trampoline+14 to target+14
        DetourHook_x64* jmpBack = (DetourHook_x64*)(trampoline + 14);
        jmpBack->jmp_opcode = 0xFF;
        jmpBack->jmp_modrm = 0x25;
        jmpBack->jmp_offset = 0;
        jmpBack->target_addr = (UINT64)target + 14;

        // Build detour: JMP [RIP+0] to detour function
        DetourHook_x64 detourJmp;
        detourJmp.jmp_opcode = 0xFF;
        detourJmp.jmp_modrm = 0x25;
        detourJmp.jmp_offset = 0;
        detourJmp.target_addr = (UINT64)detour;

        // Write detour to target
        DWORD oldProtect;
        VirtualProtect(target, 14, PAGE_EXECUTE_READWRITE, &oldProtect);
        memcpy(target, &detourJmp, sizeof(detourJmp));
        VirtualProtect(target, 14, oldProtect, &oldProtect);

        // Flush instruction cache
        FlushInstructionCache(GetCurrentProcess(), target, 14);

        *original = trampoline;
        return true;
    }

    // Uninstall hook
    static bool Uninstall(void* target, void* original)
    {
        if (!target || !original) return false;

        DWORD oldProtect;
        VirtualProtect(target, 14, PAGE_EXECUTE_READWRITE, &oldProtect);
        memcpy(target, original, 14);
        VirtualProtect(target, 14, oldProtect, &oldProtect);

        FlushInstructionCache(GetCurrentProcess(), target, 14);
        return true;
    }

    // Find function address in module
    static void* FindFunction(const char* moduleName, const char* funcName)
    {
        HMODULE hModule = GetModuleHandleA(moduleName);
        if (!hModule)
        {
            hModule = LoadLibraryA(moduleName);
            if (!hModule) return nullptr;
        }

        return (void*)GetProcAddress(hModule, funcName);
    }

    // Find function by ordinal
    static void* FindFunctionOrdinal(const char* moduleName, WORD ordinal)
    {
        HMODULE hModule = GetModuleHandleA(moduleName);
        if (!hModule)
        {
            hModule = LoadLibraryA(moduleName);
            if (!hModule) return nullptr;
        }

        return (void*)GetProcAddress(hModule, (LPCSTR)MAKELONG(ordinal, 0));
    }

    // Create VTable hook
    static bool VTableHook(void** vtable, int index, void* detour, void** original)
    {
        if (!vtable || !detour) return false;

        DWORD oldProtect;
        VirtualProtect(&vtable[index], sizeof(void*), PAGE_READWRITE, &oldProtect);
        *original = vtable[index];
        vtable[index] = detour;
        VirtualProtect(&vtable[index], sizeof(void*), oldProtect, &oldProtect);

        return true;
    }
};