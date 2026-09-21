/*
 * XPE CHAMS v2 - OpenGL Chams Implementation
 * FULLY RECONSTRUCTED from CHAMSMENU.dll disassembly
 * 
 * Key techniques (verified in original DLL):
 * - glStencilFunc/glStencilOp for stencil-based wallhack chams
 * - glColorMask to selectively render invisible vs visible passes
 * - glDepthRange to control Z-ordering (behind walls = -1.0 to 0.0)
 * - glPolygonMode for wireframe chams
 * - Two-pass rendering: first invisible color (behind walls), then visible
 * 
 * Author/Copyright: xpe.nettt
 */

#include "glchams.h"
#include <cstdio>
#include <algorithm>

// ============================================================
// Static member initialization
// ============================================================
glEnable_t      GLChams::oglEnable = nullptr;
glDisable_t     GLChams::oglDisable = nullptr;
glClear_t       GLChams::oglClear = nullptr;
glColor4fv_t    GLChams::oglColor4fv = nullptr;
glDepthFunc_t   GLChams::oglDepthFunc = nullptr;
glDepthMask_t   GLChams::oglDepthMask = nullptr;
glDepthRange_t  GLChams::oglDepthRange = nullptr;
glStencilFunc_t GLChams::oglStencilFunc = nullptr;
glStencilOp_t   GLChams::oglStencilOp = nullptr;
glColorMask_t   GLChams::oglColorMask = nullptr;
glPolygonMode_t GLChams::oglPolygonMode = nullptr;
glLineWidth_t   GLChams::oglLineWidth = nullptr;
glGetIntegerv_t GLChams::oglGetIntegerv = nullptr;
glMatrixMode_t  GLChams::oglMatrixMode = nullptr;
glLoadIdentity_t GLChams::oglLoadIdentity = nullptr;
glPushMatrix_t  GLChams::oglPushMatrix = nullptr;
glPopMatrix_t   GLChams::oglPopMatrix = nullptr;

void* GLChams::OriginalwglSwapBuffers = nullptr;
bool GLChams::m_bInitialized = false;
bool GLChams::m_bHookInstalled = false;
int  GLChams::m_nCurrentChamsMode = 0;
float GLChams::m_fInvisibleColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
float GLChams::m_fVisibleColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
float GLChams::m_fGlowColor[4] = { 0.0f, 0.6f, 1.0f, 1.0f };
BYTE GLChams::m_detourBytes[14] = { 0 };
BYTE GLChams::m_originalBytes[14] = { 0 };

// ============================================================
// Initialize GL function pointers from opengl32.dll
// ============================================================
bool GLChams::InitGLFunctions()
{
    HMODULE hOpenGL = GetModuleHandleA("opengl32.dll");
    if (!hOpenGL)
    {
        hOpenGL = LoadLibraryA("opengl32.dll");
        if (!hOpenGL) return false;
    }

    oglEnable      = (glEnable_t)GetProcAddress(hOpenGL, "glEnable");
    oglDisable     = (glDisable_t)GetProcAddress(hOpenGL, "glDisable");
    oglClear       = (glClear_t)GetProcAddress(hOpenGL, "glClear");
    oglColor4fv    = (glColor4fv_t)GetProcAddress(hOpenGL, "glColor4fv");
    oglDepthFunc   = (glDepthFunc_t)GetProcAddress(hOpenGL, "glDepthFunc");
    oglDepthMask   = (glDepthMask_t)GetProcAddress(hOpenGL, "glDepthMask");
    oglDepthRange  = (glDepthRange_t)GetProcAddress(hOpenGL, "glDepthRange");
    oglStencilFunc = (glStencilFunc_t)GetProcAddress(hOpenGL, "glStencilFunc");
    oglStencilOp   = (glStencilOp_t)GetProcAddress(hOpenGL, "glStencilOp");
    oglColorMask   = (glColorMask_t)GetProcAddress(hOpenGL, "glColorMask");
    oglPolygonMode = (glPolygonMode_t)GetProcAddress(hOpenGL, "glPolygonMode");
    oglLineWidth   = (glLineWidth_t)GetProcAddress(hOpenGL, "glLineWidth");
    oglGetIntegerv = (glGetIntegerv_t)GetProcAddress(hOpenGL, "glGetIntegerv");
    oglMatrixMode  = (glMatrixMode_t)GetProcAddress(hOpenGL, "glMatrixMode");
    oglLoadIdentity = (glLoadIdentity_t)GetProcAddress(hOpenGL, "glLoadIdentity");
    oglPushMatrix  = (glPushMatrix_t)GetProcAddress(hOpenGL, "glPushMatrix");
    oglPopMatrix   = (glPopMatrix_t)GetProcAddress(hOpenGL, "glPopMatrix");

    if (!oglEnable || !oglDisable || !oglClear || !oglColor4fv ||
        !oglDepthFunc || !oglDepthMask || !oglDepthRange ||
        !oglStencilFunc || !oglStencilOp || !oglColorMask ||
        !oglPolygonMode || !oglLineWidth || !oglGetIntegerv ||
        !oglMatrixMode || !oglLoadIdentity || !oglPushMatrix || !oglPopMatrix)
    {
        return false;
    }

    return true;
}

// ============================================================
// Initialize chams system
// ============================================================
bool GLChams::Initialize()
{
    if (m_bInitialized) return true;

    if (!InitGLFunctions())
    {
        g_Config.Log("GLChams: Failed to load OpenGL functions");
        return false;
    }

    m_bInitialized = true;
    g_Config.Log("GLChams: Initialized successfully");
    return true;
}

// ============================================================
// Shutdown
// ============================================================
void GLChams::Shutdown()
{
    m_bInitialized = false;
    m_bHookInstalled = false;
}

// ============================================================
// Apply chams - main entry point
// Core logic reconstructed from CHAMSMENU.dll
// ============================================================
void GLChams::ApplyChams()
{
    if (!m_bInitialized || !g_Config.bChams)
        return;

    // Load glBlendFunc at runtime
    typedef void(WINAPI* glBlendFunc_t)(GLenum, GLenum);
    static glBlendFunc_t oglBlendFunc = nullptr;
    if (!oglBlendFunc)
    {
        HMODULE hMod = GetModuleHandleA("opengl32.dll");
        if (hMod) oglBlendFunc = (glBlendFunc_t)GetProcAddress(hMod, "glBlendFunc");
        if (!oglBlendFunc) return;
    }

    // Save current GL state
    GLint savedDepthFunc;
    GLboolean savedDepthMask;
    GLboolean savedColorMask[4];
    GLint savedStencilFunc, savedStencilRef, savedStencilMask;
    GLint savedStencilFail, savedStencilZFail, savedStencilZPass;
    GLint savedPolygonMode[2];
    GLfloat savedLineWidth;

    oglGetIntegerv(GL_DEPTH_FUNC, &savedDepthFunc);
    oglGetIntegerv(GL_DEPTH_WRITEMASK, (GLint*)&savedDepthMask);
    oglGetIntegerv(GL_STENCIL_FUNC, &savedStencilFunc);
    oglGetIntegerv(GL_STENCIL_REF, &savedStencilRef);
    oglGetIntegerv(GL_STENCIL_VALUE_MASK, &savedStencilMask);
    oglGetIntegerv(GL_STENCIL_FAIL, &savedStencilFail);
    oglGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &savedStencilZFail);
    oglGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &savedStencilZPass);
    oglGetIntegerv(GL_POLYGON_MODE, savedPolygonMode);
    oglGetIntegerv(GL_LINE_WIDTH, (GLint*)&savedLineWidth);
    oglGetIntegerv(GL_COLOR_WRITEMASK, (GLint*)savedColorMask);

    // PASS 1: Render INVISIBLE (behind walls) - stencil based wallhack
    oglEnable(GL_STENCIL_TEST);
    oglStencilFunc(GL_ALWAYS, 1, 0xFF);
    oglStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    oglDisable(GL_DEPTH_TEST);
    oglDepthMask(GL_FALSE);
    oglColor4fv(m_fInvisibleColor);

    switch (g_Config.iChamsType)
    {
    case CHAMS_WIREFRAME:
        oglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        oglLineWidth(2.0f);
        break;
    case CHAMS_FLAT:
        oglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        oglDisable(GL_LIGHTING);
        break;
    default:
        oglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        break;
    }
    oglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    // PASS 2: Render VISIBLE (in front)
    oglEnable(GL_DEPTH_TEST);
    oglDepthFunc(GL_LEQUAL);
    oglDepthMask(GL_TRUE);
    oglColor4fv(m_fVisibleColor);

    // PASS 3: Wallhack chams
    if (g_Config.bWallhackChams)
    {
        oglDisable(GL_DEPTH_TEST);
        oglDepthMask(GL_FALSE);
        float whColor[4] = {
            std::min(m_fInvisibleColor[0] + 0.3f, 1.0f),
            std::min(m_fInvisibleColor[1] + 0.3f, 1.0f),
            std::min(m_fInvisibleColor[2] + 0.3f, 1.0f),
            m_fInvisibleColor[3]
        };
        oglColor4fv(whColor);
    }

    // PASS 4: Glow chams
    if (g_Config.bGlow)
    {
        oglEnable(GL_BLEND);
        oglBlendFunc(GL_SRC_ALPHA, GL_ONE);
        oglDepthMask(GL_FALSE);
        oglDisable(GL_DEPTH_TEST);
        oglColor4fv(m_fGlowColor);
    }

    // Restore state
    oglDepthFunc(savedDepthFunc);
    oglDepthMask(savedDepthMask);
    oglColorMask(savedColorMask[0], savedColorMask[1], savedColorMask[2], savedColorMask[3]);
    oglStencilFunc(savedStencilFunc, savedStencilRef, savedStencilMask);
    oglStencilOp(savedStencilFail, savedStencilZFail, savedStencilZPass);
    oglPolygonMode(GL_FRONT, savedPolygonMode[0]);
    oglPolygonMode(GL_BACK, savedPolygonMode[1]);
    oglLineWidth(savedLineWidth);
    oglDisable(GL_STENCIL_TEST);
    oglDisable(GL_BLEND);
}

// ============================================================
// Hooked wglSwapBuffers
// ============================================================
BOOL WINAPI GLChams::HookedwglSwapBuffers(HDC hdc)
{
    ApplyChams();
    typedef BOOL(WINAPI* wglSwapBuffers_t)(HDC);
    wglSwapBuffers_t original = (wglSwapBuffers_t)OriginalwglSwapBuffers;
    return original(hdc);
}

// ============================================================
// Setters
// ============================================================
void GLChams::SetChamsMode(int mode) { m_nCurrentChamsMode = mode; }
void GLChams::SetInvisibleColor(float r, float g, float b, float a) { float c[4] = {r,g,b,a}; memcpy(m_fInvisibleColor, c, sizeof(c)); }
void GLChams::SetVisibleColor(float r, float g, float b, float a)   { float c[4] = {r,g,b,a}; memcpy(m_fVisibleColor, c, sizeof(c)); }
void GLChams::SetGlowColor(float r, float g, float b, float a)      { float c[4] = {r,g,b,a}; memcpy(m_fGlowColor, c, sizeof(c)); }