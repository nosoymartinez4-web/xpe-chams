/*
 * XPE CHAMS v2 - Overlay implementation (D3D11 + ImGui)
 * OpenGL Chams + D3D11 Overlay with KeyAuth license validation
 * Author: xpe.nettt / Stealth Proyects
 * Toggle: F9
 */

#include "overlay.h"
#include "config.h"
#include "keyauth.h"
#include <d3d11.h>
#include <dxgi.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <string>
#include <cstdio>
#include <thread>
#include <chrono>
#include "imgui.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dwmapi.lib")

// No WRL needed

// ========================
// GLOBALS
// ========================
Overlay g_Overlay;

// KeyAuth
extern KeyAuthClass g_KeyAuth;

// D3D11
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// ImGui
static bool g_ImGuiInitialized = false;
static bool g_OverlayVisible = false;
static bool g_Authenticated = false;
static bool g_AuthFailed = false;
static char g_LicenseInput[256] = {0};
static char g_AuthMessage[256] = {0};
static int g_AuthMessageType = 0; // 0=none, 1=success, 2=error, 3=info

// Chams config
static float g_ColorR = 1.0f, g_ColorG = 0.0f, g_ColorB = 0.0f;
static float g_ColorR2 = 1.0f, g_ColorG2 = 1.0f, g_ColorB2 = 0.0f;
static bool g_ChamsEnabled = true;
static int g_ChamsMode = 0; // 0=flat, 1=wireframe, 2=texture, 3=glow
static bool g_StreamMode = false;
static float g_GlowIntensity = 0.5f;

// ========================
// FORWARD DECLARATIONS
// ========================
static void CreateRenderTarget();
static void CleanupRenderTarget();
static void InitImGui();
static void RenderOverlay();
static void DrawLicenseScreen();
static void DrawChamsPanel();
static void CheckAuthStatus();
static void UpdateAuthMessage(const char* msg, int type);

// ========================
// HOOKED PRESENT
// ========================
typedef HRESULT(WINAPI* Present_t)(IDXGISwapChain*, UINT, UINT);
Present_t OriginalPresent = nullptr;

HRESULT WINAPI PresentHook(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    g_pSwapChain = pSwapChain;

    if (!g_ImGuiInitialized)
    {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pd3dDevice)))
        {
            g_pd3dDevice->GetImmediateContext(&g_pd3dDeviceContext);
            InitImGui();
        }
    }

    // Check auth periodically
    static auto lastAuthCheck = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    if (g_Authenticated && std::chrono::duration_cast<std::chrono::seconds>(now - lastAuthCheck).count() > 60)
    {
        lastAuthCheck = now;
        std::thread([]() { CheckAuthStatus(); }).detach();
    }

    // Render
    if (g_ImGuiInitialized)
    {
        // Toggle with F9
        static bool lastF9 = false;
        bool currentF9 = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
        if (currentF9 && !lastF9)
            g_OverlayVisible = !g_OverlayVisible;
        lastF9 = currentF9;

        RenderOverlay();
    }

    // Call original
    if (OriginalPresent)
        return OriginalPresent(pSwapChain, SyncInterval, Flags);

    return S_OK;
}

// ========================
// RESIZE BUFFERS HOOK
// ========================
typedef HRESULT(WINAPI* ResizeBuffers_t)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
ResizeBuffers_t OriginalResizeBuffers = nullptr;

HRESULT WINAPI ResizeBuffersHook(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    if (g_ImGuiInitialized)
    {
        CleanupRenderTarget();
    }

    HRESULT hr = OriginalResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

    if (SUCCEEDED(hr) && g_ImGuiInitialized)
    {
        CreateRenderTarget();
    }

    return hr;
}

// ========================
// INIT IMGUI
// ========================
static void InitImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();

    // Style customization
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.WindowPadding = ImVec2(12, 12);
    style.FramePadding = ImVec2(8, 6);
    style.ItemSpacing = ImVec2(8, 6);

    // Colors (dark red theme)
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]          = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
    colors[ImGuiCol_TitleBg]           = ImVec4(0.12f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgActive]     = ImVec4(0.20f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_Border]            = ImVec4(0.30f, 0.00f, 0.00f, 0.50f);
    colors[ImGuiCol_FrameBg]           = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]    = ImVec4(0.20f, 0.05f, 0.05f, 1.00f);
    colors[ImGuiCol_FrameBgActive]     = ImVec4(0.30f, 0.05f, 0.05f, 1.00f);
    colors[ImGuiCol_Button]            = ImVec4(0.15f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_ButtonHovered]     = ImVec4(0.25f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_ButtonActive]      = ImVec4(0.35f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_Header]            = ImVec4(0.20f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_HeaderHovered]     = ImVec4(0.30f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_HeaderActive]      = ImVec4(0.40f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_CheckMark]         = ImVec4(0.80f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_SliderGrab]        = ImVec4(0.80f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]  = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_Text]              = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled]      = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

    // Font
    io.Fonts->AddFontDefault();

    // Initialize ImGui backends
    ImGui_ImplWin32_Init(FindWindowA(nullptr, nullptr));
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    CreateRenderTarget();
    g_ImGuiInitialized = true;
}

// ========================
// RENDER TARGET
// ========================
static void CreateRenderTarget()
{
    if (!g_pSwapChain) return;

    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    if (pBackBuffer)
    {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

static void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// ========================
// AUTH FUNCTIONS
// ========================
static void UpdateAuthMessage(const char* msg, int type)
{
    strncpy_s(g_AuthMessage, msg, sizeof(g_AuthMessage) - 1);
    g_AuthMessageType = type;
}

static void CheckAuthStatus()
{
    if (!g_KeyAuth.IsLoggedIn()) return;

    bool stillValid = g_KeyAuth.CheckSubscription();
    if (!stillValid)
    {
        g_Authenticated = false;
        g_AuthFailed = true;
        UpdateAuthMessage("Tu licencia ha expirado. Contacta 849 639 3107", 2);
    }
}

static void AttemptLogin(const char* key)
{
    if (!key || !key[0]) return;

    UpdateAuthMessage("Verificando licencia...", 3);

    if (g_KeyAuth.Login(key))
    {
        g_Authenticated = true;
        g_AuthFailed = false;
        char msg[256];
        snprintf(msg, sizeof(msg), "Licencia valida. Bienvenido %s", g_KeyAuth.GetUsername());
        UpdateAuthMessage(msg, 1);
    }
    else
    {
        g_Authenticated = false;
        g_AuthFailed = true;
        char msg[256];
        const char* err = g_KeyAuth.GetUsername();
        if (err && err[0])
            snprintf(msg, sizeof(msg), "Error: %s", err);
        else
            snprintf(msg, sizeof(msg), "Error: Licencia invalida");
        UpdateAuthMessage(msg, 2);
    }
}

// ========================
// LICENSE SCREEN
// ========================
static void DrawLicenseScreen()
{
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 screenSize = io.DisplaySize;

    ImGui::SetNextWindowPos(ImVec2(screenSize.x * 0.5f, screenSize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(380, 0), ImGuiCond_Always);

    ImGui::Begin("XPE CHAMS v2", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

    // Header
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.80f, 0.00f, 0.00f, 1.00f));
    ImGui::SetWindowFontScale(1.4f);
    ImGui::Text("XPE CHAMS v2");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.50f, 0.50f, 1.00f));
    ImGui::Text("by xpe.nettt / Stealth Proyects");
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    if (g_Authenticated)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.00f, 0.80f, 0.30f, 1.00f));
        ImGui::Text("Licencia verificada");
        ImGui::PopStyleColor();

        ImGui::Text("Usuario: %s", g_KeyAuth.GetUsername());
        ImGui::Text("Vence:   %s", g_KeyAuth.GetExpiry());

        if (g_KeyAuth.GetRemaining() && g_KeyAuth.GetRemaining()[0])
        {
            ImGui::Text("Restante: %s", g_KeyAuth.GetRemaining());
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("ABRIR PANEL", ImVec2(ImGui::GetContentRegionAvail().x, 40)))
        {
            g_OverlayVisible = true;
        }

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.40f, 0.40f, 1.00f));
        ImGui::Text("Presiona F9 para abrir/cerrar el panel");
        ImGui::PopStyleColor();
    }
    else
    {
        ImGui::Text("Ingresa tu licencia:");

        ImGui::PushItemWidth(-1);
        ImGui::InputText("##license", g_LicenseInput, sizeof(g_LicenseInput));
        ImGui::PopItemWidth();

        if (ImGui::Button("VALIDAR LICENCIA", ImVec2(ImGui::GetContentRegionAvail().x, 36)))
        {
            AttemptLogin(g_LicenseInput);
        }

        ImGui::Spacing();

        if (g_AuthMessage[0])
        {
            ImVec4 color;
            switch (g_AuthMessageType)
            {
                case 1: color = ImVec4(0.00f, 0.80f, 0.30f, 1.00f); break;
                case 2: color = ImVec4(0.80f, 0.00f, 0.00f, 1.00f); break;
                case 3: color = ImVec4(0.80f, 0.80f, 0.00f, 1.00f); break;
                default: color = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
            }
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextWrapped("%s", g_AuthMessage);
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.60f, 0.60f, 1.00f));
        ImGui::Text("Sin licencia? Contacta:");
        ImGui::PopStyleColor();

        if (ImGui::Button("WhatsApp 849 639 3107", ImVec2(ImGui::GetContentRegionAvail().x, 30)))
        {
            ShellExecuteA(nullptr, "open", "https://wa.me/18496393107", nullptr, nullptr, SW_SHOWNORMAL);
        }

        if (ImGui::Button("Discord", ImVec2(ImGui::GetContentRegionAvail().x, 30)))
        {
            ShellExecuteA(nullptr, "open", "https://discord.gg/My6QkneU6j", nullptr, nullptr, SW_SHOWNORMAL);
        }
    }

    ImGui::End();
}

// ========================
// CHAMS PANEL
// ========================
static void DrawChamsPanel()
{
    if (!g_OverlayVisible) return;

    ImGui::SetNextWindowSize(ImVec2(320, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);

    ImGui::Begin("XPE CHAMS v2 - Panel", &g_OverlayVisible,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

    if (ImGui::CollapsingHeader("Chams", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Activar Chams", &g_ChamsEnabled);

        ImGui::Spacing();
        ImGui::Text("Modo:");
        const char* modes[] = { "Solido", "Wireframe", "Textura", "Glow" };
        ImGui::Combo("##mode", &g_ChamsMode, modes, IM_ARRAYSIZE(modes));

        ImGui::Spacing();
        ImGui::Text("Color principal:");
        ImGui::ColorEdit3("##color1", &g_ColorR, ImGuiColorEditFlags_NoInputs);

        if (g_ChamsMode == 3)
        {
            ImGui::Text("Color secundario (Glow):");
            ImGui::ColorEdit3("##color2", &g_ColorR2, ImGuiColorEditFlags_NoInputs);

            ImGui::Text("Intensidad Glow:");
            ImGui::SliderFloat("##glow", &g_GlowIntensity, 0.0f, 1.0f, "%.2f");
        }
    }

    if (ImGui::CollapsingHeader("Stream Mode", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Activar Stream Mode", &g_StreamMode);
        if (g_StreamMode)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.80f, 0.80f, 0.00f, 1.00f));
            ImGui::Text("Stream Mode activo - chams ocultos en streaming");
            ImGui::PopStyleColor();
        }
    }

    if (ImGui::CollapsingHeader("Informacion"))
    {
        ImGui::Text("Usuario: %s", g_KeyAuth.GetUsername());
        ImGui::Text("Licencia vence: %s", g_KeyAuth.GetExpiry());
        if (g_KeyAuth.GetRemaining() && g_KeyAuth.GetRemaining()[0])
            ImGui::Text("Tiempo restante: %s", g_KeyAuth.GetRemaining());
        ImGui::Text("Toggle: F9");
    }

    ImGui::Separator();

    if (ImGui::Button("CERRAR SESION", ImVec2(ImGui::GetContentRegionAvail().x, 30)))
    {
        g_Authenticated = false;
        g_OverlayVisible = false;
        g_LicenseInput[0] = 0;
        g_AuthMessage[0] = 0;
        g_AuthMessageType = 0;
    }

    if (ImGui::Button("SOPORTE WhatsApp", ImVec2(ImGui::GetContentRegionAvail().x, 30)))
    {
        ShellExecuteA(nullptr, "open", "https://wa.me/18496393107", nullptr, nullptr, SW_SHOWNORMAL);
    }

    ImGui::End();
}

// ========================
// RENDER
// ========================
static void RenderOverlay()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (!g_Authenticated)
    {
        DrawLicenseScreen();
    }
    else
    {
        if (g_OverlayVisible)
        {
            DrawChamsPanel();
        }

        if (!g_OverlayVisible)
        {
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.3f);
            ImGui::Begin("##watermark", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.80f, 0.00f, 0.00f, 0.60f));
            ImGui::Text("XPE CHAMS v2 - F9");
            ImGui::PopStyleColor();

            ImGui::End();
        }
    }

    ImGui::Render();

    if (g_pd3dDeviceContext && g_mainRenderTargetView)
    {
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
}

// ========================
// HOOK D3D11
// ========================
static bool HookD3D11()
{
    ID3D11Device* dev = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    IDXGISwapChain* swap = nullptr;

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.Width = 1;
    sd.BufferDesc.Height = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = GetDesktopWindow();
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE,
        nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &sd, &swap, &dev, nullptr, &ctx
    );

    if (FAILED(hr) || !swap) return false;

    void** vtable = *(void***)swap;
    OriginalPresent = (Present_t)vtable[8];
    OriginalResizeBuffers = (ResizeBuffers_t)vtable[13];

    DWORD oldProtect;
    VirtualProtect(&vtable[8], sizeof(void*), PAGE_READWRITE, &oldProtect);
    vtable[8] = (void*)PresentHook;
    VirtualProtect(&vtable[8], sizeof(void*), oldProtect, &oldProtect);

    VirtualProtect(&vtable[13], sizeof(void*), PAGE_READWRITE, &oldProtect);
    vtable[13] = (void*)ResizeBuffersHook;
    VirtualProtect(&vtable[13], sizeof(void*), oldProtect, &oldProtect);

    ctx->Release();
    dev->Release();
    swap->Release();

    return true;
}

// ========================
// PUBLIC METHODS
// ========================
void Overlay::Initialize()
{
    HookD3D11();
}

void Overlay::Shutdown()
{
    if (g_ImGuiInitialized)
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        g_ImGuiInitialized = false;
    }

    CleanupRenderTarget();

    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}