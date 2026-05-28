#include "stdafx.h"
#include "PresenceInfo.h"
#include "DiscordRichPresence.h"
#include "SettingsFile.h"
#include <string>
#include <sstream>

static std::string EscapeJson(const std::string& s)
{
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s)
    {
        switch (c)
        {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b";  break;
        case '\f': out += "\\f";  break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:
            if (c < 0x20) { }
            else out += (char)c;
        }
    }
    return out;
}

PresenceInfo::PresenceInfo()
    : m_startTimestamp(0)
    , m_endTimestamp(0)
    , m_hPipe(INVALID_HANDLE_VALUE)
    , m_nonce(1)
    , CurrentPlaybackState(PlaybackState::Stopped)
{
}

PresenceInfo::~PresenceInfo()
{
    DisconnectPipe();
}

bool PresenceInfo::ConnectPipe()
{
    if (m_hPipe != INVALID_HANDLE_VALUE)
        return true;

    for (int i = 0; i < 10; i++)
    {
        std::wstring name = L"\\\\.\\pipe\\discord-ipc-" + std::to_wstring(i);
        HANDLE h = CreateFileW(name.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0, nullptr, OPEN_EXISTING, 0, nullptr);

        if (h != INVALID_HANDLE_VALUE)
        {
            m_hPipe = h;
            return true;
        }
    }
    return false;
}

void PresenceInfo::DisconnectPipe()
{
    if (m_hPipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hPipe);
        m_hPipe = INVALID_HANDLE_VALUE;
    }
}

bool PresenceInfo::SendFrame(uint32_t op, const std::string& payload)
{
    if (m_hPipe == INVALID_HANDLE_VALUE)
        return false;

    uint32_t len = (uint32_t)payload.size();
    DWORD written = 0;

    if (!WriteFile(m_hPipe, &op,  4, &written, nullptr)) { DisconnectPipe(); return false; }
    if (!WriteFile(m_hPipe, &len, 4, &written, nullptr)) { DisconnectPipe(); return false; }
    if (!WriteFile(m_hPipe, payload.c_str(), len, &written, nullptr)) { DisconnectPipe(); return false; }
    return true;
}

static std::string UrlEncode(const std::string& str)
{
    std::string encoded;
    encoded.reserve(str.size() * 2);

    for (unsigned char c : str)
    {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            encoded += c;
        }
        else if (c == ' ')
        {
            encoded += '+';
        }
        else
        {
            encoded += '%';
            encoded += "0123456789ABCDEF"[c >> 4];
            encoded += "0123456789ABCDEF"[c & 15];
        }
    }
    return encoded;
}

static bool ReadFrame(HANDLE hPipe, uint32_t& op, std::string& payload)
{
    uint32_t recvOp  = 0;
    uint32_t recvLen = 0;
    DWORD bytesRead  = 0;

    if (!ReadFile(hPipe, &recvOp,  4, &bytesRead, nullptr)) return false;
    if (!ReadFile(hPipe, &recvLen, 4, &bytesRead, nullptr)) return false;
    if (recvLen > 65536) return false;

    payload.resize(recvLen);
    if (recvLen > 0)
        if (!ReadFile(hPipe, &payload[0], recvLen, &bytesRead, nullptr)) return false;

    op = recvOp;
    return true;
}

bool PresenceInfo::DoHandshake()
{
    std::string hs = R"({"v":1,"client_id":")" +
        g_pluginSettings.ApplicationID + R"("})";

    if (!SendFrame(0, hs))
        return false;

    uint32_t op = 0;
    std::string resp;
    ReadFrame(m_hPipe, op, resp);
    return true;
}

void PresenceInfo::SendActivity()
{
    if (m_hPipe == INVALID_HANDLE_VALUE)
    {
        if (!ConnectPipe())  return;
        if (!DoHandshake())  return;
    }

    std::string activity;
    activity += R"("type":2)";

    if (!m_details.empty())
        activity += R"(,"details":")" + EscapeJson(m_details) + "\"";

    if (!m_state.empty())
        activity += R"(,"state":")" + EscapeJson(m_state) + "\"";

    // Timestamps: start + end
    if (m_startTimestamp > 0 || m_endTimestamp > 0)
    {
        activity += R"(,"timestamps":{)";
        if (m_startTimestamp > 0)
            activity += R"("start":)" + std::to_string(m_startTimestamp);
        if (m_endTimestamp > 0)
        {
            if (m_startTimestamp > 0) activity += ",";
            activity += R"("end":)" + std::to_string(m_endTimestamp);
        }
        activity += "}";
    }

    activity += R"(,"assets":{"large_image":"winamp-logo","large_text":"Winamp"})";

    // ==================== BOTÓN LAST.FM (ENLACE DIRECTO) ====================
    // Nota: Ajusta m_state y m_details según cuál sea el artista y cuál la canción en tu plugin.
    if (!m_state.empty() && !m_details.empty())
    {
        // Formato estándar de Last.fm: https://www.last.fm/music/Artista/_/Cancion
        std::string lastfm_url = "https://www.last.fm/music/" + UrlEncode(m_state) + "/_/" + UrlEncode(m_details);

        activity += R"(,"buttons":[)";
        activity += R"({"label":"Escucha en Last.fm","url":")" + EscapeJson(lastfm_url) + R"("})";
        activity += R"(])";
    }
    else if (!m_state.empty() || !m_details.empty()) // Si solo tenemos uno de los dos datos, hacemos fallback a búsqueda
    {
        std::string searchQuery = !m_details.empty() ? m_details : m_state;
        std::string lastfm_url = "https://www.last.fm/search?q=" + UrlEncode(searchQuery);

        activity += R"(,"buttons":[)";
        activity += R"({"label":"Buscar en Last.fm","url":")" + EscapeJson(lastfm_url) + R"("})";
        activity += R"(])";
    }
    // =======================================================================

    std::string payload =
        R"({"cmd":"SET_ACTIVITY","args":{"pid":)" +
        std::to_string((int)GetCurrentProcessId()) +
        R"(,"activity":{)" + activity + R"(}},"nonce":")" +
        std::to_string(m_nonce++) + R"("})";

    if (!SendFrame(1, payload))
        DisconnectPipe();
    else
    {
        uint32_t op = 0;
        std::string resp;
        ReadFrame(m_hPipe, op, resp);
    }
}


void PresenceInfo::SetStateText(char const* str)  { m_state   = str ? str : ""; }
void PresenceInfo::SetDetails(char const* str)    { m_details = str ? str : ""; }
void PresenceInfo::ClearDetails()                 { m_details.clear(); }
void PresenceInfo::SetStartTimestamp(__int64 t)   { m_startTimestamp = t; }
void PresenceInfo::SetEndTimestamp(__int64 t)     { m_endTimestamp   = t; }

void PresenceInfo::PostToDiscord()
{
    SendActivity();
}

bool PresenceInfo::HasDiscordModuleLoaded() const
{
    if (m_hPipe != INVALID_HANDLE_VALUE)
        return true;

    for (int i = 0; i < 10; i++)
    {
        std::wstring name = L"\\\\.\\pipe\\discord-ipc-" + std::to_wstring(i);
        HANDLE h = CreateFileW(name.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (h != INVALID_HANDLE_VALUE)
        {
            CloseHandle(h);
            return true;
        }
    }
    return false;
}

void PresenceInfo::InitializeDiscordRPC()
{
    if (g_pluginSettings.ApplicationID == "0")
        return;

    DisconnectPipe();
    if (!ConnectPipe()) return;
    DoHandshake();
}

void PresenceInfo::ShutdownDiscordRPC()
{
    DisconnectPipe();
}
