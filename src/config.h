/*
 * XPE CHAMS v2 - Config & Global State
 * Reconstructed from CHAMSMENU.dll by xpe.nettt
 */

#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <string>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

#define XPE_CHAMS_TITLE    "XPE CHAMS"
#define XPE_CHAMS_VERSION  "2.0"
#define XPE_CHAMS_AUTHOR   "xpe.nettt"
#define XPE_CHAMS_COPYRIGHT "xpe.nettt"

#define KEYAUTH_OWNERID   "xpe"
#define KEYAUTH_APPNAME   "XPE CHAMS"
#define KEYAUTH_VERSION   "2.0"
#define KEYAUTH_API_URL   "https://xpe-chams-keyauth.vercel.app/api/auth"

struct Config_t
{
    bool bESP;
    bool bBox;
    bool bBoxFilled;
    bool bName;
    bool bHealth;
    bool bArmor;
    bool bWeapon;
    bool bDistance;
    bool bLine;
    int  iBoxType;
    float fBoxThickness;
    bool bChams;
    bool bChamsInvisible;
    bool bChamsVisible;
    bool bChamsWireframe;
    bool bChamsFlat;
    int  iChamsType;
    float colInvisible[4];
    float colVisible[4];
    float colGlow[4];
    bool bWallhack;
    bool bWallhackChams;
    bool bWallhackGlow;
    bool bWallhackWireframe;
    float fWallhackBrightness;
    bool bGlow;
    bool bGlowThroughWalls;
    float fGlowSize;
    float colGlowColor[4];
    bool bAimbot;
    bool bAimbotFOV;
    bool bAimbotSnap;
    bool bAimbotVisibleOnly;
    float fAimbotFOV;
    float fAimbotSmooth;
    float fAimbotDistance;
    int   iAimbotBone;
    bool bRadar;
    bool bRadarBackground;
    int  iRadarSize;
    float fRadarZoom;
    float fRadarRange;
    bool bWatermark;
    bool bCrosshair;
    bool bNoRecoil;
    bool bNoSpread;
    bool bTriggerbot;
    int  iCrosshairType;
    float fCrosshairSize;
    float colCrosshair[4];
    bool bAuthenticated;
    bool bShowLogin;
    char szLicenseKey[256];
    char szUsername[64];
    bool bStreamerMode;
    bool bDebugLog;
    char szLogFile[MAX_PATH];
    ID3D11Device* pDevice;
    ID3D11DeviceContext* pContext;
    IDXGISwapChain* pSwapChain;
    ID3D11RenderTargetView* pRenderTarget;
    HWND hGameWindow;
    HWND hOverlayWindow;
    int  nScreenWidth;
    int  nScreenHeight;

    Config_t()
    {
        memset(this, 0, sizeof(*this));
        bESP = true;
        bBox = true;
        bName = true;
        bHealth = true;
        fBoxThickness = 1.5f;
        colInvisible[0] = 1.0f; colInvisible[1] = 0.0f; colInvisible[2] = 0.0f; colInvisible[3] = 1.0f;
        colVisible[0] = 0.0f; colVisible[1] = 1.0f; colVisible[2] = 0.0f; colVisible[3] = 1.0f;
        colGlow[0] = 0.0f; colGlow[1] = 0.6f; colGlow[2] = 1.0f; colGlow[3] = 1.0f;
        colGlowColor[0] = 0.0f; colGlowColor[1] = 0.6f; colGlowColor[2] = 1.0f; colGlowColor[3] = 0.7f;
        colCrosshair[0] = 0.0f; colCrosshair[1] = 1.0f; colCrosshair[2] = 0.0f; colCrosshair[3] = 1.0f;
        fAimbotFOV = 10.0f;
        fAimbotSmooth = 5.0f;
        fAimbotDistance = 200.0f;
        iRadarSize = 200;
        fRadarZoom = 50.0f;
        fRadarRange = 300.0f;
        fWallhackBrightness = 0.5f;
        fCrosshairSize = 10.0f;
        fGlowSize = 3.0f;
        bWatermark = true;
        strcpy_s(szLogFile, "xpe_chams.log");
    }

    void Save(const char* filename)
    {
        FILE* f = NULL;
        fopen_s(&f, filename, "w");
        if (!f) return;
        fprintf(f, "[XPE CHAMS]\n");
        fprintf(f, "ESP=%d\n", bESP);
        fprintf(f, "Box=%d\n", bBox);
        fprintf(f, "BoxFilled=%d\n", bBoxFilled);
        fprintf(f, "Name=%d\n", bName);
        fprintf(f, "Health=%d\n", bHealth);
        fprintf(f, "Armor=%d\n", bArmor);
        fprintf(f, "Weapon=%d\n", bWeapon);
        fprintf(f, "Distance=%d\n", bDistance);
        fprintf(f, "Line=%d\n", bLine);
        fprintf(f, "BoxType=%d\n", iBoxType);
        fprintf(f, "BoxThickness=%.1f\n", fBoxThickness);
        fprintf(f, "Chams=%d\n", bChams);
        fprintf(f, "ChamsType=%d\n", iChamsType);
        fprintf(f, "Wallhack=%d\n", bWallhack);
        fprintf(f, "Glow=%d\n", bGlow);
        fprintf(f, "GlowSize=%.1f\n", fGlowSize);
        fprintf(f, "Aimbot=%d\n", bAimbot);
        fprintf(f, "AimbotFOV=%.1f\n", fAimbotFOV);
        fprintf(f, "AimbotSmooth=%.1f\n", fAimbotSmooth);
        fprintf(f, "AimbotDistance=%.1f\n", fAimbotDistance);
        fprintf(f, "Radar=%d\n", bRadar);
        fprintf(f, "RadarSize=%d\n", iRadarSize);
        fprintf(f, "Watermark=%d\n", bWatermark);
        fprintf(f, "Crosshair=%d\n", bCrosshair);
        fprintf(f, "CrosshairType=%d\n", iCrosshairType);
        fprintf(f, "StreamerMode=%d\n", bStreamerMode);
        fclose(f);
    }

    void Load(const char* filename)
    {
        FILE* f = NULL;
        fopen_s(&f, filename, "r");
        if (!f) return;
        char line[256];
        while (fgets(line, sizeof(line), f))
        {
            size_t len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') line[--len] = 0;
            int val; float fval;
            if (sscanf_s(line, "ESP=%d", &val)) bESP = val;
            else if (sscanf_s(line, "Box=%d", &val)) bBox = val;
            else if (sscanf_s(line, "BoxFilled=%d", &val)) bBoxFilled = val;
            else if (sscanf_s(line, "Name=%d", &val)) bName = val;
            else if (sscanf_s(line, "Health=%d", &val)) bHealth = val;
            else if (sscanf_s(line, "Armor=%d", &val)) bArmor = val;
            else if (sscanf_s(line, "Weapon=%d", &val)) bWeapon = val;
            else if (sscanf_s(line, "Distance=%d", &val)) bDistance = val;
            else if (sscanf_s(line, "Line=%d", &val)) bLine = val;
            else if (sscanf_s(line, "BoxType=%d", &val)) iBoxType = val;
            else if (sscanf_s(line, "BoxThickness=%f", &fval)) fBoxThickness = fval;
            else if (sscanf_s(line, "Chams=%d", &val)) bChams = val;
            else if (sscanf_s(line, "ChamsType=%d", &val)) iChamsType = val;
            else if (sscanf_s(line, "Wallhack=%d", &val)) bWallhack = val;
            else if (sscanf_s(line, "Glow=%d", &val)) bGlow = val;
            else if (sscanf_s(line, "GlowSize=%f", &fval)) fGlowSize = fval;
            else if (sscanf_s(line, "Aimbot=%d", &val)) bAimbot = val;
            else if (sscanf_s(line, "AimbotFOV=%f", &fval)) fAimbotFOV = fval;
            else if (sscanf_s(line, "AimbotSmooth=%f", &fval)) fAimbotSmooth = fval;
            else if (sscanf_s(line, "AimbotDistance=%f", &fval)) fAimbotDistance = fval;
            else if (sscanf_s(line, "Radar=%d", &val)) bRadar = val;
            else if (sscanf_s(line, "RadarSize=%d", &val)) iRadarSize = val;
            else if (sscanf_s(line, "Watermark=%d", &val)) bWatermark = val;
            else if (sscanf_s(line, "Crosshair=%d", &val)) bCrosshair = val;
            else if (sscanf_s(line, "CrosshairType=%d", &val)) iCrosshairType = val;
            else if (sscanf_s(line, "StreamerMode=%d", &val)) bStreamerMode = val;
        }
        fclose(f);
    }

    void Log(const char* fmt, ...)
    {
        if (!bDebugLog) return;
        FILE* f = NULL;
        fopen_s(&f, szLogFile, "a");
        if (!f) return;
        va_list args;
        va_start(args, fmt);
        char buffer[512];
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        SYSTEMTIME st;
        GetLocalTime(&st);
        fprintf(f, "[%02d:%02d:%02d.%03d] %s\n", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, buffer);
        fclose(f);
    }
};

extern Config_t g_Config;