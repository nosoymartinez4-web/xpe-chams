/*
 * XPE CHAMS v2 - Overlay implementation (D3D11 + ImGui)
 * Author/Copyright: xpe.nettt
 * 
 * Reconstructed from CHAMSMENU.dll
 * Full feature set: Visuals, Chams, Glow, Wallhack, Aimbot, Radar, Misc
 */

#include "overlay.h"
#include <dwmapi.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "d3d11.lib")

// ============================================================
// Forward declarations
// ============================================================
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
Overlay* g_pOverlay = nullptr;

// ============================================================
// Global D3D11 hook state
// ============================================================
ID3D11Device* g_pDevice = nullptr;
ID3D11DeviceContext* g_pContext = nullptr;
ID3D11RenderTargetView* g_pRenderTarget = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;

// ============================================================
// Present hook trampoline
// ============================================================
typedef HRESULT(WINAPI* Present_t)(IDXGISwapChain*, UINT, UINT);
Present_t OriginalPresent = nullptr;

typedef HRESULT(WINAPI* ResizeBuffers_t)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
ResizeBuffers_t OriginalResizeBuffers = nullptr;

// ============================================================
// Constructor / Destructor
// ============================================================
Overlay::Overlay()
    : m_hGameWindow(NULL)
    , m_hOverlayWindow(NULL)
    , m_pSwapChain(nullptr)
    , m_bInitialized(false)
    , m_bMenuOpen(false)
{
    ZeroMemory(&m_wc, sizeof(m_wc));
    ZeroMemory(&m_menu, sizeof(m_menu));

    m_menu.bESP = true;
    m_menu.bBox = true;
    m_menu.bName = true;
    m_menu.bHealth = true;
    m_menu.iBoxType = 0;
    m_menu.fBoxThickness = 1.5f;
    m_menu.colInvisible = ImColor(255, 0, 0, 255);
    m_menu.colVisible = ImColor(0, 255, 0, 255);
    m_menu.colGlow = ImColor(0, 150, 255, 255);
    m_menu.colGlowColor = ImColor(0, 150, 255, 180);
    m_menu.fGlowSize = 3.0f;
    m_menu.fAimbotFOV = 10.0f;
    m_menu.fAimbotSmooth = 5.0f;
    m_menu.fAimbotDistance = 200.0f;
    m_menu.iRadarSize = 200;
    m_menu.fRadarZoom = 50.0f;
    m_menu.fRadarRange = 300.0f;
    m_menu.fWallhackBrightness = 0.5f;
    m_menu.colCrosshair = ImColor(0, 255, 0, 255);
    m_menu.fCrosshairSize = 10.0f;
    m_menu.bWatermark = true;

    g_pOverlay = this;
}

Overlay::~Overlay()
{
    Shutdown();
}

// ============================================================
// Create overlay window
// ============================================================
bool Overlay::CreateOverlayWindow()
{
    m_wc.cbSize = sizeof(WNDCLASSEXA);
    m_wc.style = CS_HREDRAW | CS_VREDRAW;
    m_wc.lpfnWndProc = WndProc;
    m_wc.cbClsExtra = 0;
    m_wc.cbWndExtra = 0;
    m_wc.hInstance = GetModuleHandle(NULL);
    m_wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    m_wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(0, 0, 0));
    m_wc.lpszClassName = "XPE_OVERLAY_CLASS";

    if (!RegisterClassExA(&m_wc))
        return false;

    RECT rect;
    GetWindowRect(m_hGameWindow, &rect);

    m_hOverlayWindow = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE,
        "XPE_OVERLAY_CLASS",
        "XPE CHAMS Overlay",
        WS_POPUP,
        rect.left, rect.top,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, m_wc.hInstance, NULL
    );

    if (!m_hOverlayWindow)
        return false;

    SetLayeredWindowAttributes(m_hOverlayWindow, RGB(0, 0, 0), 0, LWA_COLORKEY);

    LONG style = GetWindowLong(m_hOverlayWindow, GWL_EXSTYLE);
    SetWindowLong(m_hOverlayWindow, GWL_EXSTYLE, style | WS_EX_TRANSPARENT);

    ShowWindow(m_hOverlayWindow, SW_SHOW);
    UpdateWindow(m_hOverlayWindow);

    return true;
}

// ============================================================
// Hook D3D11 present
// ============================================================
bool Overlay::HookD3D11()
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
    sd.OutputWindow = m_hOverlayWindow;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE,
        nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &sd, &swap, &dev, nullptr, &ctx
    );

    if (!swap) return false;

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

// ============================================================
// Initialize overlay
// ============================================================
bool Overlay::Initialize(HWND hGameWindow)
{
    m_hGameWindow = hGameWindow;

    if (!CreateOverlayWindow())
    {
        g_Config.Log("Overlay: Failed to create overlay window");
        return false;
    }

    if (!HookD3D11())
    {
        g_Config.Log("Overlay: Failed to hook D3D11");
        return false;
    }

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.ScrollbarSize = 12.0f;
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.80f, 0.40f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.30f, 0.90f, 0.60f);

    ImGui_ImplWin32_Init(m_hOverlayWindow);
    ImGui_ImplDX11_Init(g_pDevice, g_pContext);

    m_bInitialized = true;
    g_Config.Log("Overlay initialized successfully");
    return true;
}

// ============================================================
// Present hook (D3D11)
// ============================================================
HRESULT WINAPI Overlay::PresentHook(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    if (!g_pOverlay)
        return OriginalPresent(pSwapChain, SyncInterval, Flags);

    if (!g_pDevice || !g_pContext)
    {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pDevice)))
        {
            g_pDevice->GetImmediateContext(&g_pContext);
            g_pOverlay->m_pSwapChain = pSwapChain;

            ID3D11Texture2D* pBuffer = nullptr;
            pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBuffer);
            if (pBuffer)
            {
                g_pDevice->CreateRenderTargetView(pBuffer, nullptr, &g_pRenderTarget);
                pBuffer->Release();
            }

            // Re-init ImGui with the real device
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplDX11_Init(g_pDevice, g_pContext);
        }
    }

    HRESULT hr = OriginalPresent(pSwapChain, SyncInterval, Flags);

    if (g_pDevice && g_pContext && g_pRenderTarget)
    {
        g_pOverlay->Render();
    }

    return hr;
}

// ============================================================
// ResizeBuffers hook
// ============================================================
HRESULT WINAPI Overlay::ResizeBuffersHook(IDXGISwapChain* pSwapChain, UINT BufferCount,
    UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    if (g_pRenderTarget)
    {
        g_pRenderTarget->Release();
        g_pRenderTarget = nullptr;
    }

    HRESULT hr = OriginalResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

    if (SUCCEEDED(hr))
    {
        ID3D11Texture2D* pBuffer = nullptr;
        pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBuffer);
        if (pBuffer)
        {
            g_pDevice->CreateRenderTargetView(pBuffer, nullptr, &g_pRenderTarget);
            pBuffer->Release();
        }
    }

    return hr;
}

// ============================================================
// Main render
// ============================================================
void Overlay::Render()
{
    if (!m_bInitialized || !g_pDevice || !g_pContext)
        return;

    // Start ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Handle input passthrough
    LONG style = GetWindowLong(m_hOverlayWindow, GWL_EXSTYLE);
    if (m_bMenuOpen)
    {
        SetWindowLong(m_hOverlayWindow, GWL_EXSTYLE, style & ~WS_EX_TRANSPARENT);
        SetForegroundWindow(m_hOverlayWindow);
    }
    else
    {
        SetWindowLong(m_hOverlayWindow, GWL_EXSTYLE, style | WS_EX_TRANSPARENT);
    }

    // INSERT key to toggle menu
    if (GetAsyncKeyState(VK_INSERT) & 1)
        ToggleMenu();

    // Login window
    if (g_Config.bShowLogin)
        RenderLoginWindow();

    // Main menu
    if (m_bMenuOpen)
        RenderMenu();

    // Watermark
    if (m_menu.bWatermark)
        RenderWatermark();

    // Crosshair
    if (m_menu.bCrosshair)
        RenderCrosshair();

    // Streamer mode toggle (F6)
    if (GetAsyncKeyState(VK_F6) & 1)
    {
        m_streamer.Toggle();
        g_Config.Log("Streamer mode: %s", m_streamer.IsStreaming() ? "ON" : "OFF");
    }

    // Render
    ImGui::Render();
    if (g_pRenderTarget)
    {
        g_pContext->OMSetRenderTargets(1, &g_pRenderTarget, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
}

// ============================================================
// Login window
// ============================================================
void Overlay::RenderLoginWindow()
{
    static char key[256] = { 0 };
    static char status[256] = "Enter your license key";
    static bool loggingIn = false;

    ImGui::SetNextWindowSize(ImVec2(350, 200));
    ImGui::SetNextWindowPos(ImVec2(
        (GetSystemMetrics(SM_CXSCREEN) - 350) / 2.0f,
        (GetSystemMetrics(SM_CYSCREEN) - 200) / 2.0f
    ));

    ImGui::Begin("XPE CHAMS - Login", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar);

    ImGui::TextWrapped("Welcome to XPE CHAMS\nAuthor: xpe.nettt\n");
    ImGui::Separator();

    ImGui::Text("License Key:");
    ImGui::PushItemWidth(-1);
    ImGui::InputText("##key", key, sizeof(key));
    ImGui::PopItemWidth();

    if (loggingIn)
    {
        ImGui::Text("Authenticating...");
    }
    else if (ImGui::Button("Login", ImVec2(-1, 30)))
    {
        loggingIn = true;
        strcpy_s(status, "Authenticating...");

        if (g_KeyAuth.login(key))
        {
            g_Config.bShowLogin = false;
            g_Config.bAuthenticated = true;

            HKEY hKey;
            if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\XPE CHAMS",
                0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
            {
                RegSetValueExA(hKey, "LicenseKey", 0, REG_SZ,
                    (const BYTE*)key, (DWORD)strlen(key) + 1);
                RegCloseKey(hKey);
            }

            strcpy_s(status, "Authenticated!");
        }
        else
        {
            strcpy_s(status, "Invalid key!");
        }
        loggingIn = false;
    }

    ImGui::TextColored(ImVec4(1, 1, 0, 1), "%s", status);
    ImGui::End();
}

// ============================================================
// Main menu
// ============================================================
void Overlay::RenderMenu()
{
    ImGui::SetNextWindowSize(ImVec2(520, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);

    ImGui::Begin("XPE CHAMS v2", &m_bMenuOpen,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);

    if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton))
    {
        if (ImGui::BeginTabItem("Visuals"))
        {
            ImGui::BeginChild("##vis", ImVec2(0, 0), true);
            ImGui::Checkbox("Enable ESP", &m_menu.bESP);
            ImGui::Checkbox("Box ESP", &m_menu.bBox);
            ImGui::Checkbox("Filled Box", &m_menu.bBoxFilled);
            ImGui::Checkbox("Name ESP", &m_menu.bName);
            ImGui::Checkbox("Health Bar", &m_menu.bHealth);
            ImGui::Checkbox("Armor Bar", &m_menu.bArmor);
            ImGui::Checkbox("Weapon Name", &m_menu.bWeapon);
            ImGui::Checkbox("Distance", &m_menu.bDistance);
            ImGui::Checkbox("Line ESP", &m_menu.bLine);
            ImGui::Separator();
            ImGui::Combo("Box Type", &m_menu.iBoxType, "Corner\0Full 2D\0Box 3D\0\0");
            ImGui::SliderFloat("Thickness", &m_menu.fBoxThickness, 0.5f, 5.0f, "%.1f");
            ImGui::Separator();
            ImGui::Checkbox("Wallhack", &m_menu.bWallhack);
            ImGui::Checkbox("WH + Chams", &m_menu.bWallhackChams);
            ImGui::Checkbox("WH + Glow", &m_menu.bWallhackGlow);
            ImGui::SliderFloat("Brightness", &m_menu.fWallhackBrightness, 0.0f, 1.0f, "%.2f");
            ImGui::Separator();
            ImGui::Checkbox("Glow ESP", &m_menu.bGlow);
            ImGui::Checkbox("Glow Through Walls", &m_menu.bGlowThroughWalls);
            ImGui::SliderFloat("Glow Size", &m_menu.fGlowSize, 1.0f, 10.0f, "%.1f");
            ImGui::ColorEdit4("Glow Color", (float*)&m_menu.colGlowColor, ImGuiColorEditFlags_NoInputs);
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Chams"))
        {
            ImGui::BeginChild("##chams", ImVec2(0, 0), true);
            ImGui::Checkbox("Enable Chams", &m_menu.bChams);
            ImGui::Separator();
            ImGui::Combo("Chams Type", &m_menu.iChamsType,
                "Shaded\0Flat\0Wireframe\0Glow\0\0");
            ImGui::Separator();
            ImGui::Text("Invisible (behind wall)");
            ImGui::ColorEdit4("Invis Color", (float*)&m_menu.colInvisible,
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
            ImGui::Separator();
            ImGui::Text("Visible (in front)");
            ImGui::ColorEdit4("Vis Color", (float*)&m_menu.colVisible,
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
            ImGui::Separator();
            ImGui::Checkbox("Wireframe Mode", &m_menu.bChamsWireframe);
            ImGui::Checkbox("Flat Shading", &m_menu.bChamsFlat);
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Aimbot"))
        {
            ImGui::BeginChild("##aim", ImVec2(0, 0), true);
            ImGui::Checkbox("Enable Aimbot", &m_menu.bAimbot);
            ImGui::Checkbox("Draw FOV Circle", &m_menu.bAimbotFOV);
            ImGui::Checkbox("Snap Aim", &m_menu.bAimbotSnap);
            ImGui::Checkbox("Visible Only", &m_menu.bAimbotVisibleOnly);
            ImGui::Separator();
            ImGui::SliderFloat("FOV Radius", &m_menu.fAimbotFOV, 1.0f, 90.0f, "%.0f deg");
            ImGui::SliderFloat("Smoothness", &m_menu.fAimbotSmooth, 1.0f, 30.0f, "%.0f");
            ImGui::SliderFloat("Max Distance", &m_menu.fAimbotDistance, 10.0f, 500.0f, "%.0f m");
            ImGui::Separator();
            ImGui::Combo("Aim Bone", &m_menu.iAimbotBone, "Head\0Neck\0Chest\0Pelvis\0\0");
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Radar"))
        {
            ImGui::BeginChild("##radar", ImVec2(0, 0), true);
            ImGui::Checkbox("Enable Radar", &m_menu.bRadar);
            ImGui::Checkbox("Radar Background", &m_menu.bRadarBackground);
            ImGui::Separator();
            ImGui::SliderInt("Radar Size", &m_menu.iRadarSize, 100, 400, "%d px");
            ImGui::SliderFloat("Radar Zoom", &m_menu.fRadarZoom, 10.0f, 200.0f, "%.0f");
            ImGui::SliderFloat("Radar Range", &m_menu.fRadarRange, 50.0f, 500.0f, "%.0f");
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Misc"))
        {
            ImGui::BeginChild("##misc", ImVec2(0, 0), true);
            ImGui::Checkbox("Show Watermark", &m_menu.bWatermark);
            ImGui::Checkbox("Crosshair", &m_menu.bCrosshair);
            ImGui::Separator();
            ImGui::Combo("Crosshair Type", &m_menu.iCrosshairType, "Cross\0Circle\0Dot\0\0");
            ImGui::SliderFloat("Size", &m_menu.fCrosshairSize, 2.0f, 30.0f, "%.0f");
            ImGui::ColorEdit4("Color", (float*)&m_menu.colCrosshair, ImGuiColorEditFlags_NoInputs);
            ImGui::Separator();
            bool streaming = m_streamer.IsStreaming();
            if (ImGui::Checkbox("Streamer Mode", &streaming))
                m_streamer.Toggle();
            ImGui::Text("F6 to toggle | OBS/SLOBS detection");
            ImGui::Separator();
            if (ImGui::Button("Save Config", ImVec2(120, 25)))
                g_Config.Save("xpe_chams_config.ini");
            ImGui::SameLine();
            if (ImGui::Button("Load Config", ImVec2(120, 25)))
                g_Config.Load("xpe_chams_config.ini");
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "XPE CHAMS v2 | xpe.nettt");
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "XPE CHAMS | xpe.nettt");
    ImGui::SameLine(ImGui::GetWindowWidth() - 180);
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "INSERT: Menu | F6: Stream");
    ImGui::End();
}

// ============================================================
// Watermark
// ============================================================
void Overlay::RenderWatermark()
{
    if (m_streamer.IsStreaming() && m_streamer.ShouldHideWatermark())
        return;

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.6f);

    if (ImGui::Begin("##wm", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar))
    {
        ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "XPE CHAMS");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "v2 | xpe.nettt");
    }
    ImGui::End();
}

// ============================================================
// Crosshair
// ============================================================
void Overlay::RenderCrosshair()
{
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    ImVec2 ctr((float)GetSystemMetrics(SM_CXSCREEN) / 2.0f,
        (float)GetSystemMetrics(SM_CYSCREEN) / 2.0f);
    ImColor col = m_menu.colCrosshair;
    float sz = m_menu.fCrosshairSize;
    float gap = 3.0f;

    switch (m_menu.iCrosshairType)
    {
    case 0: // Cross
        dl->AddLine(ImVec2(ctr.x - sz, ctr.y), ImVec2(ctr.x - gap, ctr.y), col, 1.5f);
        dl->AddLine(ImVec2(ctr.x + gap, ctr.y), ImVec2(ctr.x + sz, ctr.y), col, 1.5f);
        dl->AddLine(ImVec2(ctr.x, ctr.y - sz), ImVec2(ctr.x, ctr.y - gap), col, 1.5f);
        dl->AddLine(ImVec2(ctr.x, ctr.y + gap), ImVec2(ctr.x, ctr.y + sz), col, 1.5f);
        break;
    case 1: // Circle
        dl->AddCircle(ctr, sz, col, 0, 1.5f);
        dl->AddCircleFilled(ctr, 2.0f, col);
        break;
    case 2: // Dot
        dl->AddCircleFilled(ctr, sz * 0.5f, col);
        break;
    }
}

// ============================================================
// Window procedure
// ============================================================
LRESULT CALLBACK Overlay::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_SIZE:
        if (g_pSwapChain && g_pDevice && wParam != SIZE_MINIMIZED)
        {
            if (g_pRenderTarget)
            {
                g_pContext->OMSetRenderTargets(0, 0, 0);
                g_pRenderTarget->Release();
                g_pRenderTarget = nullptr;
            }
        }
        return 0;
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

// ============================================================
// Shutdown
// ============================================================
void Overlay::Shutdown()
{
    if (m_bInitialized)
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        m_bInitialized = false;
    }
    if (g_pRenderTarget) { g_pRenderTarget->Release(); g_pRenderTarget = nullptr; }
    if (m_hOverlayWindow) { DestroyWindow(m_hOverlayWindow); m_hOverlayWindow = NULL; }
    UnregisterClassA("XPE_OVERLAY_CLASS", m_wc.hInstance);
}