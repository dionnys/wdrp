// DiscordRichPresence.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "DiscordRichPresence.h"
#include "Resource.h"
#include "PresenceInfo.h"
#include "SettingsFile.h"
#include "Timer.h"

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void ReportIdleStatus();
void CALLBACK TimerProc(HWND, UINT, UINT_PTR, DWORD);

#define GPPHDR_VER 0x10
char PLUGIN_NAME[] = "Discord Rich Presence";
WNDPROC g_lpWndProcOld = 0;

typedef struct {
    int version;
    char *description;
    int(*init)();
    void(*config)();
    void(*quit)();
    HWND hwndParent;
    HINSTANCE hDllInstance;
} winampGeneralPurposePlugin;

int  init(void);
void config(void);
void quit(void);

winampGeneralPurposePlugin g_plugin = {
    GPPHDR_VER, PLUGIN_NAME, init, config, quit, 0, 0
};

Timer g_timer;
PresenceInfo g_presenceInfo;

// Obtiene now en segundos
static long long NowSeconds()
{
    std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();
}

void CALLBACK TimerProc(HWND, UINT, UINT_PTR, DWORD)
{
    assert(g_presenceInfo.CurrentPlaybackState == Playing);
    assert(g_pluginSettings.ShowElapsedTime);

    if (!g_presenceInfo.HasDiscordModuleLoaded()) return;

    int currentPosSec = SendMessage(g_plugin.hwndParent, WM_WA_IPC, 0, IPC_GETOUTPUTTIME) / 1000;
    int totalLength   = SendMessage(g_plugin.hwndParent, WM_WA_IPC, 1, IPC_GETOUTPUTTIME);

    long long now = NowSeconds();
    g_presenceInfo.SetStartTimestamp(now - currentPosSec);
    if (totalLength > 0)
        g_presenceInfo.SetEndTimestamp(now - currentPosSec + totalLength);
    else
        g_presenceInfo.SetEndTimestamp(0);

    g_presenceInfo.PostToDiscord();
}

int init()
{
    if (IsWindowUnicode(g_plugin.hwndParent))
        g_lpWndProcOld = (WNDPROC)SetWindowLongW(g_plugin.hwndParent, GWL_WNDPROC, (LONG)WndProc);
    else
        g_lpWndProcOld = (WNDPROC)SetWindowLongA(g_plugin.hwndParent, GWL_WNDPROC, (LONG)WndProc);

    g_timer.Initialize(g_plugin.hwndParent, TimerProc);
    LoadSettingsFile();
    g_presenceInfo.InitializeDiscordRPC();
    ReportIdleStatus();
    return 0;
}

void ReportIdleStatus()
{
    if (!g_presenceInfo.HasDiscordModuleLoaded()) return;
    if (g_pluginSettings.ApplicationID == "0") return;

    g_presenceInfo.CurrentPlaybackState = Stopped;
    g_presenceInfo.SetStartTimestamp(0);
    g_presenceInfo.SetEndTimestamp(0);
    g_presenceInfo.SetStateText("Idle");
    g_presenceInfo.ClearDetails();
    g_presenceInfo.PostToDiscord();
}

static std::string GetTrackMetadata(const wchar_t* filepath, const wchar_t* field)
{
    char filepathA[1024] = {};
    WideCharToMultiByte(CP_ACP, 0, filepath, -1, filepathA, 1024, nullptr, nullptr);

    char fieldA[64] = {};
    WideCharToMultiByte(CP_ACP, 0, field, -1, fieldA, 64, nullptr, nullptr);

    char buf[512] = {};
    extendedFileInfoStruct info = {};
    info.filename = filepathA;
    info.metadata = fieldA;
    info.ret      = buf;
    info.retlen   = 512;
    SendMessage(g_plugin.hwndParent, WM_WA_IPC, (WPARAM)&info, IPC_GET_EXTENDED_FILE_INFO);
    return std::string(buf);
}

void ReportCurrentSongStatus(PlaybackState playbackState)
{
    if (!g_presenceInfo.HasDiscordModuleLoaded()) return;
    if (g_pluginSettings.ApplicationID == "0") return;

    assert(playbackState != Stopped);
    g_presenceInfo.CurrentPlaybackState = playbackState;

    // Timestamps con barra de progreso
    int currentPosSec = SendMessage(g_plugin.hwndParent, WM_WA_IPC, 0, IPC_GETOUTPUTTIME) / 1000;
    int totalLength   = SendMessage(g_plugin.hwndParent, WM_WA_IPC, 1, IPC_GETOUTPUTTIME);

    if (playbackState == Playing && totalLength > 0)
    {
        long long now = NowSeconds();
        g_presenceInfo.SetStartTimestamp(now - currentPosSec);
        g_presenceInfo.SetEndTimestamp(now - currentPosSec + totalLength);
    }
    else
    {
        g_presenceInfo.SetStartTimestamp(0);
        g_presenceInfo.SetEndTimestamp(0);
    }

    // Metadatos
    int index = SendMessage(g_plugin.hwndParent, WM_WA_IPC, 0, IPC_GETLISTPOS);
    const wchar_t* filepath = (const wchar_t*)SendMessage(
        g_plugin.hwndParent, WM_WA_IPC, index, IPC_GETPLAYLISTFILEW);

    std::string artist;
    std::string title;

    if (filepath && filepath[0] != L'\0')
    {
        artist = GetTrackMetadata(filepath, L"artist");
        title  = GetTrackMetadata(filepath, L"title");
    }

    if (title.empty())
    {
        std::wstring fullTitle = (wchar_t*)SendMessage(
            g_plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_PLAYING_TITLE);
        std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
        title = conv.to_bytes(fullTitle);
    }

    std::string stateText = artist.empty() ? "Unknown Artist" : artist;
    if (playbackState == Paused) stateText += " (Paused)";
    g_presenceInfo.SetStateText(stateText.c_str());

    if (g_pluginSettings.DisplayTitleInStatus)
        g_presenceInfo.SetDetails(title.c_str());
    else
        g_presenceInfo.ClearDetails();

    g_presenceInfo.PostToDiscord();
}

void UpdateRichPresenceDetails()
{
    LONG isPlayingResult = SendMessage(g_plugin.hwndParent, WM_WA_IPC, 0, IPC_ISPLAYING);

    if (isPlayingResult == Playing)
    {
        if (g_pluginSettings.ShowElapsedTime) g_timer.Set();
        else g_timer.Stop();
        ReportCurrentSongStatus(Playing);
    }
    else if (isPlayingResult == Paused)
    {
        g_timer.Stop();
        ReportCurrentSongStatus(Paused);
    }
    else
    {
        g_timer.Stop();
        ReportIdleStatus();
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_WA_IPC && lParam == IPC_CB_MISC && wParam == IPC_CB_MISC_STATUS)
        UpdateRichPresenceDetails();
    return CallWindowProc(g_lpWndProcOld, hwnd, message, wParam, lParam);
}

void UpdateInMemorySettingsFromDialogState(HWND hWndDlg)
{
    HWND checkboxHwnd = GetDlgItem(hWndDlg, IDC_CHECK_DISPLAY_TITLE_IN_STATUS);
    g_pluginSettings.DisplayTitleInStatus = Button_GetCheck(checkboxHwnd) == BST_CHECKED;

    checkboxHwnd = GetDlgItem(hWndDlg, IDC_SHOW_ELAPSED_TIME);
    g_pluginSettings.ShowElapsedTime = Button_GetCheck(checkboxHwnd) == BST_CHECKED;

    HWND editboxHwnd = GetDlgItem(hWndDlg, IDC_EDIT_DISCORD_APPLICATION_ID);
    char stringData[255] = {};
    if (GetWindowTextA(editboxHwnd, stringData, _countof(stringData)) > 0)
        g_pluginSettings.ApplicationID = stringData;
}

void OnConfirmSettingsDialog(HWND hWndDlg)
{
    PluginSettings previousSettings = g_pluginSettings;
    UpdateInMemorySettingsFromDialogState(hWndDlg);

    bool applicationIDChanged       = previousSettings.ApplicationID != g_pluginSettings.ApplicationID;
    bool displayTitleSettingChanged  = previousSettings.DisplayTitleInStatus != g_pluginSettings.DisplayTitleInStatus;
    bool elapsedTimeChanged         = previousSettings.ShowElapsedTime != g_pluginSettings.ShowElapsedTime;

    if (!applicationIDChanged && !displayTitleSettingChanged && !elapsedTimeChanged) return;

    SaveSettingsFile();

    bool shouldUpdate = displayTitleSettingChanged || elapsedTimeChanged;

    if (applicationIDChanged)
    {
        g_presenceInfo.ShutdownDiscordRPC();
        g_presenceInfo.InitializeDiscordRPC();
        shouldUpdate = true;
    }

    if (shouldUpdate) UpdateRichPresenceDetails();
}

void PopulateSettingsDialogFields(HWND hWndDlg)
{
    HWND cb = GetDlgItem(hWndDlg, IDC_CHECK_DISPLAY_TITLE_IN_STATUS);
    Button_SetCheck(cb, g_pluginSettings.DisplayTitleInStatus ? BST_CHECKED : BST_UNCHECKED);

    cb = GetDlgItem(hWndDlg, IDC_SHOW_ELAPSED_TIME);
    Button_SetCheck(cb, g_pluginSettings.ShowElapsedTime ? BST_CHECKED : BST_UNCHECKED);

    HWND editboxHwnd = GetDlgItem(hWndDlg, IDC_EDIT_DISCORD_APPLICATION_ID);
    SetWindowTextA(editboxHwnd, g_pluginSettings.ApplicationID.c_str());
}

BOOL CALLBACK ConfigDialogProc(HWND hWndDlg, UINT wMessage, WPARAM wParam, LPARAM lParam)
{
    switch (wMessage)
    {
        case WM_INITDIALOG:
            if (GetWindowLong(hWndDlg, GWL_STYLE) & WS_CHILD) return FALSE;
            PopulateSettingsDialogFields(hWndDlg);
            return TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    OnConfirmSettingsDialog(hWndDlg);
                    EndDialog(hWndDlg, 0);
                    return TRUE;
                case IDCANCEL:
                    EndDialog(hWndDlg, 0);
                    return TRUE;
            }
        case BN_CLICKED:
            if (LOWORD(wParam) == IDC_CHECK_DISPLAY_TITLE_IN_STATUS)
            {
                HWND cb = GetDlgItem(hWndDlg, IDC_CHECK_DISPLAY_TITLE_IN_STATUS);
                bool checked = Button_GetCheck(cb) == BST_CHECKED;
                HWND elapsedCb = GetDlgItem(hWndDlg, IDC_SHOW_ELAPSED_TIME);
                if (checked) EnableWindow(elapsedCb, TRUE);
                else { Button_SetCheck(elapsedCb, BST_UNCHECKED); EnableWindow(elapsedCb, FALSE); }
            }
    }
    return FALSE;
}

void config()
{
    DialogBox(g_plugin.hDllInstance, (LPTSTR)IDD_DIALOG_CONFIG, g_plugin.hwndParent, &ConfigDialogProc);
}

void quit()
{
    g_presenceInfo.ShutdownDiscordRPC();
}

extern "C" __declspec(dllexport) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin()
{
    return &g_plugin;
}
