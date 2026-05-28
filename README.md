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

Example:

![Winamp is shown running, playing a song, side-by-side with a Discord Rich Presence status indicator showing that song's name and artist](https://raw.githubusercontent.com/clandrew/wdrp/master/Images/Example.png "Winamp is shown running, playing a song, side-by-side with a Discord Rich Presence status indicator showing that song's name and artist")

---

# Frequently Asked Questions (F.A.Q.'s)

**Q: Winamp, really? In {current year}?**

A: Yeah. Some people like it :P

**Q: How does it work?**

A: This is implemented as a Winamp plug-in that communicates directly with Discord via its local IPC named pipe, sending activity updates with `type: 2` (Listening). No external SDK is required.

**Q: How do I set it up?**

A: Here's step-by-step instructions:

1. Log in to the Discord web app.

Visit [discord.com/developers/applications](https://discord.com/developers/applications).

You'll see something like:

![A view of the Applications tab of the Discord Developer Portal, with a button that says Create an application](https://raw.githubusercontent.com/clandrew/wdrp/master/Images/Setup00.png "A view of the Applications tab of the Discord Developer Portal, with a button that says Create an application")

Click "**New Application**". Name it **Winamp** (capital W).

![An image of a dialog box titled Create an application. The user is typing in the text, Winamp. There are buttons labeled Create and Cancel](https://raw.githubusercontent.com/clandrew/wdrp/master/Images/Setup01.png "An image of a dialog box titled Create an application. The user is typing in the text, Winamp. There are buttons labeled Create and Cancel")

Afterward, you'll be taken to a screen to configure the application. You can set an icon, if you want. You can get the classic Winamp logo from [here](https://commons.wikimedia.org/wiki/File:Winamp-logo.png).

If you want to include a logo in the Rich Presence, upload an asset to **Rich Presence → Art Assets** and name it exactly `winamp-logo`. It will show up to the left of the rich presence text.

![A view of the Applications tab of the Discord Developer Portal; viewing the General Information tab for the Winamp application. The screenshot shows the Winamp Application's name as Winamp, and a CLIENT ID. An example ID of 112233445566778899 is shown.](https://raw.githubusercontent.com/clandrew/wdrp/master/Images/Setup02.png "A view of the Applications tab of the Discord Developer Portal; viewing the General Information tab for the Winamp application.")

The only thing to do here is to take note of the **Client ID**, also called an **Application ID**, since you'll need it later.

2. Download `gen_DiscordRichPresence.dll` from the latest release on GitHub [here](https://github.com/dionnys/wdrp/releases/).

3. To install the Winamp plugin, copy `gen_DiscordRichPresence.dll` to the Plugins folder of your Winamp installation.

Most often, the Plugins folder is located at **C:\Program Files (x86)\Winamp\Plugins**.

![Two Windows file folders are shown; one for the Winamp Plugins install folder and one for the unzipped plugin release. Arrows are showing that the plugin release files are copied into the Winamp Plugins folder.](https://raw.githubusercontent.com/clandrew/wdrp/master/Images/Setup04.PNG "Two Windows file folders are shown.")

When you're done, there should be a file **gen_DiscordRichPresence.dll** in the Plugins folder.

> ⚠️ The `gen_` prefix is required — Winamp only loads General Purpose plugins with this prefix.

4. Now open Winamp (or restart it if it was already open), and go to **Preferences**.

![Winamp is shown running, where the user has clicked the menu button, moused-over the Options menu, and is about to click Preferences.](https://raw.githubusercontent.com/clandrew/wdrp/master/Images/Setup05.png "Winamp Preferences menu")

Under the Plug-ins tab, under General Purpose, the "Discord Rich Presence" item should appear. If it does, the plug-in was successfully installed!

![The Winamp Preferences window is shown with Discord Rich Presence plugin selected.](https://raw.githubusercontent.com/clandrew/wdrp/master/Images/Setup06.PNG "Winamp Preferences - Discord Rich Presence plugin")

Click the button **Configure selected plug-in.**

![The Discord Rich Presence configuration dialog with Application ID field.](https://raw.githubusercontent.com/clandrew/wdrp/master/Images/Setup07.PNG "Discord Rich Presence configuration dialog")

On this menu, paste the Discord **Application ID** from before. Also, you can check or un-check the box depending on whether you are comfortable showing the currently-playing media on Discord.

Alternatively, you can close Winamp and edit the settings file directly:
```
C:\Program Files (x86)\Winamp\Plugins\DiscordRichPresence\settings.ini
```
Make sure it contains your Application ID:
```ini
ApplicationID:YOUR_ID_HERE
DisplayTitle:1
ShowElapsedTime:1
```

Click OK, or save settings.ini, and you're done!

---

**Q: If I want to use the plugin, do I need to build from source?**

A: Nope — you can go to the [Releases](https://github.com/dionnys/wdrp/releases) page and download the binaries.

**Q: Do I need to give the application my Discord credentials?**

A: No, the plugin doesn't ask for your credentials.

**Q: How come the status now says "Listening to" instead of "Playing a game"?**

A: This fork bypasses the legacy Discord RPC SDK entirely and communicates with Discord directly via its local IPC named pipe (`\\.\pipe\discord-ipc-N`), sending `activity.type = 2` (Listening) in the payload. The original RPC SDK hardcoded type 0 (Playing) with no way to change it.

**Q: Does this require the discord-rpc.dll?**

A: No. This fork removed that dependency completely. Only `gen_DiscordRichPresence.dll` is needed.

**Q: Why does it show "Unknown Artist" for some tracks?**

A: The plugin reads metadata via Winamp's `IPC_GET_EXTENDED_FILE_INFO`. If your files don't have ID3 tags it will fall back to the formatted title string. Tag your files with a tool like [Mp3tag](https://www.mp3tag.de/en/) for best results.

**Q: Sometimes, I'm seeing a small delay before my Discord Rich Presence status is updated. What gives?**

A: Discord throttles the update frequency of the status text on its side. The frequency is every 15 seconds. [See this page for more information](https://discordapp.com/developers/docs/rich-presence/how-to#updating-presence).

**Q: Is there a way to get it to show album art?**

A: Currently, probably not. Discord doesn't allow programmatic upload of images — all images need to be uploaded through the Developer Portal web site.

**Q: I can't seem to get the plug-in to start.**

A: Close Winamp and verify that `settings.ini` contains your Application ID (not all zeros):
```
C:\Program Files (x86)\Winamp\Plugins\DiscordRichPresence\settings.ini
```
You may need to edit it manually if Winamp was run as a limited-privilege user.

**Q: I'm seeing problems when I re-name the Discord application then try to use it with Rich Presence.**

A: This appears to be a bug in Discord. The way to work around it is to delete and re-create a new application with the correct name.

**Q: If I want to build the plugin, what type of environment do I use?**

A: Visual Studio 2019+ on Windows 10/11. See the build instructions below.

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

> ℹ️ This fork does **not** require the `discord-rpc` SDK.

### Build steps

1. Clone this repo
2. Open `DiscordRichPresence.sln` in Visual Studio
3. Set configuration to **Release | x86**
4. Add `C:\Program Files (x86)\Winamp SDK` to **C/C++ → Additional Include Directories**
5. Build (`Ctrl+Shift+B`)
6. Copy `Release\DiscordRichPresence.dll` to Winamp Plugins as `gen_DiscordRichPresence.dll`

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
