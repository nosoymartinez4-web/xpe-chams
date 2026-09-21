/*
 * XPE CHAMS v2 - Streamer Mode
 */
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class StreamerMode
{
public:
    StreamerMode() {}
    void Initialize() {}
    void Shutdown() {}
};

extern StreamerMode g_StreamMode;