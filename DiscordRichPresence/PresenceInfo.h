#pragma once

enum PlaybackState;

class PresenceInfo
{
    std::string m_details;
    std::string m_state;
    int64_t     m_startTimestamp;
    int64_t     m_endTimestamp;
    HANDLE      m_hPipe;
    int         m_nonce;

    bool ConnectPipe();
    void DisconnectPipe();
    bool SendFrame(uint32_t op, const std::string& payload);
    bool DoHandshake();
    void SendActivity();

public:
    PresenceInfo();
    ~PresenceInfo();

    PlaybackState CurrentPlaybackState;

    void InitializeDiscordRPC();
    void ShutdownDiscordRPC();

    bool HasDiscordModuleLoaded() const;

    void SetStateText(char const* str);
    void SetDetails(char const* str);
    void ClearDetails();
    void SetStartTimestamp(__int64 timestamp);
    void SetEndTimestamp(__int64 timestamp);

    void PostToDiscord();
};
