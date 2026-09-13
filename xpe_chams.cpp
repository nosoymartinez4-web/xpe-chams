Copiar
// XPE_Chams.cpp
#include <windows.h>
#include <GL/gl.h>

// Función original de OpenGL
typedef void(APIENTRY *PFNGLDRAWELEMENTS)(GLenum mode, GLsizei count, GLenum type, const void *indices);
PFNGLDRAWELEMENTS OriginalDrawElements = nullptr;

// Configuración de colores
// 0.0f a 1.0f (0 = mínimo, 1 = máximo)
const float EnemyColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f }; // Rojo Sólido (Atraviesa paredes)
const float AllyColor[4]  = { 0.0f, 0.0f, 1.0f, 0.5f }; // Azul Transparente (Se ve menos, pero indica aliado)

// Variables globles de estado
BOOL bIsEnemy = FALSE;
BOOL bForceDraw = FALSE; // Para debug o fuerza bruta

void APIENTRY HookedDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices)
{
    // 1. Verificamos si es un enemigo (aquí podrías añadir lógica de lectura de memoria)
    // Por ahora, asumimos que todo lo que dibujamos es un objetivo para el ejemplo.
    // Si quieres distinguir aliado/enemigo, necesitas leer la memoria del juego aquí.
    
    // OPCIÓN A: Cham Sólido (Atraviesa paredes - Mejor para bajos recursos y visibilidad)
    // Desactivamos la prueba de profundidad para que se vea TODO
    BOOL bThroughWalls = TRUE; 

    if (bIsEnemy || bForceDraw)
    {
        // Guardamos estado actual
        GLint oldColor[4];
        glGetIntegerv(GL_CURRENT_COLOR, oldColor);
        GLint oldDepthFunc;
        glGetIntegerv(GL_DEPTH_FUNC, &oldDepthFunc);
        GLboolean oldDepthMask;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &oldDepthMask);
        GLboolean oldDepthTest;
        glGetBooleanv(GL_DEPTH_TEST, &oldDepthTest);

        // Aplicamos Color Rojo Sólido
        glColor4fv(EnemyColor);

        // Si es a través de paredes, desactivamos la profundidad
        if (bThroughWalls) {
            glDisable(GL_DEPTH_TEST);
        }

        // Dibujamos
        OriginalDrawElements(mode, count, type, indices);

        // Restauramos estado
        if (bThroughWalls) {
            glEnable(GL_DEPTH_TEST);
        }
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

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        // Inicialización
        HMODULE hGL = GetModuleHandle("opengl32.dll");
        if (hGL)
        {
            OriginalDrawElements = (PFNGLDRAWELEMENTS)GetProcAddress(hGL, "DrawElements");
        }
        
        // Mensaje de confirmación (puedes quitarlo si quieres)
        // MessageBox(NULL, "XPE_Chams Inyectado", "OK", MB_OK);
    }
    return TRUE;
