/*
 * XPE CHAMS v2 - Streamer Mode
 * Detects OBS/SLOBS/Discord capture and hides overlay
 * Author: xpe.nettt
 */

#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <psapi.h>
#include <algorithm>

#pragma comment(lib, "psapi.lib")

// ============================================================
// StreamerMode class
// Detects screen capture applications and hides sensitive info
// ============================================================
class StreamerMode
{
private:
    bool m_bStreaming;
    bool m_bHideOverlay;
    bool m_bHideWatermark;
    bool m_bHideMenu;
    std::vector<std::string> m_captureApps;
    std::vector<DWORD> m_detectedPIDs;

    // Window class names of known capture apps
    const char* m_captureClasses[32];
    int m_nCaptureClasses;

    // Check if a specific process is a capture app
    bool IsCaptureApp(DWORD pid)
    {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (!hProcess) return false;

        char exeName[MAX_PATH];
        DWORD size = sizeof(exeName);
        if (QueryFullProcessImageNameA(hProcess, 0, exeName, &size))
        {
            // Extract filename
            char* fileName = strrchr(exeName, '\\');
            if (fileName) fileName++;
            else fileName = exeName;

            // Convert to lowercase for comparison
            std::string lowerName = fileName;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

            // Check against known capture apps
            for (const auto& app : m_captureApps)
            {
                if (lowerName.find(app) != std::string::npos)
                {
                    CloseHandle(hProcess);
                    return true;
                }
            }
        }

        CloseHandle(hProcess);
        return false;
    }

    // Check for capture app windows
    BOOL CALLBACK EnumCaptureWindows(HWND hwnd, LPARAM lParam)
    {
        char className[256];
        GetClassNameA(hwnd, className, sizeof(className));

        for (int i = 0; i < m_nCaptureClasses; i++)
        {
            if (m_captureClasses[i] && strstr(className, m_captureClasses[i]))
            {
                DWORD pid;
                GetWindowThreadProcessId(hwnd, &pid);
                if (IsCaptureApp(pid))
                {
                    // Found a capture app
                    if (std::find(m_detectedPIDs.begin(), m_detectedPIDs.end(), pid) == m_detectedPIDs.end())
                    {
                        m_detectedPIDs.push_back(pid);
                    }
                }
            }
        }

        return TRUE;
    }

public:
    StreamerMode()
    {
        m_bStreaming = false;
        m_bHideOverlay = false;
        m_bHideWatermark = true;
        m_bHideMenu = false;
        m_nCaptureClasses = 0;

        // Initialize capture app names (lowercase)
        m_captureApps.push_back("obs64");
        m_captureApps.push_back("obs32");
        m_captureApps.push_back("obs");
        m_captureApps.push_back("slobs");
        m_captureApps.push_back("slobs64");
        m_captureApps.push_back("streamlabs");
        m_captureApps.push_back("discord");
        m_captureApps.push_back("xsplit");
        m_captureApps.push_back("xsplit.core");
        m_captureApps.push_back("twitchstudio");
        m_captureApps.push_back("bandicam");
        m_captureApps.push_back("action");
        m_captureApps.push_back("mirillis");
        m_captureApps.push_back("nvidia");
        m_captureApps.push_back("shadowplay");
        m_captureApps.push_back("geforce");
        m_captureApps.push_back("recentral");
        m_captureApps.push_back("raze");
        m_captureApps.push_back("overwolf");
        m_captureApps.push_back("d3dgear");
        m_captureApps.push_back("fraps");

        // Window class names for capture detection
        m_captureClasses[m_nCaptureClasses++] = "OBSWindowClass";
        m_captureClasses[m_nCaptureClasses++] = "Qt5QWindowIcon";
        m_captureClasses[m_nCaptureClasses++] = "Chrome_WidgetWin_0";
        m_captureClasses[m_nCaptureClasses++] = "Windows.UI.Core.CoreWindow";
        m_captureClasses[m_nCaptureClasses++] = "ApplicationFrameWindow";
        m_captureClasses[m_nCaptureClasses++] = "XSplit";
        m_captureClasses[m_nCaptureClasses++] = "Bandicam";
    }

    // Scan for capture apps
    bool Scan()
    {
        m_detectedPIDs.clear();
        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
            return ((StreamerMode*)lParam)->EnumCaptureWindows(hwnd, lParam);
            }, (LPARAM)this);

        m_bStreaming = !m_detectedPIDs.empty();
        return m_bStreaming;
    }

    // Toggle streamer mode
    void Toggle()
    {
        m_bStreaming = !m_bStreaming;
        if (m_bStreaming)
        {
            m_bHideWatermark = true;
            m_bHideOverlay = true;
        }
        else
        {
            m_bHideWatermark = false;
            m_bHideOverlay = false;
        }
    }

    // Enable/disable
    void SetEnabled(bool enabled)
    {
        m_bStreaming = enabled;
        m_bHideWatermark = enabled;
        m_bHideOverlay = enabled;
    }

    // Getters
    bool IsStreaming() const { return m_bStreaming; }
    bool ShouldHideOverlay() const { return m_bHideOverlay; }
    bool ShouldHideWatermark() const { return m_bHideWatermark; }
    bool ShouldHideMenu() const { return m_bHideMenu; }

    // Get detected apps
    const std::vector<DWORD>& GetDetectedPIDs() const { return m_detectedPIDs; }
};