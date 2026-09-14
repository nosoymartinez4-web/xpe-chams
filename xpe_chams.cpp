// ============================================================
// XPE CHAMS v2.0 - Free Fire Wallhack for Emulators
// Copyright (c) 2026 xpe.nettt / Stealth Proyects.
// All rights reserved.
//
// Author:  xpe.nettt
// Company: Stealth Proyects
// Product: XPE CHAMS
//
// Compatibility:
//   - LDPlayer / OptiPlayer7 / BlueStacks / MSI Emulator
//   - Injection: Process Hacker, Extreme Injector, or
//     proxy opengl32.dll method
//
// Compile (MinGW):
//   windres resource.rc -O coff -o resource.o
//   g++ -shared -o "XPE CHAMS.dll" xpe_chams.cpp resource.o ^
//       -lopengl32 -lgdi32 -static -O2 -s
//
// Compile (MSVC):
//   rc /fo resource.res resource.rc
//   cl /LD /MT xpe_chams.cpp resource.res ^
//       /link /OUT:"XPE CHAMS.dll" opengl32.lib gdi32.lib
// ============================================================

#include <windows.h>
#include <gl/GL.h>
#include <cstdio>
#include <cstring>

// ============================================================
// CONFIGURATION
// ============================================================
#define MENU_TOGGLE_KEY     VK_F7
#define STREAMER_MODE_KEY   VK_F8
#define DLL_NAME            "XPE CHAMS"

// ============================================================
// TYPE DEFS FOR OPENGL
// ============================================================
typedef BOOL  (WINAPI* t_wglSwapBuffers)(HDC);
typedef void  (WINAPI* t_glDrawElements)(GLenum, GLsizei, GLenum, const GLvoid*);

t_wglSwapBuffers  o_wglSwapBuffers  = nullptr;
t_glDrawElements  o_glDrawElements  = nullptr;

HINSTANCE g_hInst = nullptr;
bool      g_running = false;

// Font display list
GLuint g_fontBase = 0;
bool   g_fontReady = false;

// ============================================================
// CHAMS CONFIG
// ============================================================
enum ChamsMode {
    CHAMS_OFF = 0,
    CHAMS_SOLID,
    CHAMS_WIREFRAME,
    CHAMS_GLOW,
    CHAMS_INVISIBLE,
    CHAMS_RAINBOW,
    CHAMS_METALLIC,
    CHAMS_MAX
};

static const char* ChamsModeNames[] = {
    "OFF",
    "SOLID",
    "WIREFRAME",
    "GLOW",
    "INVISIBLE",
    "RAINBOW",
    "METALLIC"
};

struct Config {
    bool      enabled       = true;
    ChamsMode mode          = CHAMS_SOLID;
    float     wallColor[3]  = { 0.0f, 1.0f, 0.0f };
    float     visibleColor[3] = { 1.0f, 0.0f, 0.0f };
    float     opacity       = 1.0f;
    bool      showMenu      = false;
    bool      streamerMode  = false;
    int       sel           = 0;
    float     hue           = 0.0f;
};

static Config g;

// ============================================================
// MEMORY / HOOK HELPERS
// ============================================================
static BYTE* AllocExec(size_t sz) {
    return (BYTE*)VirtualAlloc(nullptr, sz, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
}

static bool InstallHook(void* target, void* hook, void** orig) {
    if (!target || !hook) return false;
    DWORD old;
    VirtualProtect(target, 5, PAGE_EXECUTE_READWRITE, &old);
    BYTE origBytes[5]; memcpy(origBytes, target, 5);
    BYTE* tramp = AllocExec(10);
    if (!tramp) { VirtualProtect(target, 5, old, &old); return false; }
    memcpy(tramp, origBytes, 5);
    tramp[5] = 0xE9;
    *(DWORD*)(tramp+6) = (DWORD)((BYTE*)target+5) - (DWORD)(tramp+10);
    BYTE jmp[5] = { 0xE9 };
    *(DWORD*)(jmp+1) = (DWORD)hook - (DWORD)target - 5;
    memcpy(target, jmp, 5);
    VirtualProtect(target, 5, old, &old);
    *orig = tramp;
    return true;
}

// ============================================================
// STREAMER MODE
// ============================================================
static bool IsStreamAppRunning() {
    const char* apps[] = {
        "obs64.exe","obs32.exe","obs.exe",
        "discord.exe","xsplit.core.exe","twitchstudio.exe",
        "streamlabs.exe","vmix.exe","ffmpeg.exe",
        "bandicam.exe","bdcam.exe","nvidia share.exe",nullptr
    };
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if (snap == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32 pe = { sizeof(pe) };
    bool found = false;
    if (Process32First(snap, &pe)) do {
        char exe[MAX_PATH]; strcpy_s(exe, pe.szExeFile);
        for (char* p=exe; *p; ++p) *p = (char)tolower(*p);
        for (int i=0; apps[i]; ++i)
            if (strstr(exe, apps[i])) { found=true; break; }
    } while (!found && Process32Next(snap, &pe));
    CloseHandle(snap);
    return found;
}
static bool ShouldHide() {
    return g.streamerMode && IsStreamAppRunning();
}

// ============================================================
// OPENGL DRAWING
// ============================================================
static void SetupFont(HDC hdc) {
    if (g_fontReady) return;
    g_fontBase = glGenLists(96);
    HFONT f = CreateFontA(-13,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,
        ANSI_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY,FF_DONTCARE|DEFAULT_PITCH,"Consolas");
    HFONT old = (HFONT)SelectObject(hdc, f);
    wglUseFontBitmaps(hdc, 32, 96, g_fontBase);
    SelectObject(hdc, old);
    DeleteObject(f);
    g_fontReady = true;
}

static void DrawText(float x, float y, float r, float g, float b, float a, const char* txt) {
    if (!txt || !g_fontReady) return;
    GLint vp[4]; glGetIntegerv(GL_VIEWPORT, vp);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    glOrtho(0,(float)vp[2],(float)vp[3],0,-1,1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING); glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(r,g,b,a);
    glRasterPos2f(x,y);
    glPushAttrib(GL_LIST_BIT);
    glListBase(g_fontBase-32);
    glCallLists((GLsizei)strlen(txt), GL_UNSIGNED_BYTE, txt);
    glPopAttrib();
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

static void DrawRect(float x, float y, float w, float h, float r, float g, float b, float a) {
    GLint vp[4]; glGetIntegerv(GL_VIEWPORT, vp);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    glOrtho(0,(float)vp[2],(float)vp[3],0,-1,1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING); glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(r,g,b,a);
    glBegin(GL_QUADS);
    glVertex2f(x,y); glVertex2f(x+w,y);
    glVertex2f(x+w,y+h); glVertex2f(x,y+h);
    glEnd();
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

static void DrawBorder(float x, float y, float w, float h, float r, float g, float b, float a, float t) {
    DrawRect(x,y,w,t,r,g,b,a);
    DrawRect(x,y+h-t,w,t,r,g,b,a);
    DrawRect(x,y,t,h,r,g,b,a);
    DrawRect(x+w-t,y,t,h,r,g,b,a);
}

// ============================================================
// MENU — SOLID, STATIC, NO SCROLL
// ============================================================
static void RenderMenu() {
    if (!g.showMenu) return;

    GLint vp[4]; glGetIntegerv(GL_VIEWPORT, vp);
    int sw = vp[2], sh = vp[3];

    // ── Panel dimensions ──
    const int mx = 40, my = 40, mw = 290, mh = 380;
    const int pad = 12;
    const int tx = mx + pad;
    const int titleH = 32;
    const int sepY = 0; // will compute
    int y = my + titleH + 8;

    // ── Background ──
    DrawRect((float)mx, (float)my, (float)mw, (float)mh, 0.08f, 0.08f, 0.08f, 0.92f);
    DrawBorder((float)mx, (float)my, (float)mw, (float)mh, 0.35f, 0.35f, 0.35f, 0.9f, 1.0f);

    // ── Title bar ──
    DrawRect((float)mx, (float)my, (float)mw, (float)titleH, 0.12f, 0.12f, 0.12f, 1.0f);
    DrawBorder((float)mx, (float)my, (float)mw, (float)titleH, 0.4f, 0.4f, 0.4f, 0.6f, 1.0f);

    // Title line
    DrawText((float)tx, (float)(my + 8), 0.0f, 0.85f, 0.0f, 1.0f, "XPE CHAMS v2.0");
    // Subtitle right
    DrawText((float)(mx + mw - 100), (float)(my + 9), 0.5f, 0.5f, 0.5f, 0.7f, "xpe.nettt");

    // ── Separator ──
    int sep = y + 4;
    DrawRect((float)(mx+pad), (float)sep, (float)(mw-2*pad), 1.0f, 0.3f, 0.3f, 0.3f, 0.5f);
    y = sep + 12;

    // ── SECTION: CHAMS ──
    DrawText((float)tx, (float)y, 0.5f, 0.5f, 0.5f, 0.7f, "CHAMS");
    y += 18;

    // [X] Chams ON/OFF
    char buf[128];
    sprintf_s(buf, sizeof(buf), "%s  %s",
        g.enabled ? "[X]" : "[ ]",
        g.enabled ? "CHAMS ENABLED" : "CHAMS DISABLED");
    DrawText((float)(tx+8), (float)y,
        g.enabled ? 0.0f : 0.6f,
        g.enabled ? 0.85f : 0.6f,
        g.enabled ? 0.0f : 0.6f,
        1.0f, buf);
    y += 22;

    // Mode
    sprintf_s(buf, sizeof(buf), "MODE:  %s", ChamsModeNames[g.mode]);
    DrawText((float)(tx+8), (float)y, 0.8f, 0.8f, 0.8f, 1.0f, buf);
    y += 22;

    // Streamer
    sprintf_s(buf, sizeof(buf), "%s  STREAMER MODE",
        g.streamerMode ? "[X]" : "[ ]");
    DrawText((float)(tx+8), (float)y,
        g.streamerMode ? 0.85f : 0.6f,
        g.streamerMode ? 0.85f : 0.6f,
        g.streamerMode ? 0.0f : 0.6f,
        1.0f, buf);
    y += 22;

    // Status indicator
    if (ShouldHide()) {
        DrawText((float)(tx+8), (float)y, 1.0f, 0.2f, 0.2f, 1.0f, ">> STREAMER ACTIVE <<");
    } else if (g.streamerMode) {
        DrawText((float)(tx+8), (float)y, 0.3f, 0.8f, 0.3f, 0.8f, "Standby - no stream detected");
    }
    y += 22;

    // ── Separator ──
    DrawRect((float)(mx+pad), (float)(y+2), (float)(mw-2*pad), 1.0f, 0.3f, 0.3f, 0.3f, 0.4f);
    y += 16;

    // ── SECTION: CONTROLS ──
    DrawText((float)tx, (float)y, 0.5f, 0.5f, 0.5f, 0.7f, "CONTROLS");
    y += 18;

    struct KeyBind { const char* key; const char* action; };
    KeyBind binds[] = {
        {"F7",       "Toggle Menu"},
        {"F8",       "Toggle Streamer Mode"},
        {"UP/DOWN",  "Navigate"},
        {"ENTER",    "Toggle/Select"},
        {"LEFT/RIGHT","Change Mode"},
        {nullptr, nullptr}
    };
    for (int i=0; binds[i].key; ++i) {
        DrawText((float)(tx+8), (float)y, 0.6f, 0.6f, 0.6f, 0.9f, binds[i].key);
        DrawText((float)(tx+78), (float)y, 0.4f, 0.4f, 0.4f, 0.7f, binds[i].action);
        y += 17;
    }

    // ── Footer ──
    y = my + mh - 18;
    DrawRect((float)(mx+pad), (float)(y-4), (float)(mw-2*pad), 1.0f, 0.3f, 0.3f, 0.3f, 0.3f);
    DrawText((float)tx, (float)y, 0.35f, 0.35f, 0.35f, 0.6f, "(c) 2026 xpe.nettt / Stealth Proyects");

    // ── Selection highlight ──
    // Simple indicator: a small square next to the selected item
    float selY = (float)(my + titleH + 8 + 18 + 18 + g.sel * 22 + 4);
    DrawRect((float)(mx+pad-2), selY, 4.0f, 14.0f, 0.0f, 0.85f, 0.0f, 0.8f);

    // ── Watermark (always visible when menu closed) ──
    if (!g.showMenu) {
        DrawText((float)(sw - 160), 16.0f, 0.0f, 0.7f, 0.0f, 0.4f, "XPE CHAMS");
    }
}

// ============================================================
// CHAMS HOOK
// ============================================================
static void WINAPI hk_glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices) {
    if (!o_glDrawElements) return;

    if (!g.enabled || ShouldHide()) {
        o_glDrawElements(mode, count, type, indices);
        return;
    }

    switch (g.mode) {
    case CHAMS_SOLID: {
        glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        glColor4f(g.wallColor[0], g.wallColor[1], g.wallColor[2], g.opacity);
        o_glDrawElements(mode, count, type, indices);
        glDepthMask(GL_TRUE); glDepthFunc(GL_EQUAL);
        glColor4f(g.visibleColor[0], g.visibleColor[1], g.visibleColor[2], g.opacity);
        o_glDrawElements(mode, count, type, indices);
        glDepthFunc(GL_LEQUAL);
        break;
    }
    case CHAMS_WIREFRAME: {
        glDisable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glColor4f(g.wallColor[0], g.wallColor[1], g.wallColor[2], 1.0f);
        glLineWidth(2.0f);
        o_glDrawElements(mode, count, type, indices);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_DEPTH_TEST); glLineWidth(1.0f);
        break;
    }
    case CHAMS_GLOW: {
        glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL); glDepthMask(GL_FALSE);
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColor4f(g.wallColor[0], g.wallColor[1], g.wallColor[2], 0.3f);
        o_glDrawElements(mode, count, type, indices);
        glDepthMask(GL_TRUE); glDepthFunc(GL_EQUAL);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(g.visibleColor[0], g.visibleColor[1], g.visibleColor[2], g.opacity);
        o_glDrawElements(mode, count, type, indices);
        glDisable(GL_BLEND); glDepthFunc(GL_LEQUAL);
        break;
    }
    case CHAMS_INVISIBLE: {
        glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL); glDepthMask(GL_FALSE);
        glColor4f(g.wallColor[0], g.wallColor[1], g.wallColor[2], g.opacity);
        o_glDrawElements(mode, count, type, indices);
        glDepthMask(GL_TRUE); glDepthFunc(GL_LEQUAL);
        break;
    }
    case CHAMS_RAINBOW: {
        int hi = (int)(g.hue*6)%6; float f = g.hue*6-hi;
        float v=1,p=0,q=1-f,t=f, r,g,b;
        switch(hi){case 0:r=v;g=t;b=p;break;case 1:r=q;g=v;b=p;break;
        case 2:r=p;g=v;b=t;break;case 3:r=p;g=q;b=v;break;
        case 4:r=t;g=p;b=v;break;default:r=v;g=p;b=q;break;}
        glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL); glDepthMask(GL_FALSE);
        glColor4f(r,g,b,g.opacity);
        o_glDrawElements(mode, count, type, indices);
        glDepthMask(GL_TRUE); glDepthFunc(GL_EQUAL);
        glColor4f(r*0.5f,g*0.5f,b*0.5f,g.opacity);
        o_glDrawElements(mode, count, type, indices);
        glDepthFunc(GL_LEQUAL);
        break;
    }
    case CHAMS_METALLIC: {
        glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL); glDepthMask(GL_FALSE);
        glColor4f(g.wallColor[0]*1.5f,g.wallColor[1]*1.5f,g.wallColor[2]*1.5f,g.opacity);
        o_glDrawElements(mode, count, type, indices);
        glDepthMask(GL_TRUE); glDepthFunc(GL_EQUAL);
        glColor4f(g.visibleColor[0],g.visibleColor[1],g.visibleColor[2],0.8f);
        o_glDrawElements(mode, count, type, indices);
        glDepthFunc(GL_LEQUAL);
        break;
    }
    default:
        o_glDrawElements(mode, count, type, indices);
        break;
    }
}

// ============================================================
// SWAP BUFFERS HOOK
// ============================================================
static BOOL WINAPI hk_wglSwapBuffers(HDC hdc) {
    if (!o_wglSwapBuffers) return FALSE;
    if (!g_fontReady) SetupFont(hdc);
    g.hue += 0.005f; if (g.hue > 1.0f) g.hue -= 1.0f;
    BOOL ret = o_wglSwapBuffers(hdc);
    RenderMenu();
    return ret;
}

// ============================================================
// INPUT THREAD
// ============================================================
static DWORD WINAPI InputThread(LPVOID) {
    while (g_running) {
        Sleep(50);

        // F7 - Toggle menu
        if (GetAsyncKeyState(MENU_TOGGLE_KEY) & 1) {
            g.showMenu = !g.showMenu;
        }

        // F8 - Streamer mode
        if (GetAsyncKeyState(STREAMER_MODE_KEY) & 1) {
            g.streamerMode = !g.streamerMode;
        }

        // Menu navigation (only when menu is open)
        if (g.showMenu) {
            if (GetAsyncKeyState(VK_UP) & 1) {
                g.sel--;
                if (g.sel < 0) g.sel = 2;
            }
            if (GetAsyncKeyState(VK_DOWN) & 1) {
                g.sel++;
                if (g.sel > 2) g.sel = 0;
            }
            if (GetAsyncKeyState(VK_RETURN) & 1) {
                switch (g.sel) {
                    case 0: g.enabled = !g.enabled; break;
                    case 1:
                        g.mode = (ChamsMode)((g.mode + 1) % CHAMS_MAX);
                        break;
                    case 2: g.streamerMode = !g.streamerMode; break;
                }
            }
            if (GetAsyncKeyState(VK_LEFT) & 1) {
                if (g.sel == 1)
                    g.mode = (ChamsMode)((g.mode - 1 + CHAMS_MAX) % CHAMS_MAX);
            }
            if (GetAsyncKeyState(VK_RIGHT) & 1) {
                if (g.sel == 1)
                    g.mode = (ChamsMode)((g.mode + 1) % CHAMS_MAX);
            }
        }
    }
    return 0;
}

// ============================================================
// HOOK INSTALL
// ============================================================
static bool InstallOpenGLHooks() {
    HMODULE hGL = GetModuleHandleA("opengl32.dll");
    if (!hGL) hGL = LoadLibraryA("opengl32.dll");
    if (!hGL) return false;

    void* addrWB = GetProcAddress(hGL, "wglSwapBuffers");
    void* addrDE = GetProcAddress(hGL, "glDrawElements");
    if (!addrWB || !addrDE) return false;

    bool ok = true;
    ok &= InstallHook(addrWB, hk_wglSwapBuffers, (void**)&o_wglSwapBuffers);
    ok &= InstallHook(addrDE, hk_glDrawElements, (void**)&o_glDrawElements);
    return ok;
}

// ============================================================
// DLL ENTRY
// ============================================================
BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
    switch (reason) {
    case DLL_PROCESS_ATTACH: {
        g_hInst = hMod;
        g_running = true;
        DisableThreadLibraryCalls(hMod);

        // Try hooks immediately
        if (!InstallOpenGLHooks()) {
            // Retry later (opengl32.dll might not be loaded yet)
            CreateThread(nullptr, 0, [](LPVOID)->DWORD{
                Sleep(2000);
                InstallOpenGLHooks();
                return 0;
            }, nullptr, 0, nullptr);
        }

        CreateThread(nullptr, 0, InputThread, nullptr, 0, nullptr);
        break;
    }
    case DLL_PROCESS_DETACH: {
        g_running = false;
        Sleep(100);
        break;
    }
    }
    return TRUE;
}
