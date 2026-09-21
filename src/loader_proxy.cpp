/*
 * XPE CHAMS v2 - Universal Proxy Loader
 * 
 * Works with: LDPlayer (all versions), BlueStacks, MuMu, Nox
 * 
 * Strategy: Acts as ldopengl32.dll proxy that:
 *   1. Loads the REAL opengl32.dll from system32
 *   2. Forwards ALL OpenGL calls to the real DLL
 *   3. Loads XPE_CHAMS.dll for injection
 * 
 * For LDPlayer: rename to ldopengl32.dll and place in LDPlayer's folder
 * For BlueStacks: rename to opengl32.dll and place in BlueStacks folder
 * 
 * Author/Copyright: xpe.nettt
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstring>

// ============================================================
// Forward declarations for exported functions
// ============================================================
#define EXPORT __declspec(dllexport)

// ============================================================
// Global state
// ============================================================
HMODULE g_hRealOpenGL = NULL;
HMODULE g_hXPEChams = NULL;
bool g_bInitialized = false;

// ============================================================
// Load real opengl32.dll from system directory
// ============================================================
bool LoadRealOpenGL()
{
    if (g_hRealOpenGL)
        return true;

    char systemPath[MAX_PATH];
    GetSystemDirectoryA(systemPath, MAX_PATH);
    strcat_s(systemPath, "\\opengl32.dll");

    g_hRealOpenGL = LoadLibraryA(systemPath);
    if (!g_hRealOpenGL)
    {
        // Fallback: try loading without path
        g_hRealOpenGL = LoadLibraryA("opengl32.dll");
    }

    return (g_hRealOpenGL != NULL);
}

// ============================================================
// Load XPE CHAMS DLL for injection
// ============================================================
bool LoadXPEChams()
{
    if (g_hXPEChams)
        return true;

    // Try loading from current directory
    g_hXPEChams = LoadLibraryA("XPE_CHAMS.dll");
    if (!g_hXPEChams)
    {
        // Try loading from same directory as this proxy DLL
        char proxyPath[MAX_PATH];
        GetModuleFileNameA(GetModuleHandleA("ldopengl32.dll"), proxyPath, MAX_PATH);

        char* lastSlash = strrchr(proxyPath, '\\');
        if (lastSlash)
        {
            *(lastSlash + 1) = 0;
            strcat_s(proxyPath, "XPE_CHAMS.dll");
            g_hXPEChams = LoadLibraryA(proxyPath);
        }
    }

    if (g_hXPEChams)
    {
        // Call initialization export if exists
        FARPROC initFunc = GetProcAddress(g_hXPEChams, "InitializeXPEChams");
        if (initFunc)
        {
            initFunc();
        }
    }

    return (g_hXPEChams != NULL);
}

// ============================================================
// Get real OpenGL function by ordinal
// ============================================================
FARPROC GetRealGLProc(const char* name, WORD ordinal)
{
    if (!g_hRealOpenGL && !LoadRealOpenGL())
        return NULL;

    if (ordinal > 0)
    {
        return GetProcAddress(g_hRealOpenGL, (LPCSTR)MAKELONG(ordinal, 0));
    }

    return GetProcAddress(g_hRealOpenGL, name);
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
        DisableThreadLibraryCalls(hinstDLL);

        // Load real OpenGL first
        if (!LoadRealOpenGL())
        {
            return FALSE;
        }

        // Load XPE CHAMS in a separate thread to not block OpenGL init
        HANDLE hThread = CreateThread(NULL, 0,
            [](LPVOID) -> DWORD {
                Sleep(100); // Wait for OpenGL context to be created
                LoadXPEChams();
                g_bInitialized = true;
                return 0;
            }, NULL, 0, NULL);

        if (hThread) CloseHandle(hThread);

        return TRUE;
    }

    case DLL_PROCESS_DETACH:
    {
        if (g_hXPEChams)
        {
            FreeLibrary(g_hXPEChams);
            g_hXPEChams = NULL;
        }
        if (g_hRealOpenGL)
        {
            FreeLibrary(g_hRealOpenGL);
            g_hRealOpenGL = NULL;
        }
        break;
    }
    }

    return TRUE;
}

// ============================================================
// Proxy functions - Forward ALL OpenGL calls to real DLL
// ============================================================
// These are generated for the most common OpenGL 1.1 functions
// The .def file handles the rest via ordinal forwarding

EXPORT void WINAPI glAccum(GLenum op, GLfloat value)
{
    typedef void(WINAPI* func_t)(GLenum, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glAccum", 0);
    if (real) real(op, value);
}

EXPORT void WINAPI glAlphaFunc(GLenum func, GLclampf ref)
{
    typedef void(WINAPI* func_t)(GLenum, GLclampf);
    static func_t real = (func_t)GetRealGLProc("glAlphaFunc", 0);
    if (real) real(func, ref);
}

EXPORT GLboolean WINAPI glAreTexturesResident(GLsizei n, const GLuint* textures, GLboolean* residences)
{
    typedef GLboolean(WINAPI* func_t)(GLsizei, const GLuint*, GLboolean*);
    static func_t real = (func_t)GetRealGLProc("glAreTexturesResident", 0);
    return real ? real(n, textures, residences) : GL_FALSE;
}

EXPORT void WINAPI glArrayElement(GLint i)
{
    typedef void(WINAPI* func_t)(GLint);
    static func_t real = (func_t)GetRealGLProc("glArrayElement", 0);
    if (real) real(i);
}

EXPORT void WINAPI glBegin(GLenum mode)
{
    typedef void(WINAPI* func_t)(GLenum);
    static func_t real = (func_t)GetRealGLProc("glBegin", 0);
    if (real) real(mode);
}

EXPORT void WINAPI glBindTexture(GLenum target, GLuint texture)
{
    typedef void(WINAPI* func_t)(GLenum, GLuint);
    static func_t real = (func_t)GetRealGLProc("glBindTexture", 0);
    if (real) real(target, texture);
}

EXPORT void WINAPI glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig,
    GLfloat xmove, GLfloat ymove, const GLubyte* bitmap)
{
    typedef void(WINAPI* func_t)(GLsizei, GLsizei, GLfloat, GLfloat, GLfloat, GLfloat, const GLubyte*);
    static func_t real = (func_t)GetRealGLProc("glBitmap", 0);
    if (real) real(width, height, xorig, yorig, xmove, ymove, bitmap);
}

EXPORT void WINAPI glBlendFunc(GLenum sfactor, GLenum dfactor)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum);
    static func_t real = (func_t)GetRealGLProc("glBlendFunc", 0);
    if (real) real(sfactor, dfactor);
}

EXPORT void WINAPI glCallList(GLuint list)
{
    typedef void(WINAPI* func_t)(GLuint);
    static func_t real = (func_t)GetRealGLProc("glCallList", 0);
    if (real) real(list);
}

EXPORT void WINAPI glCallLists(GLsizei n, GLenum type, const GLvoid* lists)
{
    typedef void(WINAPI* func_t)(GLsizei, GLenum, const GLvoid*);
    static func_t real = (func_t)GetRealGLProc("glCallLists", 0);
    if (real) real(n, type, lists);
}

EXPORT void WINAPI glClear(GLbitfield mask)
{
    typedef void(WINAPI* func_t)(GLbitfield);
    static func_t real = (func_t)GetRealGLProc("glClear", 0);
    if (real) real(mask);
}

EXPORT void WINAPI glClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    typedef void(WINAPI* func_t)(GLfloat, GLfloat, GLfloat, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glClearAccum", 0);
    if (real) real(red, green, blue, alpha);
}

EXPORT void WINAPI glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    typedef void(WINAPI* func_t)(GLclampf, GLclampf, GLclampf, GLclampf);
    static func_t real = (func_t)GetRealGLProc("glClearColor", 0);
    if (real) real(red, green, blue, alpha);
}

EXPORT void WINAPI glClearDepth(GLclampd depth)
{
    typedef void(WINAPI* func_t)(GLclampd);
    static func_t real = (func_t)GetRealGLProc("glClearDepth", 0);
    if (real) real(depth);
}

EXPORT void WINAPI glClearIndex(GLfloat c)
{
    typedef void(WINAPI* func_t)(GLfloat);
    static func_t real = (func_t)GetRealGLProc("glClearIndex", 0);
    if (real) real(c);
}

EXPORT void WINAPI glClearStencil(GLint s)
{
    typedef void(WINAPI* func_t)(GLint);
    static func_t real = (func_t)GetRealGLProc("glClearStencil", 0);
    if (real) real(s);
}

EXPORT void WINAPI glClipPlane(GLenum plane, const GLdouble* equation)
{
    typedef void(WINAPI* func_t)(GLenum, const GLdouble*);
    static func_t real = (func_t)GetRealGLProc("glClipPlane", 0);
    if (real) real(plane, equation);
}

EXPORT void WINAPI glColor3b(GLbyte red, GLbyte green, GLbyte blue)
{
    typedef void(WINAPI* func_t)(GLbyte, GLbyte, GLbyte);
    static func_t real = (func_t)GetRealGLProc("glColor3b", 0);
    if (real) real(red, green, blue);
}

EXPORT void WINAPI glColor3bv(const GLbyte* v)
{
    typedef void(WINAPI* func_t)(const GLbyte*);
    static func_t real = (func_t)GetRealGLProc("glColor3bv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glColor3d(GLdouble red, GLdouble green, GLdouble blue)
{
    typedef void(WINAPI* func_t)(GLdouble, GLdouble, GLdouble);
    static func_t real = (func_t)GetRealGLProc("glColor3d", 0);
    if (real) real(red, green, blue);
}

EXPORT void WINAPI glColor3dv(const GLdouble* v)
{
    typedef void(WINAPI* func_t)(const GLdouble*);
    static func_t real = (func_t)GetRealGLProc("glColor3dv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glColor3f(GLfloat red, GLfloat green, GLfloat blue)
{
    typedef void(WINAPI* func_t)(GLfloat, GLfloat, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glColor3f", 0);
    if (real) real(red, green, blue);
}

EXPORT void WINAPI glColor3fv(const GLfloat* v)
{
    typedef void(WINAPI* func_t)(const GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glColor3fv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glColor3i(GLint red, GLint green, GLint blue)
{
    typedef void(WINAPI* func_t)(GLint, GLint, GLint);
    static func_t real = (func_t)GetRealGLProc("glColor3i", 0);
    if (real) real(red, green, blue);
}

EXPORT void WINAPI glColor3iv(const GLint* v)
{
    typedef void(WINAPI* func_t)(const GLint*);
    static func_t real = (func_t)GetRealGLProc("glColor3iv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glColor3s(GLshort red, GLshort green, GLshort blue)
{
    typedef void(WINAPI* func_t)(GLshort, GLshort, GLshort);
    static func_t real = (func_t)GetRealGLProc("glColor3s", 0);
    if (real) real(red, green, blue);
}

EXPORT void WINAPI glColor3sv(const GLshort* v)
{
    typedef void(WINAPI* func_t)(const GLshort*);
    static func_t real = (func_t)GetRealGLProc("glColor3sv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glColor3ub(GLubyte red, GLubyte green, GLubyte blue)
{
    typedef void(WINAPI* func_t)(GLubyte, GLubyte, GLubyte);
    static func_t real = (func_t)GetRealGLProc("glColor3ub", 0);
    if (real) real(red, green, blue);
}

EXPORT void WINAPI glColor3ubv(const GLubyte* v)
{
    typedef void(WINAPI* func_t)(const GLubyte*);
    static func_t real = (func_t)GetRealGLProc("glColor3ubv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glColor3ui(GLuint red, GLuint green, GLuint blue)
{
    typedef void(WINAPI* func_t)(GLuint, GLuint, GLuint);
    static func_t real = (func_t)GetRealGLProc("glColor3ui", 0);
    if (real) real(red, green, blue);
}

EXPORT void WINAPI glColor3uiv(const GLuint* v)
{
    typedef void(WINAPI* func_t)(const GLuint*);
    static func_t real = (func_t)GetRealGLProc("glColor3uiv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glColor3us(GLushort red, GLushort green, GLushort blue)
{
    typedef void(WINAPI* func_t)(GLushort, GLushort, GLushort);
    static func_t real = (func_t)GetRealGLProc("glColor3us", 0);
    if (real) real(red, green, blue);
}

EXPORT void WINAPI glColor3usv(const GLushort* v)
{
    typedef void(WINAPI* func_t)(const GLushort*);
    static func_t real = (func_t)GetRealGLProc("glColor3usv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glColor4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
    typedef void(WINAPI* func_t)(GLbyte, GLbyte, GLbyte, GLbyte);
    static func_t real = (func_t)GetRealGLProc("glColor4b", 0);
    if (real) real(red, green, blue, alpha);
}

EXPORT void WINAPI glColor4bv(const GLbyte* v)
{
    typedef void(WINAPI* func_t)(const GLbyte*);
    static func_t real = (func_t)GetRealGLProc("glColor4bv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
    typedef void(WINAPI* func_t)(GLdouble, GLdouble, GLdouble, GLdouble);
    static func_t real = (func_t)GetRealGLProc("glColor4d", 0);
    if (real) real(red, green, blue, alpha);
}

EXPORT void WINAPI glColor4dv(const GLdouble* v)
{
    typedef void(WINAPI* func_t)(const GLdouble*);
    static func_t real = (func_t)GetRealGLProc("glColor4dv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    typedef void(WINAPI* func_t)(GLfloat, GLfloat, GLfloat, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glColor4f", 0);
    if (real) real(red, green, blue, alpha);
}

EXPORT void WINAPI glColor4fv(const GLfloat* v)
{
    typedef void(WINAPI* func_t)(const GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glColor4fv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glColor4i(GLint red, GLint green, GLint blue, GLint alpha)
{
    typedef void(WINAPI* func_t)(GLint, GLint, GLint, GLint);
    static func_t real = (func_t)GetRealGLProc("glColor4i", 0);
    if (real) real(red, green, blue, alpha);
}

EXPORT void WINAPI glColor4iv(const GLint* v)
{
    typedef void(WINAPI* func_t)(const GLint*);
    static func_t real = (func_t)GetRealGLProc("glColor4iv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glColor4s(GLshort red, GLshort green, GLshort blue, GLshort alpha)
{
    typedef void(WINAPI* func_t)(GLshort, GLshort, GLshort, GLshort);
    static func_t real = (func_t)GetRealGLProc("glColor4s", 0);
    if (real) real(red, green, blue, alpha);
}

EXPORT void WINAPI glColor4sv(const GLshort* v)
{
    typedef void(WINAPI* func_t)(const GLshort*);
    static func_t real = (func_t)GetRealGLProc("glColor4sv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    typedef void(WINAPI* func_t)(GLubyte, GLubyte, GLubyte, GLubyte);
    static func_t real = (func_t)GetRealGLProc("glColor4ub", 0);
    if (real) real(red, green, blue, alpha);
}

EXPORT void WINAPI glColor4ubv(const GLubyte* v)
{
    typedef void(WINAPI* func_t)(const GLubyte*);
    static func_t real = (func_t)GetRealGLProc("glColor4ubv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glColor4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
    typedef void(WINAPI* func_t)(GLuint, GLuint, GLuint, GLuint);
    static func_t real = (func_t)GetRealGLProc("glColor4ui", 0);
    if (real) real(red, green, blue, alpha);
}

EXPORT void WINAPI glColor4uiv(const GLuint* v)
{
    typedef void(WINAPI* func_t)(const GLuint*);
    static func_t real = (func_t)GetRealGLProc("glColor4uiv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glColor4us(GLushort red, GLushort green, GLushort blue, GLushort alpha)
{
    typedef void(WINAPI* func_t)(GLushort, GLushort, GLushort, GLushort);
    static func_t real = (func_t)GetRealGLProc("glColor4us", 0);
    if (real) real(red, green, blue, alpha);
}

EXPORT void WINAPI glColor4usv(const GLushort* v)
{
    typedef void(WINAPI* func_t)(const GLushort*);
    static func_t real = (func_t)GetRealGLProc("glColor4usv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    typedef void(WINAPI* func_t)(GLboolean, GLboolean, GLboolean, GLboolean);
    static func_t real = (func_t)GetRealGLProc("glColorMask", 0);
    if (real) real(red, green, blue, alpha);
}

EXPORT void WINAPI glColorMaterial(GLenum face, GLenum mode)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum);
    static func_t real = (func_t)GetRealGLProc("glColorMaterial", 0);
    if (real) real(face, mode);
}

EXPORT void WINAPI glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer)
{
    typedef void(WINAPI* func_t)(GLint, GLenum, GLsizei, const GLvoid*);
    static func_t real = (func_t)GetRealGLProc("glColorPointer", 0);
    if (real) real(size, type, stride, pointer);
}

EXPORT void WINAPI glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
    typedef void(WINAPI* func_t)(GLint, GLint, GLsizei, GLsizei, GLenum);
    static func_t real = (func_t)GetRealGLProc("glCopyPixels", 0);
    if (real) real(x, y, width, height, type);
}

EXPORT void WINAPI glCopyTexImage1D(GLenum target, GLint level, GLenum internalFormat,
    GLint x, GLint y, GLsizei width, GLint border)
{
    typedef void(WINAPI* func_t)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLint);
    static func_t real = (func_t)GetRealGLProc("glCopyTexImage1D", 0);
    if (real) real(target, level, internalFormat, x, y, width, border);
}

EXPORT void WINAPI glCopyTexImage2D(GLenum target, GLint level, GLenum internalFormat,
    GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    typedef void(WINAPI* func_t)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint);
    static func_t real = (func_t)GetRealGLProc("glCopyTexImage2D", 0);
    if (real) real(target, level, internalFormat, x, y, width, height, border);
}

EXPORT void WINAPI glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset,
    GLint x, GLint y, GLsizei width)
{
    typedef void(WINAPI* func_t)(GLenum, GLint, GLint, GLint, GLint, GLsizei);
    static func_t real = (func_t)GetRealGLProc("glCopyTexSubImage1D", 0);
    if (real) real(target, level, xoffset, x, y, width);
}

EXPORT void WINAPI glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
    GLint x, GLint y, GLsizei width, GLsizei height)
{
    typedef void(WINAPI* func_t)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
    static func_t real = (func_t)GetRealGLProc("glCopyTexSubImage2D", 0);
    if (real) real(target, level, xoffset, yoffset, x, y, width, height);
}

EXPORT void WINAPI glCullFace(GLenum mode)
{
    typedef void(WINAPI* func_t)(GLenum);
    static func_t real = (func_t)GetRealGLProc("glCullFace", 0);
    if (real) real(mode);
}

EXPORT void WINAPI glDeleteLists(GLuint list, GLsizei range)
{
    typedef void(WINAPI* func_t)(GLuint, GLsizei);
    static func_t real = (func_t)GetRealGLProc("glDeleteLists", 0);
    if (real) real(list, range);
}

EXPORT void WINAPI glDeleteTextures(GLsizei n, const GLuint* textures)
{
    typedef void(WINAPI* func_t)(GLsizei, const GLuint*);
    static func_t real = (func_t)GetRealGLProc("glDeleteTextures", 0);
    if (real) real(n, textures);
}

EXPORT void WINAPI glDepthFunc(GLenum func)
{
    typedef void(WINAPI* func_t)(GLenum);
    static func_t real = (func_t)GetRealGLProc("glDepthFunc", 0);
    if (real) real(func);
}

EXPORT void WINAPI glDepthMask(GLboolean flag)
{
    typedef void(WINAPI* func_t)(GLboolean);
    static func_t real = (func_t)GetRealGLProc("glDepthMask", 0);
    if (real) real(flag);
}

EXPORT void WINAPI glDepthRange(GLclampd n, GLclampd f)
{
    typedef void(WINAPI* func_t)(GLclampd, GLclampd);
    static func_t real = (func_t)GetRealGLProc("glDepthRange", 0);
    if (real) real(n, f);
}

EXPORT void WINAPI glDisable(GLenum cap)
{
    typedef void(WINAPI* func_t)(GLenum);
    static func_t real = (func_t)GetRealGLProc("glDisable", 0);
    if (real) real(cap);
}

EXPORT void WINAPI glDisableClientState(GLenum array)
{
    typedef void(WINAPI* func_t)(GLenum);
    static func_t real = (func_t)GetRealGLProc("glDisableClientState", 0);
    if (real) real(array);
}

EXPORT void WINAPI glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    typedef void(WINAPI* func_t)(GLenum, GLint, GLsizei);
    static func_t real = (func_t)GetRealGLProc("glDrawArrays", 0);
    if (real) real(mode, first, count);
}

EXPORT void WINAPI glDrawBuffer(GLenum mode)
{
    typedef void(WINAPI* func_t)(GLenum);
    static func_t real = (func_t)GetRealGLProc("glDrawBuffer", 0);
    if (real) real(mode);
}

EXPORT void WINAPI glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices)
{
    typedef void(WINAPI* func_t)(GLenum, GLsizei, GLenum, const GLvoid*);
    static func_t real = (func_t)GetRealGLProc("glDrawElements", 0);
    if (real) real(mode, count, type, indices);
}

EXPORT void WINAPI glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels)
{
    typedef void(WINAPI* func_t)(GLsizei, GLsizei, GLenum, GLenum, const GLvoid*);
    static func_t real = (func_t)GetRealGLProc("glDrawPixels", 0);
    if (real) real(width, height, format, type, pixels);
}

EXPORT void WINAPI glEdgeFlag(GLboolean flag)
{
    typedef void(WINAPI* func_t)(GLboolean);
    static func_t real = (func_t)GetRealGLProc("glEdgeFlag", 0);
    if (real) real(flag);
}

EXPORT void WINAPI glEdgeFlagPointer(GLsizei stride, const GLvoid* pointer)
{
    typedef void(WINAPI* func_t)(GLsizei, const GLvoid*);
    static func_t real = (func_t)GetRealGLProc("glEdgeFlagPointer", 0);
    if (real) real(stride, pointer);
}

EXPORT void WINAPI glEdgeFlagv(const GLboolean* flag)
{
    typedef void(WINAPI* func_t)(const GLboolean*);
    static func_t real = (func_t)GetRealGLProc("glEdgeFlagv", 0);
    if (real) real(flag);
}

EXPORT void WINAPI glEnable(GLenum cap)
{
    typedef void(WINAPI* func_t)(GLenum);
    static func_t real = (func_t)GetRealGLProc("glEnable", 0);
    if (real) real(cap);
}

EXPORT void WINAPI glEnableClientState(GLenum array)
{
    typedef void(WINAPI* func_t)(GLenum);
    static func_t real = (func_t)GetRealGLProc("glEnableClientState", 0);
    if (real) real(array);
}

EXPORT void WINAPI glEnd(void)
{
    typedef void(WINAPI* func_t)();
    static func_t real = (func_t)GetRealGLProc("glEnd", 0);
    if (real) real();
}

EXPORT void WINAPI glEndList(void)
{
    typedef void(WINAPI* func_t)();
    static func_t real = (func_t)GetRealGLProc("glEndList", 0);
    if (real) real();
}

EXPORT void WINAPI glEvalCoord1d(GLdouble u)
{
    typedef void(WINAPI* func_t)(GLdouble);
    static func_t real = (func_t)GetRealGLProc("glEvalCoord1d", 0);
    if (real) real(u);
}

EXPORT void WINAPI glEvalCoord1f(GLfloat u)
{
    typedef void(WINAPI* func_t)(GLfloat);
    static func_t real = (func_t)GetRealGLProc("glEvalCoord1f", 0);
    if (real) real(u);
}

EXPORT void WINAPI glEvalCoord2d(GLdouble u, GLdouble v)
{
    typedef void(WINAPI* func_t)(GLdouble, GLdouble);
    static func_t real = (func_t)GetRealGLProc("glEvalCoord2d", 0);
    if (real) real(u, v);
}

EXPORT void WINAPI glEvalCoord2f(GLfloat u, GLfloat v)
{
    typedef void(WINAPI* func_t)(GLfloat, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glEvalCoord2f", 0);
    if (real) real(u, v);
}

EXPORT void WINAPI glEvalMesh1(GLenum mode, GLint i1, GLint i2)
{
    typedef void(WINAPI* func_t)(GLenum, GLint, GLint);
    static func_t real = (func_t)GetRealGLProc("glEvalMesh1", 0);
    if (real) real(mode, i1, i2);
}

EXPORT void WINAPI glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
    typedef void(WINAPI* func_t)(GLenum, GLint, GLint, GLint, GLint);
    static func_t real = (func_t)GetRealGLProc("glEvalMesh2", 0);
    if (real) real(mode, i1, i2, j1, j2);
}

EXPORT void WINAPI glEvalPoint1(GLint i)
{
    typedef void(WINAPI* func_t)(GLint);
    static func_t real = (func_t)GetRealGLProc("glEvalPoint1", 0);
    if (real) real(i);
}

EXPORT void WINAPI glEvalPoint2(GLint i, GLint j)
{
    typedef void(WINAPI* func_t)(GLint, GLint);
    static func_t real = (func_t)GetRealGLProc("glEvalPoint2", 0);
    if (real) real(i, j);
}

EXPORT void WINAPI glFeedbackBuffer(GLsizei size, GLenum type, GLfloat* buffer)
{
    typedef void(WINAPI* func_t)(GLsizei, GLenum, GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glFeedbackBuffer", 0);
    if (real) real(size, type, buffer);
}

EXPORT void WINAPI glFinish(void)
{
    typedef void(WINAPI* func_t)();
    static func_t real = (func_t)GetRealGLProc("glFinish", 0);
    if (real) real();
}

EXPORT void WINAPI glFlush(void)
{
    typedef void(WINAPI* func_t)();
    static func_t real = (func_t)GetRealGLProc("glFlush", 0);
    if (real) real();
}

EXPORT void WINAPI glFogf(GLenum pname, GLfloat param)
{
    typedef void(WINAPI* func_t)(GLenum, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glFogf", 0);
    if (real) real(pname, param);
}

EXPORT void WINAPI glFogfv(GLenum pname, const GLfloat* params)
{
    typedef void(WINAPI* func_t)(GLenum, const GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glFogfv", 0);
    if (real) real(pname, params);
}

EXPORT void WINAPI glFogi(GLenum pname, GLint param)
{
    typedef void(WINAPI* func_t)(GLenum, GLint);
    static func_t real = (func_t)GetRealGLProc("glFogi", 0);
    if (real) real(pname, param);
}

EXPORT void WINAPI glFogiv(GLenum pname, const GLint* params)
{
    typedef void(WINAPI* func_t)(GLenum, const GLint*);
    static func_t real = (func_t)GetRealGLProc("glFogiv", 0);
    if (real) real(pname, params);
}

EXPORT void WINAPI glFrontFace(GLenum mode)
{
    typedef void(WINAPI* func_t)(GLenum);
    static func_t real = (func_t)GetRealGLProc("glFrontFace", 0);
    if (real) real(mode);
}

EXPORT void WINAPI glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    typedef void(WINAPI* func_t)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
    static func_t real = (func_t)GetRealGLProc("glFrustum", 0);
    if (real) real(left, right, bottom, top, zNear, zFar);
}

EXPORT GLuint WINAPI glGenLists(GLsizei range)
{
    typedef GLuint(WINAPI* func_t)(GLsizei);
    static func_t real = (func_t)GetRealGLProc("glGenLists", 0);
    return real ? real(range) : 0;
}

EXPORT void WINAPI glGenTextures(GLsizei n, GLuint* textures)
{
    typedef void(WINAPI* func_t)(GLsizei, GLuint*);
    static func_t real = (func_t)GetRealGLProc("glGenTextures", 0);
    if (real) real(n, textures);
}

EXPORT void WINAPI glGetBooleanv(GLenum pname, GLboolean* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLboolean*);
    static func_t real = (func_t)GetRealGLProc("glGetBooleanv", 0);
    if (real) real(pname, params);
}

EXPORT void WINAPI glGetDoublev(GLenum pname, GLdouble* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLdouble*);
    static func_t real = (func_t)GetRealGLProc("glGetDoublev", 0);
    if (real) real(pname, params);
}

EXPORT GLenum WINAPI glGetError(void)
{
    typedef GLenum(WINAPI* func_t)();
    static func_t real = (func_t)GetRealGLProc("glGetError", 0);
    return real ? real() : GL_NO_ERROR;
}

EXPORT void WINAPI glGetFloatv(GLenum pname, GLfloat* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glGetFloatv", 0);
    if (real) real(pname, params);
}

EXPORT void WINAPI glGetIntegerv(GLenum pname, GLint* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLint*);
    static func_t real = (func_t)GetRealGLProc("glGetIntegerv", 0);
    if (real) real(pname, params);
}

EXPORT void WINAPI glGetLightfv(GLenum light, GLenum pname, GLfloat* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glGetLightfv", 0);
    if (real) real(light, pname, params);
}

EXPORT void WINAPI glGetLightiv(GLenum light, GLenum pname, GLint* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, GLint*);
    static func_t real = (func_t)GetRealGLProc("glGetLightiv", 0);
    if (real) real(light, pname, params);
}

EXPORT void WINAPI glGetMapdv(GLenum target, GLenum query, GLdouble* v)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, GLdouble*);
    static func_t real = (func_t)GetRealGLProc("glGetMapdv", 0);
    if (real) real(target, query, v);
}

EXPORT void WINAPI glGetMapfv(GLenum target, GLenum query, GLfloat* v)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glGetMapfv", 0);
    if (real) real(target, query, v);
}

EXPORT void WINAPI glGetMapiv(GLenum target, GLenum query, GLint* v)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, GLint*);
    static func_t real = (func_t)GetRealGLProc("glGetMapiv", 0);
    if (real) real(target, query, v);
}

EXPORT void WINAPI glGetMaterialfv(GLenum face, GLenum pname, GLfloat* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glGetMaterialfv", 0);
    if (real) real(face, pname, params);
}

EXPORT void WINAPI glGetMaterialiv(GLenum face, GLenum pname, GLint* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, GLint*);
    static func_t real = (func_t)GetRealGLProc("glGetMaterialiv", 0);
    if (real) real(face, pname, params);
}

EXPORT void WINAPI glGetPixelMapfv(GLenum map, GLfloat* values)
{
    typedef void(WINAPI* func_t)(GLenum, GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glGetPixelMapfv", 0);
    if (real) real(map, values);
}

EXPORT void WINAPI glGetPixelMapuiv(GLenum map, GLuint* values)
{
    typedef void(WINAPI* func_t)(GLenum, GLuint*);
    static func_t real = (func_t)GetRealGLProc("glGetPixelMapuiv", 0);
    if (real) real(map, values);
}

EXPORT void WINAPI glGetPixelMapusv(GLenum map, GLushort* values)
{
    typedef void(WINAPI* func_t)(GLenum, GLushort*);
    static func_t real = (func_t)GetRealGLProc("glGetPixelMapusv", 0);
    if (real) real(map, values);
}

EXPORT void WINAPI glGetPointerv(GLenum pname, GLvoid** params)
{
    typedef void(WINAPI* func_t)(GLenum, GLvoid**);
    static func_t real = (func_t)GetRealGLProc("glGetPointerv", 0);
    if (real) real(pname, params);
}

EXPORT void WINAPI glGetPolygonStipple(GLubyte* mask)
{
    typedef void(WINAPI* func_t)(GLubyte*);
    static func_t real = (func_t)GetRealGLProc("glGetPolygonStipple", 0);
    if (real) real(mask);
}

EXPORT const GLubyte* WINAPI glGetString(GLenum name)
{
    typedef const GLubyte*(WINAPI* func_t)(GLenum);
    static func_t real = (func_t)GetRealGLProc("glGetString", 0);
    return real ? real(name) : NULL;
}

EXPORT void WINAPI glGetTexEnvfv(GLenum target, GLenum pname, GLfloat* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glGetTexEnvfv", 0);
    if (real) real(target, pname, params);
}

EXPORT void WINAPI glGetTexEnviv(GLenum target, GLenum pname, GLint* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, GLint*);
    static func_t real = (func_t)GetRealGLProc("glGetTexEnviv", 0);
    if (real) real(target, pname, params);
}

EXPORT void WINAPI glGetTexGendv(GLenum coord, GLenum pname, GLdouble* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, GLdouble*);
    static func_t real = (func_t)GetRealGLProc("glGetTexGendv", 0);
    if (real) real(coord, pname, params);
}

EXPORT void WINAPI glGetTexGenfv(GLenum coord, GLenum pname, GLfloat* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glGetTexGenfv", 0);
    if (real) real(coord, pname, params);
}

EXPORT void WINAPI glGetTexGeniv(GLenum coord, GLenum pname, GLint* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, GLint*);
    static func_t real = (func_t)GetRealGLProc("glGetTexGeniv", 0);
    if (real) real(coord, pname, params);
}

EXPORT void WINAPI glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid* pixels)
{
    typedef void(WINAPI* func_t)(GLenum, GLint, GLenum, GLenum, GLvoid*);
    static func_t real = (func_t)GetRealGLProc("glGetTexImage", 0);
    if (real) real(target, level, format, type, pixels);
}

EXPORT void WINAPI glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLint, GLenum, GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glGetTexLevelParameterfv", 0);
    if (real) real(target, level, pname, params);
}

EXPORT void WINAPI glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLint, GLenum, GLint*);
    static func_t real = (func_t)GetRealGLProc("glGetTexLevelParameteriv", 0);
    if (real) real(target, level, pname, params);
}

EXPORT void WINAPI glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glGetTexParameterfv", 0);
    if (real) real(target, pname, params);
}

EXPORT void WINAPI glGetTexParameteriv(GLenum target, GLenum pname, GLint* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, GLint*);
    static func_t real = (func_t)GetRealGLProc("glGetTexParameteriv", 0);
    if (real) real(target, pname, params);
}

EXPORT void WINAPI glHint(GLenum target, GLenum mode)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum);
    static func_t real = (func_t)GetRealGLProc("glHint", 0);
    if (real) real(target, mode);
}

EXPORT void WINAPI glIndexMask(GLuint mask)
{
    typedef void(WINAPI* func_t)(GLuint);
    static func_t real = (func_t)GetRealGLProc("glIndexMask", 0);
    if (real) real(mask);
}

EXPORT void WINAPI glIndexPointer(GLenum type, GLsizei stride, const GLvoid* pointer)
{
    typedef void(WINAPI* func_t)(GLenum, GLsizei, const GLvoid*);
    static func_t real = (func_t)GetRealGLProc("glIndexPointer", 0);
    if (real) real(type, stride, pointer);
}

EXPORT void WINAPI glIndexd(GLdouble c)
{
    typedef void(WINAPI* func_t)(GLdouble);
    static func_t real = (func_t)GetRealGLProc("glIndexd", 0);
    if (real) real(c);
}

EXPORT void WINAPI glIndexdv(const GLdouble* c)
{
    typedef void(WINAPI* func_t)(const GLdouble*);
    static func_t real = (func_t)GetRealGLProc("glIndexdv", 0);
    if (real) real(c);
}

EXPORT void WINAPI glIndexf(GLfloat c)
{
    typedef void(WINAPI* func_t)(GLfloat);
    static func_t real = (func_t)GetRealGLProc("glIndexf", 0);
    if (real) real(c);
}

EXPORT void WINAPI glIndexfv(const GLfloat* c)
{
    typedef void(WINAPI* func_t)(const GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glIndexfv", 0);
    if (real) real(c);
}

EXPORT void WINAPI glIndexi(GLint c)
{
    typedef void(WINAPI* func_t)(GLint);
    static func_t real = (func_t)GetRealGLProc("glIndexi", 0);
    if (real) real(c);
}

EXPORT void WINAPI glIndexiv(const GLint* c)
{
    typedef void(WINAPI* func_t)(const GLint*);
    static func_t real = (func_t)GetRealGLProc("glIndexiv", 0);
    if (real) real(c);
}

EXPORT void WINAPI glIndexs(GLshort c)
{
    typedef void(WINAPI* func_t)(GLshort);
    static func_t real = (func_t)GetRealGLProc("glIndexs", 0);
    if (real) real(c);
}

EXPORT void WINAPI glIndexsv(const GLshort* c)
{
    typedef void(WINAPI* func_t)(const GLshort*);
    static func_t real = (func_t)GetRealGLProc("glIndexsv", 0);
    if (real) real(c);
}

EXPORT void WINAPI glIndexub(GLubyte c)
{
    typedef void(WINAPI* func_t)(GLubyte);
    static func_t real = (func_t)GetRealGLProc("glIndexub", 0);
    if (real) real(c);
}

EXPORT void WINAPI glIndexubv(const GLubyte* c)
{
    typedef void(WINAPI* func_t)(const GLubyte*);
    static func_t real = (func_t)GetRealGLProc("glIndexubv", 0);
    if (real) real(c);
}

EXPORT void WINAPI glInitNames(void)
{
    typedef void(WINAPI* func_t)();
    static func_t real = (func_t)GetRealGLProc("glInitNames", 0);
    if (real) real();
}

EXPORT void WINAPI glInterleavedArrays(GLenum format, GLsizei stride, const GLvoid* pointer)
{
    typedef void(WINAPI* func_t)(GLenum, GLsizei, const GLvoid*);
    static func_t real = (func_t)GetRealGLProc("glInterleavedArrays", 0);
    if (real) real(format, stride, pointer);
}

EXPORT GLboolean WINAPI glIsEnabled(GLenum cap)
{
    typedef GLboolean(WINAPI* func_t)(GLenum);
    static func_t real = (func_t)GetRealGLProc("glIsEnabled", 0);
    return real ? real(cap) : GL_FALSE;
}

EXPORT GLboolean WINAPI glIsList(GLuint list)
{
    typedef GLboolean(WINAPI* func_t)(GLuint);
    static func_t real = (func_t)GetRealGLProc("glIsList", 0);
    return real ? real(list) : GL_FALSE;
}

EXPORT GLboolean WINAPI glIsTexture(GLuint texture)
{
    typedef GLboolean(WINAPI* func_t)(GLuint);
    static func_t real = (func_t)GetRealGLProc("glIsTexture", 0);
    return real ? real(texture) : GL_FALSE;
}

EXPORT void WINAPI glLightModelf(GLenum pname, GLfloat param)
{
    typedef void(WINAPI* func_t)(GLenum, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glLightModelf", 0);
    if (real) real(pname, param);
}

EXPORT void WINAPI glLightModelfv(GLenum pname, const GLfloat* params)
{
    typedef void(WINAPI* func_t)(GLenum, const GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glLightModelfv", 0);
    if (real) real(pname, params);
}

EXPORT void WINAPI glLightModeli(GLenum pname, GLint param)
{
    typedef void(WINAPI* func_t)(GLenum, GLint);
    static func_t real = (func_t)GetRealGLProc("glLightModeli", 0);
    if (real) real(pname, param);
}

EXPORT void WINAPI glLightModeliv(GLenum pname, const GLint* params)
{
    typedef void(WINAPI* func_t)(GLenum, const GLint*);
    static func_t real = (func_t)GetRealGLProc("glLightModeliv", 0);
    if (real) real(pname, params);
}

EXPORT void WINAPI glLightf(GLenum light, GLenum pname, GLfloat param)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glLightf", 0);
    if (real) real(light, pname, param);
}

EXPORT void WINAPI glLightfv(GLenum light, GLenum pname, const GLfloat* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, const GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glLightfv", 0);
    if (real) real(light, pname, params);
}

EXPORT void WINAPI glLighti(GLenum light, GLenum pname, GLint param)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, GLint);
    static func_t real = (func_t)GetRealGLProc("glLighti", 0);
    if (real) real(light, pname, param);
}

EXPORT void WINAPI glLightiv(GLenum light, GLenum pname, const GLint* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, const GLint*);
    static func_t real = (func_t)GetRealGLProc("glLightiv", 0);
    if (real) real(light, pname, params);
}

EXPORT void WINAPI glLineStipple(GLint factor, GLushort pattern)
{
    typedef void(WINAPI* func_t)(GLint, GLushort);
    static func_t real = (func_t)GetRealGLProc("glLineStipple", 0);
    if (real) real(factor, pattern);
}

EXPORT void WINAPI glLineWidth(GLfloat width)
{
    typedef void(WINAPI* func_t)(GLfloat);
    static func_t real = (func_t)GetRealGLProc("glLineWidth", 0);
    if (real) real(width);
}

EXPORT void WINAPI glListBase(GLuint base)
{
    typedef void(WINAPI* func_t)(GLuint);
    static func_t real = (func_t)GetRealGLProc("glListBase", 0);
    if (real) real(base);
}

EXPORT void WINAPI glLoadIdentity(void)
{
    typedef void(WINAPI* func_t)();
    static func_t real = (func_t)GetRealGLProc("glLoadIdentity", 0);
    if (real) real();
}

EXPORT void WINAPI glLoadMatrixd(const GLdouble* m)
{
    typedef void(WINAPI* func_t)(const GLdouble*);
    static func_t real = (func_t)GetRealGLProc("glLoadMatrixd", 0);
    if (real) real(m);
}

EXPORT void WINAPI glLoadMatrixf(const GLfloat* m)
{
    typedef void(WINAPI* func_t)(const GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glLoadMatrixf", 0);
    if (real) real(m);
}

EXPORT void WINAPI glLoadName(GLuint name)
{
    typedef void(WINAPI* func_t)(GLuint);
    static func_t real = (func_t)GetRealGLProc("glLoadName", 0);
    if (real) real(name);
}

EXPORT void WINAPI glLogicOp(GLenum opcode)
{
    typedef void(WINAPI* func_t)(GLenum);
    static func_t real = (func_t)GetRealGLProc("glLogicOp", 0);
    if (real) real(opcode);
}

EXPORT void WINAPI glMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble* points)
{
    typedef void(WINAPI* func_t)(GLenum, GLdouble, GLdouble, GLint, GLint, const GLdouble*);
    static func_t real = (func_t)GetRealGLProc("glMap1d", 0);
    if (real) real(target, u1, u2, stride, order, points);
}

EXPORT void WINAPI glMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat* points)
{
    typedef void(WINAPI* func_t)(GLenum, GLfloat, GLfloat, GLint, GLint, const GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glMap1f", 0);
    if (real) real(target, u1, u2, stride, order, points);
}

EXPORT void WINAPI glMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder,
    GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble* points)
{
    typedef void(WINAPI* func_t)(GLenum, GLdouble, GLdouble, GLint, GLint, GLdouble, GLdouble, GLint, GLint, const GLdouble*);
    static func_t real = (func_t)GetRealGLProc("glMap2d", 0);
    if (real) real(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

EXPORT void WINAPI glMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
    GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat* points)
{
    typedef void(WINAPI* func_t)(GLenum, GLfloat, GLfloat, GLint, GLint, GLfloat, GLfloat, GLint, GLint, const GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glMap2f", 0);
    if (real) real(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

EXPORT void WINAPI glMapGrid1d(GLint un, GLdouble u1, GLdouble u2)
{
    typedef void(WINAPI* func_t)(GLint, GLdouble, GLdouble);
    static func_t real = (func_t)GetRealGLProc("glMapGrid1d", 0);
    if (real) real(un, u1, u2);
}

EXPORT void WINAPI glMapGrid1f(GLint un, GLfloat u1, GLfloat u2)
{
    typedef void(WINAPI* func_t)(GLint, GLfloat, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glMapGrid1f", 0);
    if (real) real(un, u1, u2);
}

EXPORT void WINAPI glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
    typedef void(WINAPI* func_t)(GLint, GLdouble, GLdouble, GLint, GLdouble, GLdouble);
    static func_t real = (func_t)GetRealGLProc("glMapGrid2d", 0);
    if (real) real(un, u1, u2, vn, v1, v2);
}

EXPORT void WINAPI glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
    typedef void(WINAPI* func_t)(GLint, GLfloat, GLfloat, GLint, GLfloat, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glMapGrid2f", 0);
    if (real) real(un, u1, u2, vn, v1, v2);
}

EXPORT void WINAPI glMaterialf(GLenum face, GLenum pname, GLfloat param)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glMaterialf", 0);
    if (real) real(face, pname, param);
}

EXPORT void WINAPI glMaterialfv(GLenum face, GLenum pname, const GLfloat* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, const GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glMaterialfv", 0);
    if (real) real(face, pname, params);
}

EXPORT void WINAPI glMateriali(GLenum face, GLenum pname, GLint param)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, GLint);
    static func_t real = (func_t)GetRealGLProc("glMateriali", 0);
    if (real) real(face, pname, param);
}

EXPORT void WINAPI glMaterialiv(GLenum face, GLenum pname, const GLint* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, const GLint*);
    static func_t real = (func_t)GetRealGLProc("glMaterialiv", 0);
    if (real) real(face, pname, params);
}

EXPORT void WINAPI glMatrixMode(GLenum mode)
{
    typedef void(WINAPI* func_t)(GLenum);
    static func_t real = (func_t)GetRealGLProc("glMatrixMode", 0);
    if (real) real(mode);
}

EXPORT void WINAPI glMultMatrixd(const GLdouble* m)
{
    typedef void(WINAPI* func_t)(const GLdouble*);
    static func_t real = (func_t)GetRealGLProc("glMultMatrixd", 0);
    if (real) real(m);
}

EXPORT void WINAPI glMultMatrixf(const GLfloat* m)
{
    typedef void(WINAPI* func_t)(const GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glMultMatrixf", 0);
    if (real) real(m);
}

EXPORT void WINAPI glNewList(GLuint list, GLenum mode)
{
    typedef void(WINAPI* func_t)(GLuint, GLenum);
    static func_t real = (func_t)GetRealGLProc("glNewList", 0);
    if (real) real(list, mode);
}

EXPORT void WINAPI glNormal3b(GLbyte nx, GLbyte ny, GLbyte nz)
{
    typedef void(WINAPI* func_t)(GLbyte, GLbyte, GLbyte);
    static func_t real = (func_t)GetRealGLProc("glNormal3b", 0);
    if (real) real(nx, ny, nz);
}

EXPORT void WINAPI glNormal3bv(const GLbyte* v)
{
    typedef void(WINAPI* func_t)(const GLbyte*);
    static func_t real = (func_t)GetRealGLProc("glNormal3bv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz)
{
    typedef void(WINAPI* func_t)(GLdouble, GLdouble, GLdouble);
    static func_t real = (func_t)GetRealGLProc("glNormal3d", 0);
    if (real) real(nx, ny, nz);
}

EXPORT void WINAPI glNormal3dv(const GLdouble* v)
{
    typedef void(WINAPI* func_t)(const GLdouble*);
    static func_t real = (func_t)GetRealGLProc("glNormal3dv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
    typedef void(WINAPI* func_t)(GLfloat, GLfloat, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glNormal3f", 0);
    if (real) real(nx, ny, nz);
}

EXPORT void WINAPI glNormal3fv(const GLfloat* v)
{
    typedef void(WINAPI* func_t)(const GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glNormal3fv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glNormal3i(GLint nx, GLint ny, GLint nz)
{
    typedef void(WINAPI* func_t)(GLint, GLint, GLint);
    static func_t real = (func_t)GetRealGLProc("glNormal3i", 0);
    if (real) real(nx, ny, nz);
}

EXPORT void WINAPI glNormal3iv(const GLint* v)
{
    typedef void(WINAPI* func_t)(const GLint*);
    static func_t real = (func_t)GetRealGLProc("glNormal3iv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glNormal3s(GLshort nx, GLshort ny, GLshort nz)
{
    typedef void(WINAPI* func_t)(GLshort, GLshort, GLshort);
    static func_t real = (func_t)GetRealGLProc("glNormal3s", 0);
    if (real) real(nx, ny, nz);
}

EXPORT void WINAPI glNormal3sv(const GLshort* v)
{
    typedef void(WINAPI* func_t)(const GLshort*);
    static func_t real = (func_t)GetRealGLProc("glNormal3sv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glNormalPointer(GLenum type, GLsizei stride, const GLvoid* pointer)
{
    typedef void(WINAPI* func_t)(GLenum, GLsizei, const GLvoid*);
    static func_t real = (func_t)GetRealGLProc("glNormalPointer", 0);
    if (real) real(type, stride, pointer);
}

EXPORT void WINAPI glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    typedef void(WINAPI* func_t)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
    static func_t real = (func_t)GetRealGLProc("glOrtho", 0);
    if (real) real(left, right, bottom, top, zNear, zFar);
}

EXPORT void WINAPI glPassThrough(GLfloat token)
{
    typedef void(WINAPI* func_t)(GLfloat);
    static func_t real = (func_t)GetRealGLProc("glPassThrough", 0);
    if (real) real(token);
}

EXPORT void WINAPI glPixelMapfv(GLenum map, GLsizei mapsize, const GLfloat* values)
{
    typedef void(WINAPI* func_t)(GLenum, GLsizei, const GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glPixelMapfv", 0);
    if (real) real(map, mapsize, values);
}

EXPORT void WINAPI glPixelMapuiv(GLenum map, GLsizei mapsize, const GLuint* values)
{
    typedef void(WINAPI* func_t)(GLenum, GLsizei, const GLuint*);
    static func_t real = (func_t)GetRealGLProc("glPixelMapuiv", 0);
    if (real) real(map, mapsize, values);
}

EXPORT void WINAPI glPixelMapusv(GLenum map, GLsizei mapsize, const GLushort* values)
{
    typedef void(WINAPI* func_t)(GLenum, GLsizei, const GLushort*);
    static func_t real = (func_t)GetRealGLProc("glPixelMapusv", 0);
    if (real) real(map, mapsize, values);
}

EXPORT void WINAPI glPixelStoref(GLenum pname, GLfloat param)
{
    typedef void(WINAPI* func_t)(GLenum, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glPixelStoref", 0);
    if (real) real(pname, param);
}

EXPORT void WINAPI glPixelStorei(GLenum pname, GLint param)
{
    typedef void(WINAPI* func_t)(GLenum, GLint);
    static func_t real = (func_t)GetRealGLProc("glPixelStorei", 0);
    if (real) real(pname, param);
}

EXPORT void WINAPI glPixelTransferf(GLenum pname, GLfloat param)
{
    typedef void(WINAPI* func_t)(GLenum, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glPixelTransferf", 0);
    if (real) real(pname, param);
}

EXPORT void WINAPI glPixelTransferi(GLenum pname, GLint param)
{
    typedef void(WINAPI* func_t)(GLenum, GLint);
    static func_t real = (func_t)GetRealGLProc("glPixelTransferi", 0);
    if (real) real(pname, param);
}

EXPORT void WINAPI glPixelZoom(GLfloat xfactor, GLfloat yfactor)
{
    typedef void(WINAPI* func_t)(GLfloat, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glPixelZoom", 0);
    if (real) real(xfactor, yfactor);
}

EXPORT void WINAPI glPointSize(GLfloat size)
{
    typedef void(WINAPI* func_t)(GLfloat);
    static func_t real = (func_t)GetRealGLProc("glPointSize", 0);
    if (real) real(size);
}

EXPORT void WINAPI glPolygonMode(GLenum face, GLenum mode)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum);
    static func_t real = (func_t)GetRealGLProc("glPolygonMode", 0);
    if (real) real(face, mode);
}

EXPORT void WINAPI glPolygonOffset(GLfloat factor, GLfloat units)
{
    typedef void(WINAPI* func_t)(GLfloat, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glPolygonOffset", 0);
    if (real) real(factor, units);
}

EXPORT void WINAPI glPolygonStipple(const GLubyte* mask)
{
    typedef void(WINAPI* func_t)(const GLubyte*);
    static func_t real = (func_t)GetRealGLProc("glPolygonStipple", 0);
    if (real) real(mask);
}

EXPORT void WINAPI glPopAttrib(void)
{
    typedef void(WINAPI* func_t)();
    static func_t real = (func_t)GetRealGLProc("glPopAttrib", 0);
    if (real) real();
}

EXPORT void WINAPI glPopClientAttrib(void)
{
    typedef void(WINAPI* func_t)();
    static func_t real = (func_t)GetRealGLProc("glPopClientAttrib", 0);
    if (real) real();
}

EXPORT void WINAPI glPopMatrix(void)
{
    typedef void(WINAPI* func_t)();
    static func_t real = (func_t)GetRealGLProc("glPopMatrix", 0);
    if (real) real();
}

EXPORT void WINAPI glPopName(void)
{
    typedef void(WINAPI* func_t)();
    static func_t real = (func_t)GetRealGLProc("glPopName", 0);
    if (real) real();
}

EXPORT void WINAPI glPrioritizeTextures(GLsizei n, const GLuint* textures, const GLclampf* priorities)
{
    typedef void(WINAPI* func_t)(GLsizei, const GLuint*, const GLclampf*);
    static func_t real = (func_t)GetRealGLProc("glPrioritizeTextures", 0);
    if (real) real(n, textures, priorities);
}

EXPORT void WINAPI glPushAttrib(GLbitfield mask)
{
    typedef void(WINAPI* func_t)(GLbitfield);
    static func_t real = (func_t)GetRealGLProc("glPushAttrib", 0);
    if (real) real(mask);
}

EXPORT void WINAPI glPushClientAttrib(GLbitfield mask)
{
    typedef void(WINAPI* func_t)(GLbitfield);
    static func_t real = (func_t)GetRealGLProc("glPushClientAttrib", 0);
    if (real) real(mask);
}

EXPORT void WINAPI glPushMatrix(void)
{
    typedef void(WINAPI* func_t)();
    static func_t real = (func_t)GetRealGLProc("glPushMatrix", 0);
    if (real) real();
}

EXPORT void WINAPI glPushName(GLuint name)
{
    typedef void(WINAPI* func_t)(GLuint);
    static func_t real = (func_t)GetRealGLProc("glPushName", 0);
    if (real) real(name);
}

EXPORT void WINAPI glRasterPos2d(GLdouble x, GLdouble y)
{
    typedef void(WINAPI* func_t)(GLdouble, GLdouble);
    static func_t real = (func_t)GetRealGLProc("glRasterPos2d", 0);
    if (real) real(x, y);
}

EXPORT void WINAPI glRasterPos2f(GLfloat x, GLfloat y)
{
    typedef void(WINAPI* func_t)(GLfloat, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glRasterPos2f", 0);
    if (real) real(x, y);
}

EXPORT void WINAPI glRasterPos2i(GLint x, GLint y)
{
    typedef void(WINAPI* func_t)(GLint, GLint);
    static func_t real = (func_t)GetRealGLProc("glRasterPos2i", 0);
    if (real) real(x, y);
}

EXPORT void WINAPI glRasterPos2s(GLshort x, GLshort y)
{
    typedef void(WINAPI* func_t)(GLshort, GLshort);
    static func_t real = (func_t)GetRealGLProc("glRasterPos2s", 0);
    if (real) real(x, y);
}

EXPORT void WINAPI glRasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
    typedef void(WINAPI* func_t)(GLdouble, GLdouble, GLdouble);
    static func_t real = (func_t)GetRealGLProc("glRasterPos3d", 0);
    if (real) real(x, y, z);
}

EXPORT void WINAPI glRasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
    typedef void(WINAPI* func_t)(GLfloat, GLfloat, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glRasterPos3f", 0);
    if (real) real(x, y, z);
}

EXPORT void WINAPI glRasterPos3i(GLint x, GLint y, GLint z)
{
    typedef void(WINAPI* func_t)(GLint, GLint, GLint);
    static func_t real = (func_t)GetRealGLProc("glRasterPos3i", 0);
    if (real) real(x, y, z);
}

EXPORT void WINAPI glRasterPos3s(GLshort x, GLshort y, GLshort z)
{
    typedef void(WINAPI* func_t)(GLshort, GLshort, GLshort);
    static func_t real = (func_t)GetRealGLProc("glRasterPos3s", 0);
    if (real) real(x, y, z);
}

EXPORT void WINAPI glRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    typedef void(WINAPI* func_t)(GLdouble, GLdouble, GLdouble, GLdouble);
    static func_t real = (func_t)GetRealGLProc("glRasterPos4d", 0);
    if (real) real(x, y, z, w);
}

EXPORT void WINAPI glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    typedef void(WINAPI* func_t)(GLfloat, GLfloat, GLfloat, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glRasterPos4f", 0);
    if (real) real(x, y, z, w);
}

EXPORT void WINAPI glRasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
    typedef void(WINAPI* func_t)(GLint, GLint, GLint, GLint);
    static func_t real = (func_t)GetRealGLProc("glRasterPos4i", 0);
    if (real) real(x, y, z, w);
}

EXPORT void WINAPI glRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
    typedef void(WINAPI* func_t)(GLshort, GLshort, GLshort, GLshort);
    static func_t real = (func_t)GetRealGLProc("glRasterPos4s", 0);
    if (real) real(x, y, z, w);
}

EXPORT void WINAPI glReadBuffer(GLenum mode)
{
    typedef void(WINAPI* func_t)(GLenum);
    static func_t real = (func_t)GetRealGLProc("glReadBuffer", 0);
    if (real) real(mode);
}

EXPORT void WINAPI glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels)
{
    typedef void(WINAPI* func_t)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid*);
    static func_t real = (func_t)GetRealGLProc("glReadPixels", 0);
    if (real) real(x, y, width, height, format, type, pixels);
}

EXPORT void WINAPI glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
    typedef void(WINAPI* func_t)(GLdouble, GLdouble, GLdouble, GLdouble);
    static func_t real = (func_t)GetRealGLProc("glRectd", 0);
    if (real) real(x1, y1, x2, y2);
}

EXPORT void WINAPI glRectdv(const GLdouble* v1, const GLdouble* v2)
{
    typedef void(WINAPI* func_t)(const GLdouble*, const GLdouble*);
    static func_t real = (func_t)GetRealGLProc("glRectdv", 0);
    if (real) real(v1, v2);
}

EXPORT void WINAPI glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    typedef void(WINAPI* func_t)(GLfloat, GLfloat, GLfloat, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glRectf", 0);
    if (real) real(x1, y1, x2, y2);
}

EXPORT void WINAPI glRectfv(const GLfloat* v1, const GLfloat* v2)
{
    typedef void(WINAPI* func_t)(const GLfloat*, const GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glRectfv", 0);
    if (real) real(v1, v2);
}

EXPORT void WINAPI glRecti(GLint x1, GLint y1, GLint x2, GLint y2)
{
    typedef void(WINAPI* func_t)(GLint, GLint, GLint, GLint);
    static func_t real = (func_t)GetRealGLProc("glRecti", 0);
    if (real) real(x1, y1, x2, y2);
}

EXPORT void WINAPI glRectiv(const GLint* v1, const GLint* v2)
{
    typedef void(WINAPI* func_t)(const GLint*, const GLint*);
    static func_t real = (func_t)GetRealGLProc("glRectiv", 0);
    if (real) real(v1, v2);
}

EXPORT void WINAPI glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
    typedef void(WINAPI* func_t)(GLshort, GLshort, GLshort, GLshort);
    static func_t real = (func_t)GetRealGLProc("glRects", 0);
    if (real) real(x1, y1, x2, y2);
}

EXPORT void WINAPI glRectsv(const GLshort* v1, const GLshort* v2)
{
    typedef void(WINAPI* func_t)(const GLshort*, const GLshort*);
    static func_t real = (func_t)GetRealGLProc("glRectsv", 0);
    if (real) real(v1, v2);
}

EXPORT GLint WINAPI glRenderMode(GLenum mode)
{
    typedef GLint(WINAPI* func_t)(GLenum);
    static func_t real = (func_t)GetRealGLProc("glRenderMode", 0);
    return real ? real(mode) : 0;
}

EXPORT void WINAPI glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
    typedef void(WINAPI* func_t)(GLdouble, GLdouble, GLdouble, GLdouble);
    static func_t real = (func_t)GetRealGLProc("glRotated", 0);
    if (real) real(angle, x, y, z);
}

EXPORT void WINAPI glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    typedef void(WINAPI* func_t)(GLfloat, GLfloat, GLfloat, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glRotatef", 0);
    if (real) real(angle, x, y, z);
}

EXPORT void WINAPI glScaled(GLdouble x, GLdouble y, GLdouble z)
{
    typedef void(WINAPI* func_t)(GLdouble, GLdouble, GLdouble);
    static func_t real = (func_t)GetRealGLProc("glScaled", 0);
    if (real) real(x, y, z);
}

EXPORT void WINAPI glScalef(GLfloat x, GLfloat y, GLfloat z)
{
    typedef void(WINAPI* func_t)(GLfloat, GLfloat, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glScalef", 0);
    if (real) real(x, y, z);
}

EXPORT void WINAPI glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    typedef void(WINAPI* func_t)(GLint, GLint, GLsizei, GLsizei);
    static func_t real = (func_t)GetRealGLProc("glScissor", 0);
    if (real) real(x, y, width, height);
}

EXPORT void WINAPI glSelectBuffer(GLsizei size, GLuint* buffer)
{
    typedef void(WINAPI* func_t)(GLsizei, GLuint*);
    static func_t real = (func_t)GetRealGLProc("glSelectBuffer", 0);
    if (real) real(size, buffer);
}

EXPORT void WINAPI glShadeModel(GLenum mode)
{
    typedef void(WINAPI* func_t)(GLenum);
    static func_t real = (func_t)GetRealGLProc("glShadeModel", 0);
    if (real) real(mode);
}

EXPORT void WINAPI glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
    typedef void(WINAPI* func_t)(GLenum, GLint, GLuint);
    static func_t real = (func_t)GetRealGLProc("glStencilFunc", 0);
    if (real) real(func, ref, mask);
}

EXPORT void WINAPI glStencilMask(GLuint mask)
{
    typedef void(WINAPI* func_t)(GLuint);
    static func_t real = (func_t)GetRealGLProc("glStencilMask", 0);
    if (real) real(mask);
}

EXPORT void WINAPI glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, GLenum);
    static func_t real = (func_t)GetRealGLProc("glStencilOp", 0);
    if (real) real(fail, zfail, zpass);
}

EXPORT void WINAPI glTexCoord1d(GLdouble s)
{
    typedef void(WINAPI* func_t)(GLdouble);
    static func_t real = (func_t)GetRealGLProc("glTexCoord1d", 0);
    if (real) real(s);
}

EXPORT void WINAPI glTexCoord1dv(const GLdouble* v)
{
    typedef void(WINAPI* func_t)(const GLdouble*);
    static func_t real = (func_t)GetRealGLProc("glTexCoord1dv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glTexCoord1f(GLfloat s)
{
    typedef void(WINAPI* func_t)(GLfloat);
    static func_t real = (func_t)GetRealGLProc("glTexCoord1f", 0);
    if (real) real(s);
}

EXPORT void WINAPI glTexCoord1fv(const GLfloat* v)
{
    typedef void(WINAPI* func_t)(const GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glTexCoord1fv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glTexCoord1i(GLint s)
{
    typedef void(WINAPI* func_t)(GLint);
    static func_t real = (func_t)GetRealGLProc("glTexCoord1i", 0);
    if (real) real(s);
}

EXPORT void WINAPI glTexCoord1iv(const GLint* v)
{
    typedef void(WINAPI* func_t)(const GLint*);
    static func_t real = (func_t)GetRealGLProc("glTexCoord1iv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glTexCoord1s(GLshort s)
{
    typedef void(WINAPI* func_t)(GLshort);
    static func_t real = (func_t)GetRealGLProc("glTexCoord1s", 0);
    if (real) real(s);
}

EXPORT void WINAPI glTexCoord1sv(const GLshort* v)
{
    typedef void(WINAPI* func_t)(const GLshort*);
    static func_t real = (func_t)GetRealGLProc("glTexCoord1sv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glTexCoord2d(GLdouble s, GLdouble t)
{
    typedef void(WINAPI* func_t)(GLdouble, GLdouble);
    static func_t real = (func_t)GetRealGLProc("glTexCoord2d", 0);
    if (real) real(s, t);
}

EXPORT void WINAPI glTexCoord2dv(const GLdouble* v)
{
    typedef void(WINAPI* func_t)(const GLdouble*);
    static func_t real = (func_t)GetRealGLProc("glTexCoord2dv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glTexCoord2f(GLfloat s, GLfloat t)
{
    typedef void(WINAPI* func_t)(GLfloat, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glTexCoord2f", 0);
    if (real) real(s, t);
}

EXPORT void WINAPI glTexCoord2fv(const GLfloat* v)
{
    typedef void(WINAPI* func_t)(const GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glTexCoord2fv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glTexCoord2i(GLint s, GLint t)
{
    typedef void(WINAPI* func_t)(GLint, GLint);
    static func_t real = (func_t)GetRealGLProc("glTexCoord2i", 0);
    if (real) real(s, t);
}

EXPORT void WINAPI glTexCoord2iv(const GLint* v)
{
    typedef void(WINAPI* func_t)(const GLint*);
    static func_t real = (func_t)GetRealGLProc("glTexCoord2iv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glTexCoord2s(GLshort s, GLshort t)
{
    typedef void(WINAPI* func_t)(GLshort, GLshort);
    static func_t real = (func_t)GetRealGLProc("glTexCoord2s", 0);
    if (real) real(s, t);
}

EXPORT void WINAPI glTexCoord2sv(const GLshort* v)
{
    typedef void(WINAPI* func_t)(const GLshort*);
    static func_t real = (func_t)GetRealGLProc("glTexCoord2sv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glTexCoord3d(GLdouble s, GLdouble t, GLdouble r)
{
    typedef void(WINAPI* func_t)(GLdouble, GLdouble, GLdouble);
    static func_t real = (func_t)GetRealGLProc("glTexCoord3d", 0);
    if (real) real(s, t, r);
}

EXPORT void WINAPI glTexCoord3dv(const GLdouble* v)
{
    typedef void(WINAPI* func_t)(const GLdouble*);
    static func_t real = (func_t)GetRealGLProc("glTexCoord3dv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glTexCoord3f(GLfloat s, GLfloat t, GLfloat r)
{
    typedef void(WINAPI* func_t)(GLfloat, GLfloat, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glTexCoord3f", 0);
    if (real) real(s, t, r);
}

EXPORT void WINAPI glTexCoord3fv(const GLfloat* v)
{
    typedef void(WINAPI* func_t)(const GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glTexCoord3fv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glTexCoord3i(GLint s, GLint t, GLint r)
{
    typedef void(WINAPI* func_t)(GLint, GLint, GLint);
    static func_t real = (func_t)GetRealGLProc("glTexCoord3i", 0);
    if (real) real(s, t, r);
}

EXPORT void WINAPI glTexCoord3iv(const GLint* v)
{
    typedef void(WINAPI* func_t)(const GLint*);
    static func_t real = (func_t)GetRealGLProc("glTexCoord3iv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glTexCoord3s(GLshort s, GLshort t, GLshort r)
{
    typedef void(WINAPI* func_t)(GLshort, GLshort, GLshort);
    static func_t real = (func_t)GetRealGLProc("glTexCoord3s", 0);
    if (real) real(s, t, r);
}

EXPORT void WINAPI glTexCoord3sv(const GLshort* v)
{
    typedef void(WINAPI* func_t)(const GLshort*);
    static func_t real = (func_t)GetRealGLProc("glTexCoord3sv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
    typedef void(WINAPI* func_t)(GLdouble, GLdouble, GLdouble, GLdouble);
    static func_t real = (func_t)GetRealGLProc("glTexCoord4d", 0);
    if (real) real(s, t, r, q);
}

EXPORT void WINAPI glTexCoord4dv(const GLdouble* v)
{
    typedef void(WINAPI* func_t)(const GLdouble*);
    static func_t real = (func_t)GetRealGLProc("glTexCoord4dv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    typedef void(WINAPI* func_t)(GLfloat, GLfloat, GLfloat, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glTexCoord4f", 0);
    if (real) real(s, t, r, q);
}

EXPORT void WINAPI glTexCoord4fv(const GLfloat* v)
{
    typedef void(WINAPI* func_t)(const GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glTexCoord4fv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glTexCoord4i(GLint s, GLint t, GLint r, GLint q)
{
    typedef void(WINAPI* func_t)(GLint, GLint, GLint, GLint);
    static func_t real = (func_t)GetRealGLProc("glTexCoord4i", 0);
    if (real) real(s, t, r, q);
}

EXPORT void WINAPI glTexCoord4iv(const GLint* v)
{
    typedef void(WINAPI* func_t)(const GLint*);
    static func_t real = (func_t)GetRealGLProc("glTexCoord4iv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q)
{
    typedef void(WINAPI* func_t)(GLshort, GLshort, GLshort, GLshort);
    static func_t real = (func_t)GetRealGLProc("glTexCoord4s", 0);
    if (real) real(s, t, r, q);
}

EXPORT void WINAPI glTexCoord4sv(const GLshort* v)
{
    typedef void(WINAPI* func_t)(const GLshort*);
    static func_t real = (func_t)GetRealGLProc("glTexCoord4sv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glTexEnvf", 0);
    if (real) real(target, pname, param);
}

EXPORT void WINAPI glTexEnvfv(GLenum target, GLenum pname, const GLfloat* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, const GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glTexEnvfv", 0);
    if (real) real(target, pname, params);
}

EXPORT void WINAPI glTexEnvi(GLenum target, GLenum pname, GLint param)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, GLint);
    static func_t real = (func_t)GetRealGLProc("glTexEnvi", 0);
    if (real) real(target, pname, param);
}

EXPORT void WINAPI glTexEnviv(GLenum target, GLenum pname, const GLint* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, const GLint*);
    static func_t real = (func_t)GetRealGLProc("glTexEnviv", 0);
    if (real) real(target, pname, params);
}

EXPORT void WINAPI glTexGend(GLenum coord, GLenum pname, GLdouble param)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, GLdouble);
    static func_t real = (func_t)GetRealGLProc("glTexGend", 0);
    if (real) real(coord, pname, param);
}

EXPORT void WINAPI glTexGendv(GLenum coord, GLenum pname, const GLdouble* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, const GLdouble*);
    static func_t real = (func_t)GetRealGLProc("glTexGendv", 0);
    if (real) real(coord, pname, params);
}

EXPORT void WINAPI glTexGenf(GLenum coord, GLenum pname, GLfloat param)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glTexGenf", 0);
    if (real) real(coord, pname, param);
}

EXPORT void WINAPI glTexGenfv(GLenum coord, GLenum pname, const GLfloat* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, const GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glTexGenfv", 0);
    if (real) real(coord, pname, params);
}

EXPORT void WINAPI glTexGeni(GLenum coord, GLenum pname, GLint param)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, GLint);
    static func_t real = (func_t)GetRealGLProc("glTexGeni", 0);
    if (real) real(coord, pname, param);
}

EXPORT void WINAPI glTexGeniv(GLenum coord, GLenum pname, const GLint* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, const GLint*);
    static func_t real = (func_t)GetRealGLProc("glTexGeniv", 0);
    if (real) real(coord, pname, params);
}

EXPORT void WINAPI glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width,
    GLint border, GLenum format, GLenum type, const GLvoid* pixels)
{
    typedef void(WINAPI* func_t)(GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum, const GLvoid*);
    static func_t real = (func_t)GetRealGLProc("glTexImage1D", 0);
    if (real) real(target, level, internalformat, width, border, format, type, pixels);
}

EXPORT void WINAPI glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
    GLint border, GLenum format, GLenum type, const GLvoid* pixels)
{
    typedef void(WINAPI* func_t)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*);
    static func_t real = (func_t)GetRealGLProc("glTexImage2D", 0);
    if (real) real(target, level, internalformat, width, height, border, format, type, pixels);
}

EXPORT void WINAPI glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glTexParameterf", 0);
    if (real) real(target, pname, param);
}

EXPORT void WINAPI glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, const GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glTexParameterfv", 0);
    if (real) real(target, pname, params);
}

EXPORT void WINAPI glTexParameteri(GLenum target, GLenum pname, GLint param)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, GLint);
    static func_t real = (func_t)GetRealGLProc("glTexParameteri", 0);
    if (real) real(target, pname, param);
}

EXPORT void WINAPI glTexParameteriv(GLenum target, GLenum pname, const GLint* params)
{
    typedef void(WINAPI* func_t)(GLenum, GLenum, const GLint*);
    static func_t real = (func_t)GetRealGLProc("glTexParameteriv", 0);
    if (real) real(target, pname, params);
}

EXPORT void WINAPI glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width,
    GLenum format, GLenum type, const GLvoid* pixels)
{
    typedef void(WINAPI* func_t)(GLenum, GLint, GLint, GLsizei, GLenum, GLenum, const GLvoid*);
    static func_t real = (func_t)GetRealGLProc("glTexSubImage1D", 0);
    if (real) real(target, level, xoffset, width, format, type, pixels);
}

EXPORT void WINAPI glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
    GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels)
{
    typedef void(WINAPI* func_t)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*);
    static func_t real = (func_t)GetRealGLProc("glTexSubImage2D", 0);
    if (real) real(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

EXPORT void WINAPI glTranslated(GLdouble x, GLdouble y, GLdouble z)
{
    typedef void(WINAPI* func_t)(GLdouble, GLdouble, GLdouble);
    static func_t real = (func_t)GetRealGLProc("glTranslated", 0);
    if (real) real(x, y, z);
}

EXPORT void WINAPI glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
    typedef void(WINAPI* func_t)(GLfloat, GLfloat, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glTranslatef", 0);
    if (real) real(x, y, z);
}

EXPORT void WINAPI glVertex2d(GLdouble x, GLdouble y)
{
    typedef void(WINAPI* func_t)(GLdouble, GLdouble);
    static func_t real = (func_t)GetRealGLProc("glVertex2d", 0);
    if (real) real(x, y);
}

EXPORT void WINAPI glVertex2dv(const GLdouble* v)
{
    typedef void(WINAPI* func_t)(const GLdouble*);
    static func_t real = (func_t)GetRealGLProc("glVertex2dv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glVertex2f(GLfloat x, GLfloat y)
{
    typedef void(WINAPI* func_t)(GLfloat, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glVertex2f", 0);
    if (real) real(x, y);
}

EXPORT void WINAPI glVertex2fv(const GLfloat* v)
{
    typedef void(WINAPI* func_t)(const GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glVertex2fv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glVertex2i(GLint x, GLint y)
{
    typedef void(WINAPI* func_t)(GLint, GLint);
    static func_t real = (func_t)GetRealGLProc("glVertex2i", 0);
    if (real) real(x, y);
}

EXPORT void WINAPI glVertex2iv(const GLint* v)
{
    typedef void(WINAPI* func_t)(const GLint*);
    static func_t real = (func_t)GetRealGLProc("glVertex2iv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glVertex2s(GLshort x, GLshort y)
{
    typedef void(WINAPI* func_t)(GLshort, GLshort);
    static func_t real = (func_t)GetRealGLProc("glVertex2s", 0);
    if (real) real(x, y);
}

EXPORT void WINAPI glVertex2sv(const GLshort* v)
{
    typedef void(WINAPI* func_t)(const GLshort*);
    static func_t real = (func_t)GetRealGLProc("glVertex2sv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glVertex3d(GLdouble x, GLdouble y, GLdouble z)
{
    typedef void(WINAPI* func_t)(GLdouble, GLdouble, GLdouble);
    static func_t real = (func_t)GetRealGLProc("glVertex3d", 0);
    if (real) real(x, y, z);
}

EXPORT void WINAPI glVertex3dv(const GLdouble* v)
{
    typedef void(WINAPI* func_t)(const GLdouble*);
    static func_t real = (func_t)GetRealGLProc("glVertex3dv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
    typedef void(WINAPI* func_t)(GLfloat, GLfloat, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glVertex3f", 0);
    if (real) real(x, y, z);
}

EXPORT void WINAPI glVertex3fv(const GLfloat* v)
{
    typedef void(WINAPI* func_t)(const GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glVertex3fv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glVertex3i(GLint x, GLint y, GLint z)
{
    typedef void(WINAPI* func_t)(GLint, GLint, GLint);
    static func_t real = (func_t)GetRealGLProc("glVertex3i", 0);
    if (real) real(x, y, z);
}

EXPORT void WINAPI glVertex3iv(const GLint* v)
{
    typedef void(WINAPI* func_t)(const GLint*);
    static func_t real = (func_t)GetRealGLProc("glVertex3iv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glVertex3s(GLshort x, GLshort y, GLshort z)
{
    typedef void(WINAPI* func_t)(GLshort, GLshort, GLshort);
    static func_t real = (func_t)GetRealGLProc("glVertex3s", 0);
    if (real) real(x, y, z);
}

EXPORT void WINAPI glVertex3sv(const GLshort* v)
{
    typedef void(WINAPI* func_t)(const GLshort*);
    static func_t real = (func_t)GetRealGLProc("glVertex3sv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    typedef void(WINAPI* func_t)(GLdouble, GLdouble, GLdouble, GLdouble);
    static func_t real = (func_t)GetRealGLProc("glVertex4d", 0);
    if (real) real(x, y, z, w);
}

EXPORT void WINAPI glVertex4dv(const GLdouble* v)
{
    typedef void(WINAPI* func_t)(const GLdouble*);
    static func_t real = (func_t)GetRealGLProc("glVertex4dv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    typedef void(WINAPI* func_t)(GLfloat, GLfloat, GLfloat, GLfloat);
    static func_t real = (func_t)GetRealGLProc("glVertex4f", 0);
    if (real) real(x, y, z, w);
}

EXPORT void WINAPI glVertex4fv(const GLfloat* v)
{
    typedef void(WINAPI* func_t)(const GLfloat*);
    static func_t real = (func_t)GetRealGLProc("glVertex4fv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glVertex4i(GLint x, GLint y, GLint z, GLint w)
{
    typedef void(WINAPI* func_t)(GLint, GLint, GLint, GLint);
    static func_t real = (func_t)GetRealGLProc("glVertex4i", 0);
    if (real) real(x, y, z, w);
}

EXPORT void WINAPI glVertex4iv(const GLint* v)
{
    typedef void(WINAPI* func_t)(const GLint*);
    static func_t real = (func_t)GetRealGLProc("glVertex4iv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glVertex4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
    typedef void(WINAPI* func_t)(GLshort, GLshort, GLshort, GLshort);
    static func_t real = (func_t)GetRealGLProc("glVertex4s", 0);
    if (real) real(x, y, z, w);
}

EXPORT void WINAPI glVertex4sv(const GLshort* v)
{
    typedef void(WINAPI* func_t)(const GLshort*);
    static func_t real = (func_t)GetRealGLProc("glVertex4sv", 0);
    if (real) real(v);
}

EXPORT void WINAPI glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    typedef void(WINAPI* func_t)(GLint, GLint, GLsizei, GLsizei);
    static func_t real = (func_t)GetRealGLProc("glViewport", 0);
    if (real) real(x, y, width, height);
}

// ============================================================
// wgl functions
// ============================================================
EXPORT int WINAPI wglChoosePixelFormat(HDC hdc, const PIXELFORMATDESCRIPTOR* ppfd)
{
    typedef int(WINAPI* func_t)(HDC, const PIXELFORMATDESCRIPTOR*);
    static func_t real = (func_t)GetRealGLProc("wglChoosePixelFormat", 0);
    return real ? real(hdc, ppfd) : 0;
}

EXPORT int WINAPI wglDescribePixelFormat(HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd)
{
    typedef int(WINAPI* func_t)(HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);
    static func_t real = (func_t)GetRealGLProc("wglDescribePixelFormat", 0);
    return real ? real(hdc, iPixelFormat, nBytes, ppfd) : 0;
}

EXPORT int WINAPI wglGetPixelFormat(HDC hdc)
{
    typedef int(WINAPI* func_t)(HDC);
    static func_t real = (func_t)GetRealGLProc("wglGetPixelFormat", 0);
    return real ? real(hdc) : 0;
}

EXPORT BOOL WINAPI wglSetPixelFormat(HDC hdc, int iPixelFormat, const PIXELFORMATDESCRIPTOR* ppfd)
{
    typedef BOOL(WINAPI* func_t)(HDC, int, const PIXELFORMATDESCRIPTOR*);
    static func_t real = (func_t)GetRealGLProc("wglSetPixelFormat", 0);
    return real ? real(hdc, iPixelFormat, ppfd) : FALSE;
}

EXPORT BOOL WINAPI wglSwapBuffers(HDC hdc)
{
    typedef BOOL(WINAPI* func_t)(HDC);
    static func_t real = (func_t)GetRealGLProc("wglSwapBuffers", 0);
    return real ? real(hdc) : FALSE;
}

EXPORT BOOL WINAPI wglCopyContext(HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask)
{
    typedef BOOL(WINAPI* func_t)(HGLRC, HGLRC, UINT);
    static func_t real = (func_t)GetRealGLProc("wglCopyContext", 0);
    return real ? real(hglrcSrc, hglrcDst, mask) : FALSE;
}

EXPORT HGLRC WINAPI wglCreateContext(HDC hdc)
{
    typedef HGLRC(WINAPI* func_t)(HDC);
    static func_t real = (func_t)GetRealGLProc("wglCreateContext", 0);
    return real ? real(hdc) : NULL;
}

EXPORT HGLRC WINAPI wglCreateLayerContext(HDC hdc, int iLayerPlane)
{
    typedef HGLRC(WINAPI* func_t)(HDC, int);
    static func_t real = (func_t)GetRealGLProc("wglCreateLayerContext", 0);
    return real ? real(hdc, iLayerPlane) : NULL;
}

EXPORT BOOL WINAPI wglDeleteContext(HGLRC hglrc)
{
    typedef BOOL(WINAPI* func_t)(HGLRC);
    static func_t real = (func_t)GetRealGLProc("wglDeleteContext", 0);
    return real ? real(hglrc) : FALSE;
}

EXPORT HGLRC WINAPI wglGetCurrentContext(void)
{
    typedef HGLRC(WINAPI* func_t)();
    static func_t real = (func_t)GetRealGLProc("wglGetCurrentContext", 0);
    return real ? real() : NULL;
}

EXPORT HDC WINAPI wglGetCurrentDC(void)
{
    typedef HDC(WINAPI* func_t)();
    static func_t real = (func_t)GetRealGLProc("wglGetCurrentDC", 0);
    return real ? real() : NULL;
}

EXPORT PROC WINAPI wglGetProcAddress(LPCSTR lpszProc)
{
    typedef PROC(WINAPI* func_t)(LPCSTR);
    static func_t real = (func_t)GetRealGLProc("wglGetProcAddress", 0);
    return real ? real(lpszProc) : NULL;
}

EXPORT BOOL WINAPI wglMakeCurrent(HDC hdc, HGLRC hglrc)
{
    typedef BOOL(WINAPI* func_t)(HDC, HGLRC);
    static func_t real = (func_t)GetRealGLProc("wglMakeCurrent", 0);
    return real ? real(hdc, hglrc) : FALSE;
}

EXPORT BOOL WINAPI wglShareLists(HGLRC hglrc1, HGLRC hglrc2)
{
    typedef BOOL(WINAPI* func_t)(HGLRC, HGLRC);
    static func_t real = (func_t)GetRealGLProc("wglShareLists", 0);
    return real ? real(hglrc1, hglrc2) : FALSE;
}

EXPORT BOOL WINAPI wglUseFontBitmapsA(HDC hdc, DWORD first, DWORD count, DWORD listBase)
{
    typedef BOOL(WINAPI* func_t)(HDC, DWORD, DWORD, DWORD);
    static func_t real = (func_t)GetRealGLProc("wglUseFontBitmapsA", 0);
    return real ? real(hdc, first, count, listBase) : FALSE;
}

EXPORT BOOL WINAPI wglUseFontBitmapsW(HDC hdc, DWORD first, DWORD count, DWORD listBase)
{
    typedef BOOL(WINAPI* func_t)(HDC, DWORD, DWORD, DWORD);
    static func_t real = (func_t)GetRealGLProc("wglUseFontBitmapsW", 0);
    return real ? real(hdc, first, count, listBase) : FALSE;
}

EXPORT BOOL WINAPI wglUseFontOutlinesA(HDC hdc, DWORD first, DWORD count, DWORD listBase,
    FLOAT deviation, FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf)
{
    typedef BOOL(WINAPI* func_t)(HDC, DWORD, DWORD, DWORD, FLOAT, FLOAT, int, LPGLYPHMETRICSFLOAT);
    static func_t real = (func_t)GetRealGLProc("wglUseFontOutlinesA", 0);
    return real ? real(hdc, first, count, listBase, deviation, extrusion, format, lpgmf) : FALSE;
}

EXPORT BOOL WINAPI wglUseFontOutlinesW(HDC hdc, DWORD first, DWORD count, DWORD listBase,
    FLOAT deviation, FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf)
{
    typedef BOOL(WINAPI* func_t)(HDC, DWORD, DWORD, DWORD, FLOAT, FLOAT, int, LPGLYPHMETRICSFLOAT);
    static func_t real = (func_t)GetRealGLProc("wglUseFontOutlinesW", 0);
    return real ? real(hdc, first, count, listBase, deviation, extrusion, format, lpgmf) : FALSE;
}

// ============================================================
// Exported ordinal functions (forwarded via .def file)
// ============================================================
// All remaining functions are forwarded via the .def file using:
//   funcname @ordinal = SYSTEM32\opengl32.funcname
// This handles all ~1200+ functions we don't explicitly proxy above