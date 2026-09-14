// XPE_Chams.cpp
#include <windows.h>
#include <GL/gl.h>
#include <detours.h>

#pragma comment(lib, "detours.lib")
#pragma comment(lib, "opengl32.lib")

// Función original de OpenGL
typedef void(APIENTRY *PFNGLDRAWELEMENTS)(GLenum mode, GLsizei count, GLenum type, const void *indices);

// Puntero a la función original
PFNGLDRAWELEMENTS OriginalDrawElements = nullptr;

// Configuración de colores
const float EnemyColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f }; // Rojo Sólido (Atraviesa paredes)
const float AllyColor[4]  = { 0.0f, 0.0f, 1.0f, 0.5f }; // Azul Transparente

// Variables globales de estado
BOOL bIsEnemy = FALSE;
BOOL bForceDraw = FALSE;

// La función HOOK que se llamará en lugar de la original
void APIENTRY HookedDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices)
{
    // Si es un enemigo, aplicamos el color rojo y desactivamos profundidad
    if (bIsEnemy || bForceDraw)
    {
        // Guardamos estado actual
        GLfloat oldColor[4];
        glGetFloatv(GL_CURRENT_COLOR, oldColor);
        GLint oldDepthFunc;
        glGetIntegerv(GL_DEPTH_FUNC, &oldDepthFunc);
        GLboolean oldDepthMask;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &oldDepthMask);
        GLboolean oldDepthTest;
        glGetBooleanv(GL_DEPTH_TEST, &oldDepthTest);

        // Aplicamos Color Rojo Sólido
        glColor4fv(EnemyColor);

        // ATRAVIESA PAREDES: Desactivamos la prueba de profundidad
        glDisable(GL_DEPTH_TEST);

        // Llamamos a la función ORIGINAL (Trampoline)
        OriginalDrawElements(mode, count, type, indices);

        // Restauramos estado
        glEnable(GL_DEPTH_TEST);
        glColor4fv(oldColor);
        glDepthFunc((GLenum)oldDepthFunc);
        glDepthMask(oldDepthMask);
        if (!oldDepthTest) glDisable(GL_DEPTH_TEST);
        else glEnable(GL_DEPTH_TEST);
    }
    else
    {
        // Dibujado normal
        OriginalDrawElements(mode, count, type, indices);
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        // Obtener la dirección de opengl32.dll
        HMODULE hGL = GetModuleHandleA("opengl32.dll");
        if (hGL)
        {
            // Obtener la función original
            OriginalDrawElements = (PFNGLDRAWELEMENTS)GetProcAddress(hGL, "glDrawElements");

            if (OriginalDrawElements)
            {
                // Iniciar la transacción de Detours
                DetourTransactionBegin();
                DetourUpdateThread(GetCurrentThread());

                // Hacer el hook real
                DetourAttach(&(LPVOID&)OriginalDrawElements, HookedDrawElements);

                // Confirmar la transacción
                DetourTransactionCommit();
            }
        }
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH)
    {
        if (OriginalDrawElements)
        {
            // Remover el hook al descargar la DLL
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourDetach(&(LPVOID&)OriginalDrawElements, HookedDrawElements);
            DetourTransactionCommit();
        }
    }

    return TRUE;
}
