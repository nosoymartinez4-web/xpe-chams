/*
 * XPE CHAMS v2 - Overlay (D3D11 + ImGui)
 */
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class Overlay
{
public:
    Overlay() {}
    ~Overlay() {}

    void Initialize();
    void Shutdown();
};

extern Overlay g_Overlay;