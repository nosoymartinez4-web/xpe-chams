/*
 * XPE CHAMS v2 - OpenGL Chams (reconstructed from CHAMSMENU.dll)
 * Uses: glStencilFunc, glStencilOp, glColorMask, glDepthRange, glPolygonMode
 * Author: xpe.nettt
 */

#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <vector>
#include "config.h"

// ============================================================
// OpenGL function typedefs
// ============================================================
typedef void(WINAPI* glEnable_t)(GLenum cap);
typedef void(WINAPI* glDisable_t)(GLenum cap);
typedef void(WINAPI* glClear_t)(GLbitfield mask);
typedef void(WINAPI* glColor4fv_t)(const GLfloat* v);
typedef void(WINAPI* glDepthFunc_t)(GLenum func);
typedef void(WINAPI* glDepthMask_t)(GLboolean flag);
typedef void(WINAPI* glDepthRange_t)(GLclampd n, GLclampd f);
typedef void(WINAPI* glStencilFunc_t)(GLenum func, GLint ref, GLuint mask);
typedef void(WINAPI* glStencilOp_t)(GLenum fail, GLenum zfail, GLenum zpass);
typedef void(WINAPI* glColorMask_t)(GLboolean r, GLboolean g, GLboolean b, GLboolean a);
typedef void(WINAPI* glPolygonMode_t)(GLenum face, GLenum mode);
typedef void(WINAPI* glLineWidth_t)(GLfloat width);
typedef void(WINAPI* glGetIntegerv_t)(GLenum pname, GLint* data);
typedef void(WINAPI* glMatrixMode_t)(GLenum mode);
typedef void(WINAPI* glLoadIdentity_t)(void);
typedef void(WINAPI* glPushMatrix_t)(void);
typedef void(WINAPI* glPopMatrix_t)(void);

// ============================================================
// Chams mode flags (matching original DLL)
// ============================================================
enum ChamsMode
{
    CHAMS_DISABLED = 0,
    CHAMS_SHADED = 1,
    CHAMS_FLAT = 2,
    CHAMS_WIREFRAME = 3,
    CHAMS_GLOW = 4
};

// ============================================================
// GLChams class
// ============================================================
class GLChams
{
private:
    // OpenGL function pointers
    static glEnable_t      oglEnable;
    static glDisable_t     oglDisable;
    static glClear_t       oglClear;
    static glColor4fv_t    oglColor4fv;
    static glDepthFunc_t   oglDepthFunc;
    static glDepthMask_t   oglDepthMask;
    static glDepthRange_t  oglDepthRange;
    static glStencilFunc_t oglStencilFunc;
    static glStencilOp_t   oglStencilOp;
    static glColorMask_t   oglColorMask;
    static glPolygonMode_t oglPolygonMode;
    static glLineWidth_t   oglLineWidth;
    static glGetIntegerv_t oglGetIntegerv;
    static glMatrixMode_t  oglMatrixMode;
    static glLoadIdentity_t oglLoadIdentity;
    static glPushMatrix_t  oglPushMatrix;
    static glPopMatrix_t   oglPopMatrix;

    // Hook chain
    static void* OriginalwglSwapBuffers;

    // Chams state
    static bool m_bInitialized;
    static bool m_bHookInstalled;
    static int  m_nCurrentChamsMode;
    static float m_fInvisibleColor[4];
    static float m_fVisibleColor[4];
    static float m_fGlowColor[4];

    // Detour buffer (14 bytes for x64 jmp)
    static BYTE m_detourBytes[14];
    static BYTE m_originalBytes[14];

    // Initialize GL function pointers from opengl32.dll
    static bool InitGLFunctions();

    // Chams rendering logic (reconstructed from original DLL)
    static void ApplyChamsInvisible();
    static void ApplyChamsVisible();
    static void ApplyWallhackChams();
    static void ApplyGlowChams();

public:
    static bool Initialize();
    static void Shutdown();
    static void SetChamsMode(int mode);
    static void SetInvisibleColor(float r, float g, float b, float a);
    static void SetVisibleColor(float r, float g, float b, float a);
    static void SetGlowColor(float r, float g, float b, float a);

    // Hooked wglSwapBuffers
    static BOOL WINAPI HookedwglSwapBuffers(HDC hdc);

    // Apply chams based on current config
    static void ApplyChams();
};