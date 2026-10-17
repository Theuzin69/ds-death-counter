<p align="center">
  <img src="docs/images/icon.png" width="96" alt="DS Death Counter icon">
</p>

<h1 align="center">DS Death Counter</h1>

<p align="center">
  Automatic death counter with an on-screen overlay for <b>Dark Souls Remastered</b> and <b>Dark Souls II: Scholar of the First Sin</b>.<br>
  For streamers, and for anyone who wants to know how many times (and where) they died.
</p>

<p align="center">
  <a href="https://github.com/Theuzin69/ds-death-counter/releases/latest"><img src="https://img.shields.io/github/v/release/Theuzin69/ds-death-counter?label=download&color=e8762c" alt="Latest release"></a>
  <a href="https://github.com/Theuzin69/ds-death-counter/releases"><img src="https://img.shields.io/github/downloads/Theuzin69/ds-death-counter/total?color=8a5a2b" alt="Downloads"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-blue" alt="MIT license"></a>
  <a href="docs/how-it-was-made.md"><img src="https://img.shields.io/badge/built%20with-AI%20(Claude)-d97757" alt="Built with AI"></a>
  <img src="https://img.shields.io/badge/platform-Windows%2010%20%7C%2011%20(x64)-0078d6" alt="Windows 10/11 x64">
</p>

<p align="center">
  <b>English</b> · <a href="README.pt-BR.md">Português (Brasil)</a>
</p>

<p align="center">
  <img src="docs/images/app-main.png" width="820" alt="DS Death Counter main window">
</p>

> [!IMPORTANT]
> **This app was made with AI.** The code, the memory research and this documentation were written by **Claude** (Anthropic's *Claude Opus 5.5* model, through *Claude Code*), directed and tested in-game by **MatheusMuzzi**.
> The full story, including what was verified and how, is in **[How it was made](docs/how-it-was-made.md)**.

## Features

- **Counts deaths by itself.** It reads the death counter the game already keeps for each character, so there is nothing to click while you play.
- **One count per save/character**, each with its own history.
- **Never loses the count.** Close the game and counting stops; open it again and it continues where it left off. Deaths that happen while the counter is closed are still added.
- **Overlay on top of the game** with your choice of font, size, corner, colors and outline. `Ctrl+Shift+D` shows/hides it.
- **List of every death**: date/time, area (map), last bonfire, cause and play time. Statistics by area and by cause, plus CSV export.
- **Falls are detected automatically.** For other causes, pick from suggestions (bosses, enemies, traps…) with one click.
- **Editable count.** An edited count is clearly flagged in the app and the overlay number changes color.
- **Runs quietly in the system tray** and can start with Windows: open the game and it is already counting.
- **Read-only.** It never writes to the game, injects nothing and doesn't touch your saves (see [Is it safe?](#is-it-safe-can-i-get-banned)).
- Single ~1 MB executable, no runtimes to install.

> [!NOTE]
> The app's interface is currently in **Brazilian Portuguese**. An English UI is planned; contributions are welcome.

## Download

Go to **[Releases](https://github.com/Theuzin69/ds-death-counter/releases/latest)** and download one of:

| File | What it is |
|---|---|
| `DSDeathCounter-Setup-x.y.z.exe` | **Installer (recommended).** Installs for your user only (no admin rights), adds a Start menu shortcut, can start with Windows, and has an uninstaller. |
| `DSDeathCounter-x.y.z-portable.zip` | **Portable.** Unzip anywhere and run `DSDeathCounter.exe`. |
| `SHA256SUMS.txt` | Checksums to verify the downloads. |
| Source code (zip / tar.gz) | Added automatically by GitHub. |

<a id="smartscreen"></a>

## ⚠️ Windows will warn you the first time (this is expected)

DS Death Counter is **not code-signed**. A code-signing certificate costs hundreds of dollars a year, which doesn't make sense for a free hobby project. Because of that, the first time you open the installer (or the exe), Windows will most likely show:

> **Windows protected your PC**
> Microsoft Defender SmartScreen prevented an unrecognized app from starting. Running this app might put your PC at risk.
> *Publisher: Unknown publisher*

To continue, click **More info** and then **Run anyway**.

<details>
<summary><b>Why does this happen, and how can I check the file is legit?</b></summary>

SmartScreen warns about **any** new file that isn't signed and hasn't been downloaded by many people yet. It is not a virus detection. Your browser may show a similar warning ("isn't commonly downloaded"). In Edge choose **⋯ > Keep**, in Chrome **Keep**.

Ways to make sure you have the real file:

1. Download **only** from this repository's [Releases](https://github.com/Theuzin69/ds-death-counter/releases) page.
2. Release files are **built automatically by GitHub Actions** from the public source code. The [build logs](https://github.com/Theuzin69/ds-death-counter/actions) are public.
3. Compare the checksum with `SHA256SUMS.txt` from the release:
   ```powershell
   Get-FileHash .\DSDeathCounter-Setup-1.0.0.exe -Algorithm SHA256
   ```
4. Verify the build provenance (proves the file came from this repository's workflow) with the [GitHub CLI](https://cli.github.com/):
   ```powershell
   gh attestation verify .\DSDeathCounter-Setup-1.0.0.exe --repo Theuzin69/ds-death-counter
   ```
5. Optionally, upload it to [VirusTotal](https://www.virustotal.com/).

Some antivirus programs may also flag it heuristically, because it reads another program's memory (that is how it counts deaths). It only ever **reads**.
</details>

## Quick start

1. Install (or unzip) and open **DS Death Counter**. Expect the [SmartScreen warning](#smartscreen) above.
2. In the welcome screen, let it find the game (or point to `DarkSoulsRemastered.exe`) and keep **Start with Windows** checked. Dark Souls II needs no setup: it is recognized whenever `DarkSoulsII.exe` is running.
3. Set the game to **Windowed** (or borderless) mode. The overlay can't always draw over exclusive fullscreen. There's a button in *Configurações › Janela do jogo* that makes the game window borderless, so it looks just like fullscreen.
4. Load a character and play. The count shows up in the corner of the game, and every death goes to the list.

The **[User guide](docs/user-guide.md)** covers every screen and option, OBS/streaming tips, backups and troubleshooting.

## Is it safe? Can I get banned?

The counter was designed to stay away from anything that could get you flagged:

- It **only reads** the game's memory (`ReadProcessMemory`), the same technique used for years by LiveSplit autosplitters and DSDeaths. The game process is opened with read access only.
- It **never writes** to the game, **injects no DLL**, installs **no hooks** and doesn't modify game files or saves.
- The overlay is a **separate window** on top of the game; nothing is drawn inside the game.
- Editing your count only changes the number stored by this app. The game's own counter is never touched.
- Dark Souls bans (the "soft ban") come from impossible character data being sent to the servers, which this app doesn't touch.

No third-party tool can honestly promise zero risk, but read-only memory access is as safe as it gets.

## Compatibility

| | |
|---|---|
| Windows | 10 or 11, 64-bit |
| Dark Souls Remastered | Supports the original 1.01 release and the 1.03.x builds (current Steam version). The game version is detected automatically. |
| Dark Souls II | **Scholar of the First Sin** (64-bit), new in the next version. Checked against the 1.02 executable; the current Steam build (1.03) is found by the same byte signatures but has not been confirmed in-game yet. The original 32-bit Dark Souls II (2014) is **not** supported: the app says so instead of counting. |
| Display mode | Windowed / borderless windowed. Exclusive fullscreen may hide the overlay. |
| Interface language | Brazilian Portuguese (English planned) |

If something doesn't look right, open the **Diagnóstico** tab, click **Copiar** and paste it into a [bug report](https://github.com/Theuzin69/ds-death-counter/issues/new/choose).

## Documentation

- [User guide](docs/user-guide.md) · [Guia do usuário (PT-BR)](docs/guia-do-usuario.md)
- [How it was made (AI disclosure)](docs/how-it-was-made.md) · [Como foi feito (PT-BR)](docs/como-foi-feito.md)
- [Changelog](CHANGELOG.md)
- [Contributing / building from source](CONTRIBUTING.md)

## Building from source

Requires Visual Studio 2019/2022 with the *Desktop development with C++* workload (or the Build Tools).

```bat
build.bat
```

This produces `build\DSDeathCounter.exe`, statically linked with no extra DLLs. See [CONTRIBUTING.md](CONTRIBUTING.md) for tests, the installer and how releases are published.

## Credits

- Idea, requirements and in-game testing: **MatheusMuzzi**
- Code, reverse-engineering and docs: **Claude** (Anthropic), via Claude Code. See [How it was made](docs/how-it-was-made.md).
- Memory offsets and signatures build on public community research: [DSDeaths](https://github.com/quidrex/DSDeaths), [SoulSplitter](https://github.com/FrankvdStam/SoulSplitter), [DSR-Gadget](https://github.com/JKAnderson/DSR-Gadget), [DS-Gadget](https://github.com/JKAnderson/DS-Gadget) and [DS2S-META](https://github.com/Nordgaren/DS2S-META).
- Libraries: [Dear ImGui](https://github.com/ocornut/imgui) and [nlohmann/json](https://github.com/nlohmann/json), both MIT. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## License

[MIT](LICENSE) © 2026 MatheusMuzzi

*DARK SOULS is a trademark of BANDAI NAMCO Entertainment Inc. / FromSoftware, Inc. DS Death Counter is a fan-made tool and is not affiliated with, endorsed by or sponsored by them.*
