# Discord Rich Presence Winamp plugin

Do you use Winamp to play media, and Discord for social purposes? This simple, easy-to-use plugin integrates Winamp into Discord showing your currently playing track with full **"Listening to"** status — just like Spotify.

## Features

- ✅ Shows as **"Listening to Winamp"** (not "Playing a game")
- ✅ Displays **song title** and **artist** separately
- ✅ **Progress bar** with elapsed and total time
- ✅ Settings menu for ease of use — no restart required
- ✅ Option to hide currently-playing title
- ✅ Option to show elapsed time
- ✅ Direct Discord IPC — no external SDK dependency

Tested on:
- Winamp version 5.623 (x86)
- Winamp version 5.8 Build 3660 (x86)

This plugin is for Windows 7/8/10/11 x86-compatible environments.

---

## How it looks

```
Listening to Winamp
┌──────────────────────────────┐
│  Dreadlock Holiday           │  ← song title
│  10 CC                       │  ← artist
│  Winamp                      │
│  00:14 ──────────── 05:01    │  ← progress bar
└──────────────────────────────┘
```

---

## Setup

### 1. Create a Discord Application

Log in to the [Discord Developer Portal](https://discord.com/developers/applications) and click **New Application**. Name it **Winamp** (capital W).

Take note of the **Application ID** (also called Client ID) — you'll need it later.

#### Optional: Add logo to Rich Presence

Upload an image to **Rich Presence → Art Assets** and name it exactly `winamp-logo`. It will appear to the left of the presence text. You can get the classic Winamp logo from [Wikimedia Commons](https://commons.wikimedia.org/wiki/File:Winamp-logo.png).

### 2. Install the plugin

Copy the following files to your Winamp Plugins folder (usually `C:\Program Files (x86)\Winamp\Plugins\`):

```
gen_DiscordRichPresence.dll   → Plugins\gen_DiscordRichPresence.dll
```

> ⚠️ The `gen_` prefix is required — Winamp only loads General Purpose plugins with this prefix.

### 3. Configure

Open Winamp → **Options → Preferences → Plug-ins → General Purpose** → select **Discord Rich Presence** → click **Configure**.

Paste your **Application ID** from step 1 and click OK.

Alternatively, edit the settings file directly:
```
C:\Program Files (x86)\Winamp\Plugins\DiscordRichPresence\settings.ini
```

Make sure it contains your Application ID:
```ini
ApplicationID:YOUR_ID_HERE
DisplayTitle:1
ShowElapsedTime:1
```

### 4. Done!

Open Winamp and play a song. Your Discord status will update automatically.

---

## Building from source

### Requirements

| Dependency | Where to get it |
|---|---|
| Visual Studio 2019+ | [visualstudio.microsoft.com](https://visualstudio.microsoft.com/downloads/) |
| MSVC v141 or v142 toolset | Via VS Installer |
| Windows 10 SDK (10.0.17134.0+) | Via VS Installer |
| Winamp SDK (`wa_ipc.h`) | [getwacup.com/sdk/Winamp/wa_ipc.h](https://getwacup.com/sdk/Winamp/wa_ipc.h) |

Place `wa_ipc.h` at:
```
C:\Program Files (x86)\Winamp SDK\Winamp\wa_ipc.h
```

> ℹ️ This fork no longer requires the `discord-rpc` SDK — Discord IPC is handled directly via named pipe.

### Build steps

1. Clone this repo
2. Open `DiscordRichPresence.sln` in Visual Studio
3. Set configuration to **Release | x86**
4. Add `C:\Program Files (x86)\Winamp SDK` to **C/C++ → Additional Include Directories**
5. Build (`Ctrl+Shift+B`)
6. Output: `Release\DiscordRichPresence.dll` → copy to Winamp Plugins as `gen_DiscordRichPresence.dll`

---

## FAQ

**Q: How does it show "Listening to" instead of "Playing a game"?**

A: This fork bypasses the legacy Discord RPC SDK entirely and communicates with Discord directly via its local IPC named pipe (`\\.\pipe\discord-ipc-N`), sending `activity.type = 2` (Listening) in the payload. The original RPC SDK hardcoded type 0 (Playing) with no way to change it.

**Q: Does this require the discord-rpc.dll?**

A: No. This fork removed that dependency completely. Only `gen_DiscordRichPresence.dll` is needed.

**Q: Why does it show "Unknown Artist" for some tracks?**

A: The plugin reads metadata via Winamp's `IPC_GET_EXTENDED_FILE_INFO`. If your files don't have ID3 tags, it falls back to the formatted title string. Tag your files with a tool like [Mp3tag](https://www.mp3tag.de/en/) for best results.

**Q: Is there a delay in status updates?**

A: Discord throttles presence updates to once every 15 seconds on their side.

**Q: Can it show album art?**

A: Not currently. Discord requires images to be pre-uploaded to the Developer Portal. Dynamic image upload via the API is not supported.

**Q: I renamed the Discord application and now it doesn't work.**

A: This is a known Discord bug. Delete the application and create a new one with the correct name.

---

## Changes from upstream (clandrew/wdrp)

| Feature | Upstream | This fork |
|---|---|---|
| Activity type | Playing (0) | **Listening (2)** ✅ |
| Artist metadata | ❌ | ✅ |
| Progress bar | ❌ | ✅ |
| discord-rpc.dll dependency | Required | **Removed** ✅ |
| Discord IPC | Via SDK | **Direct pipe** ✅ |

---

*Forked from [clandrew/wdrp](https://github.com/clandrew/wdrp). Original MIT license applies.*
