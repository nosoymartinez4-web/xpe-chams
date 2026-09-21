/*
 * XPE CHAMS v2 - KeyAuth implementation (Custom Server)
 * Connects to xpe-keyauth.vercel.app
 * Author: xpe.nettt / Stealth Proyects
 */

#include "keyauth.h"
#include <string>
#include <cstring>
#include <cstdio>
#include <wininet.h>

#pragma comment(lib, "wininet.lib")

KeyAuthClass g_KeyAuth;

KeyAuthClass::KeyAuthClass()
{
    m_initialized = false;
    m_loggedin = false;
    m_subscribed = false;
    m_statuscode = 0;
    m_licensekey[0] = 0;
    m_username[0] = 0;
    m_expiry[0] = 0;
    m_remaining[0] = 0;
}

bool KeyAuthClass::HttpRequest(const char* endpoint, const char* postdata, char* response, int responseSize)
{
    HINTERNET hInternet = InternetOpenA("XPE CHAMS/2.0",
        INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) return false;

    HINTERNET hConnect = InternetConnectA(hInternet, "xpe-keyauth.vercel.app",
        INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) { InternetCloseHandle(hInternet); return false; }

    char path[256];
    snprintf(path, sizeof(path), "/api/%s", endpoint);

    HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST",
        path, NULL, NULL, NULL,
        INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_CACHE_WRITE |
        INTERNET_FLAG_RELOAD, 0);
    if (!hRequest) { InternetCloseHandle(hConnect); InternetCloseHandle(hInternet); return false; }

    const char* headers = "Content-Type: application/x-www-form-urlencoded\r\n";

    if (!HttpSendRequestA(hRequest, headers, strlen(headers),
        (LPVOID)postdata, strlen(postdata)))
    {
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    DWORD bytesRead = 0;
    if (!InternetReadFile(hRequest, response, responseSize - 1, &bytesRead))
    {
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    response[bytesRead] = 0;

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    return true;
}

bool KeyAuthClass::Login(const char* licenseKey)
{
    if (!licenseKey || !licenseKey[0]) return false;

    strncpy(m_licensekey, licenseKey, sizeof(m_licensekey) - 1);
    m_licensekey[sizeof(m_licensekey) - 1] = 0;

    char postdata[512];
    snprintf(postdata, sizeof(postdata), "{\"key\":\"%s\",\"hwid\":\"UNKNOWN\"}", licenseKey);

    char response[4096] = {0};
    if (!HttpRequest("verify", postdata, response, sizeof(response)))
    {
        m_statuscode = -1;
        return false;
    }

    const char* successStr = strstr(response, "\"success\":");
    if (!successStr)
    {
        m_statuscode = -2;
        return false;
    }

    successStr += 10;
    while (*successStr == ' ' || *successStr == '\t') successStr++;

    if (strncmp(successStr, "true", 4) == 0)
    {
        m_loggedin = true;
        m_subscribed = true;
        m_statuscode = 1;

        const char* userStr = strstr(response, "\"username\":\"");
        if (userStr)
        {
            userStr += 11;
            int i = 0;
            while (*userStr && *userStr != '"' && i < (int)sizeof(m_username) - 1)
                m_username[i++] = *userStr++;
            m_username[i] = 0;
        }

        const char* expStr = strstr(response, "\"expiry\":\"");
        if (expStr)
        {
            expStr += 9;
            int i = 0;
            while (*expStr && *expStr != '"' && i < (int)sizeof(m_expiry) - 1)
                m_expiry[i++] = *expStr++;
            m_expiry[i] = 0;
        }

        const char* remStr = strstr(response, "\"remaining\":\"");
        if (remStr)
        {
            remStr += 11;
            int i = 0;
            while (*remStr && *remStr != '"' && i < (int)sizeof(m_remaining) - 1)
                m_remaining[i++] = *remStr++;
            m_remaining[i] = 0;
        }

        return true;
    }
    else
    {
        m_loggedin = false;
        m_subscribed = false;
        m_statuscode = 0;

        const char* msgStr = strstr(response, "\"message\":\"");
        if (msgStr)
        {
            msgStr += 10;
            int i = 0;
            while (*msgStr && *msgStr != '"' && i < (int)sizeof(m_username) - 1)
                m_username[i++] = *msgStr++;
            m_username[i] = 0;
        }

        return false;
    }
}

bool KeyAuthClass::CheckSubscription()
{
    if (!m_loggedin || !m_licensekey[0]) return false;

    char postdata[512];
    snprintf(postdata, sizeof(postdata), "{\"key\":\"%s\"}", m_licensekey);

    char response[4096] = {0};
    if (!HttpRequest("check", postdata, response, sizeof(response)))
        return m_subscribed;

    const char* successStr = strstr(response, "\"success\":");
    if (!successStr) return m_subscribed;

    successStr += 10;
    while (*successStr == ' ' || *successStr == '\t') successStr++;

    m_subscribed = (strncmp(successStr, "true", 4) == 0);
    return m_subscribed;
}

int KeyAuthClass::GetStatus() const { return m_statuscode; }
bool KeyAuthClass::IsLoggedIn() const { return m_loggedin; }
bool KeyAuthClass::IsSubscribed() const { return m_subscribed; }
const char* KeyAuthClass::GetUsername() const { return m_username; }
const char* KeyAuthClass::GetExpiry() const { return m_expiry; }
const char* KeyAuthClass::GetRemaining() const { return m_remaining; }