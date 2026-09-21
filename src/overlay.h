/*
 * XPE CHAMS v2 - Overlay (D3D11 + ImGui)
 * Author/Copyright: xpe.nettt
 */

#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/backends/imgui_impl_dx11.h"
#include "../vendor/imgui/backends/imgui_impl_win32.h"
#include "config.h"
#include "keyauth.h"
#include "streammode.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

extern ID3D11Device* g_pDevice;
extern ID3D11DeviceContext* g_pContext;
extern ID3D11RenderTargetView* g_pRenderTarget;
extern IDXGISwapChain* g_pSwapChain;

class Overlay
{
private:
    HWND m_hGameWindow;
    HWND m_hOverlayWindow;
    WNDCLASSEXA m_wc;
    IDXGISwapChain* m_pSwapChain;
    bool m_bInitialized;
    bool m_bMenuOpen;

    struct MenuState {
        bool bESP, bBox, bBoxFilled, bName, bHealth, bArmor, bWeapon, bDistance, bLine;
        int  iBoxType;
        float fBoxThickness;
        bool bChams, bChamsInvisible, bChamsVisible, bChamsWireframe, bChamsFlat;
        int  iChamsType;
        ImColor colInvisible, colVisible, colGlow;
        bool bGlow, bGlowThroughWalls;
        float fGlowSize;
        ImColor colGlowColor;
        bool bAimbot, bAimbotFOV, bAimbotSnap, bAimbotVisibleOnly;
        float fAimbotFOV, fAimbotSmooth, fAimbotDistance;
        int   iAimbotBone;
        bool bWallhack, bWallhackChams, bWallhackGlow, bWallhackWireframe;
        float fWallhackBrightness;
        bool bRadar, bRadarBackground;
        int  iRadarSize;
        float fRadarZoom, fRadarRange;
        bool bWatermark, bCrosshair, bNoRecoil, bNoSpread, bTriggerbot;
        int  iCrosshairType;
        float fCrosshairSize;
        ImColor colCrosshair;
    };

    MenuState m_menu;
    StreamerMode m_streamer;

    bool CreateOverlayWindow();
    bool HookD3D11();
    void RenderMenu();
    void RenderLoginWindow();
    void RenderWatermark();
    void RenderCrosshair();

    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

public:
    Overlay();
    ~Overlay();

    bool Initialize(HWND hGameWindow);
    void Render();
    void Shutdown();
    void ToggleMenu() { m_bMenuOpen = !m_bMenuOpen; }
    bool IsMenuOpen() const { return m_bMenuOpen; }

    static HRESULT WINAPI PresentHook(IDXGISwapChain*, UINT, UINT);
    static HRESULT WINAPI ResizeBuffersHook(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
};

typedef HRESULT(WINAPI* Present_t)(IDXGISwapChain*, UINT, UINT);
typedef HRESULT(WINAPI* ResizeBuffers_t)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

extern Present_t OriginalPresent;
extern ResizeBuffers_t OriginalResizeBuffers;