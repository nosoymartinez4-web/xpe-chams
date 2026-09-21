/*
 * XPE CHAMS v2 - KeyAuth Authentication (Custom Server)
 * Connects to xpe-keyauth.vercel.app
 * Author: xpe.nettt / Stealth Proyects
 */

#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class KeyAuthClass
{
private:
    char m_licensekey[256];
    char m_username[64];
    char m_expiry[64];
    char m_remaining[64];
    bool m_initialized;
    bool m_loggedin;
    bool m_subscribed;
    int  m_statuscode;

    bool HttpRequest(const char* endpoint, const char* postdata, char* response, int responseSize);

public:
    KeyAuthClass();

    bool Login(const char* licenseKey);
    bool CheckSubscription();

    int GetStatus() const;
    bool IsLoggedIn() const;
    bool IsSubscribed() const;
    const char* GetUsername() const;
    const char* GetExpiry() const;
    const char* GetRemaining() const;
};

extern KeyAuthClass g_KeyAuth;