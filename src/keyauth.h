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

    // Login with a license key against xpe-keyauth.vercel.app
    bool Login(const char* licenseKey);

    // Check if subscription is still valid
    bool CheckSubscription();

    // Getters    int  GetStatus() const;
    const char* GetRemaining() const;
    bool IsLoggedIn() const;
    bool IsSubscribed() const;
    const char* GetUsername() const;
    const char* GetExpiry() const;
};

extern KeyAuthClass g_KeyAuth;