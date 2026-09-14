// XPE_Chams.cpp
#include <windows.h>
#include <GL/gl.h>
#include <cstring>

#pragma comment(lib, "opengl32.lib")

// Typedef para la función original
typedef void(APIENTRY *PFNGLDRAWELEMENTS)(GLenum mode, GLsizei count, GLenum type, const void *indices);

// Variables globales
PFNGLDRAWELEMENTS OriginalDrawElements = nullptr;
unsigned char OriginalCode[20];
bool bHooked = false;

// Configuración de colores
const float EnemyColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
const float AllyColor[4]  = { 0.0f, 0.0f, 1.0f, 0.5f };

BOOL bIsEnemy = TRUE;
BOOL bForceDraw = TRUE;

// Función Hook
void APIENTRY HookedDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices)
{
    if (bIsEnemy || bForceDraw)
    {
        GLfloat oldColor[4];
        glGetFloatv(GL_CURRENT_COLOR, oldColor);
        GLint oldDepthFunc;
        glGetIntegerv(GL_DEPTH_FUNC, &oldDepthFunc);
        GLboolean oldDepthMask;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &oldDepthMask);
        GLboolean oldDepthTest;
        glGetBooleanv(GL_DEPTH_TEST, &oldDepthTest);

        glColor4fv(EnemyColor);
        glDisable(GL_DEPTH_TEST);

        // Llamar a la función original
        OriginalDrawElements(mode, count, type, indices);

        glEnable(GL_DEPTH_TEST);
        glColor4fv(oldColor);
        glDepthFunc((GLenum)oldDepthFunc);
        glDepthMask(oldDepthMask);
        if (!oldDepthTest) glDisable(GL_DEPTH_TEST);
        else glEnable(GL_DEPTH_TEST);
    }
    else
    {
        OriginalDrawElements(mode, count, type, indices);
    }
}

// Hook inline simple para x64
void InstallHook()
{
    HMODULE hGL = GetModuleHandleA("opengl32.dll");
    if (!hGL) return;

    PFNGLDRAWELEMENTS pOriginal = (PFNGLDRAWELEMENTS)GetProcAddress(hGL, "glDrawElements");
    if (!pOriginal) return;

    // Guardar función original
    OriginalDrawElements = pOriginal;

    // Cambiar protección de memoria
    DWORD oldProtect;
    VirtualProtect(pOriginal, 20, PAGE_EXECUTE_READWRITE, &oldProtect);

    // Guardar código original
    memcpy(OriginalCode, pOriginal, 14);

    // Crear JMP a nuestra función hook
    unsigned char* pCode = (unsigned char*)pOriginal;

    // mov rax, HookedDrawElements (48 B8)
    pCode[0] = 0x48;
    pCode[1] = 0xB8;
    *(PFNGLDRAWELEMENTS*)(pCode + 2) = HookedDrawElements;

    // jmp rax (FF E0)
    pCode[10] = 0xFF;
    pCode[11] = 0xE0;

    // Rellenar con NOPs
    for (int i = 12; i < 20; i++)
        pCode[i] = 0x90;

    // Restaurar protección
    VirtualProtect(pOriginal, 20, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), pOriginal, 20);

    bHooked = true;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        InstallHook();
    }
    return TRUE;
}
