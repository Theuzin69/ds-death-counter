# User guide

**English** · [Português (Brasil)](guia-do-usuario.md)

The app's interface is in Brazilian Portuguese. This guide quotes the on-screen labels in *italics* and explains them in English.

- [1. Installing](#1-installing)
- [2. First launch](#2-first-launch)
- [3. While you play](#3-while-you-play)
- [4. Characters and counts](#4-characters-and-counts)
- [5. The death list](#5-the-death-list)
- [Sekiro: Shadows Die Twice](#sekiro)
- [6. Settings](#6-settings)
- [7. Streaming (OBS)](#7-streaming-obs)
- [8. Your data](#8-your-data)
- [9. Troubleshooting](#9-troubleshooting)
- [10. Uninstalling](#10-uninstalling)

---

## 1. Installing

Download from the [Releases page](https://github.com/Theuzin69/ds-death-counter/releases/latest):

- **Installer** (`DSDeathCounter-Setup-x.y.z.exe`, recommended): installs to `%LOCALAPPDATA%\Programs\DSDeathCounter` for your Windows user only (no administrator rights), creates a Start menu shortcut and optionally a desktop shortcut, and offers *Iniciar com o Windows* (start with Windows).
- **Portable** (`DSDeathCounter-x.y.z-portable.zip`): unzip anywhere and run `DSDeathCounter.exe`.

### The Windows SmartScreen warning

The app isn't code-signed, so the first time you run it Windows shows **"Windows protected your PC"**. Click **More info → Run anyway**. Your browser may also say the file "isn't commonly downloaded"; choose **Keep**. This is expected for small unsigned projects and isn't a virus detection. The [README](../README.md#smartscreen) explains how to verify that your download is the official build (checksums and build attestations).

## 2. First launch

A welcome screen (*Bem-vindo ao DS Death Counter*) asks for two things:

1. **Link to the game** (*Vincular ao jogo*). The app looks for Dark Souls Remastered in your Steam libraries. If it doesn't find it, click *Procurar DarkSoulsRemastered.exe…* and select the game's exe. This path is only used by the *Iniciar Dark Souls* (launch game) button: the counter recognizes the running game by itself either way. Dark Souls II needs no linking (see [Dark Souls II](#dark-souls-ii-scholar-of-the-first-sin)).
2. **Always on** (*Sempre ligado*). Keep *Iniciar com o Windows, minimizado na bandeja* checked. The app then starts with Windows in the system tray, so your deaths are counted whenever you open the game, without you having to remember to open the counter.

Click *Começar* (start).

## 3. While you play

### Status line

At the top of the window:

| Status | Meaning |
|---|---|
| *Aguardando o jogo abrir…* | No supported game is running. *Iniciar Dark Souls* launches the game set in *Configurações*. |
| *(game) encontrado, preparando a leitura…* | The game was found and the counter is locating its data (takes a moment). |
| *Versão do jogo não suportada (…)* | A 32-bit version of the game is running, e.g. the original Dark Souls II. Only 64-bit versions are read. |
| *Aguardando o Dark Souls Remastered ou o Sekiro abrir…* | No supported game is running. *Iniciar Dark Souls* launches the game set in *Configurações*. |
| *Dark Souls Remastered encontrado, preparando a leitura…* (or *Sekiro encontrado…*) | The game was found and the counter is locating its data (takes a moment). |
| *Conectado ao jogo (versão X). Carregue um personagem.* | Connected; you're on the title screen. |
| *Contando as mortes de NAME (DSR, slot N)* | Counting (*DSR* or *Sekiro*: the game this character belongs to). |
| *O jogo está rodando como administrador…* | Windows blocked access. See [Troubleshooting](#9-troubleshooting). |

### The overlay

The current character's count appears in a corner of the game window (default: top-left, `Mortes: 123`). It follows the game window when you move or resize it.

- **Display mode:** the overlay is a separate transparent window, so it always works in **Windowed** or **borderless windowed** mode. In exclusive **fullscreen** it may not appear, or may flicker, depending on Windows and your graphics driver.
- **Fullscreen look without exclusive fullscreen:** set the game to Windowed at your monitor's resolution, then use *Configurações › Janela do jogo › Deixar a janela do jogo sem bordas*. The app only removes the window frame; nothing inside the game is changed.
- **Show/hide:** `Ctrl+Shift+D`, or the tray icon menu.
- By default the overlay is hidden when you alt-tab away from the game and on the game's title screen.
- **Edited counts** are drawn in a different color (orange by default).

### Tray icon

Closing the window keeps the counter running in the system tray. Hover the tray icon to see the current count. Right-click it for *Abrir* (open), *Mostrar overlay* (toggle overlay) and *Sair* (quit).

### Bloodborne (PC port, experimental)

The counter also works with the **Bloodborne PC Windows port** (v0.5, process `bbport.exe`) running Bloodborne 1.09. Start the game the way the port tells you to; the counter finds `bbport.exe` by itself, exactly like it finds Dark Souls Remastered. (The *Iniciar Dark Souls* button only launches Dark Souls Remastered.)

- Status line: *Bloodborne encontrado, preparando a leitura…*, then *Conectado ao Bloodborne (port de PC, bbport.exe). Carregue um personagem.*, then *Contando as mortes de NAME (Bloodborne)*.
- What's read: the game's own death counter, the character's name, level, NG+ cycle and play time. Each death is recorded with date/time and play time.
- Not available for Bloodborne yet: area, last lamp and fall detection (the area shows as *Desconhecida*). You can still type a cause or area by hand.
- Bloodborne characters are told apart by **name** (the save slot isn't read), so two Bloodborne characters with the same name share one entry in the list.
- The overlay works the same way: use a windowed or borderless window in the port's settings.
- This support hasn't been confirmed in a real play session yet. If the count doesn't go up when you die, send the *Diagnóstico* text in a bug report.

## 4. Characters and counts

The *Personagens* tab lists every character the counter has seen (*Saves / personagens*). The character currently being played is marked with a green dot and listed first.

- **How characters are told apart:** by game, save slot and character name (in Sekiro, by save slot only). Each character in the list is tagged with its game (*DSR* or *Sekiro*). If you delete a character and create a new one with the same name in the same slot, the counter notices that the play time went back to zero and starts a new count.
- **Where the number comes from:** the game keeps a death counter per character inside the save. The app reads it, so the count is exact even for deaths that happened before you installed the app, or while it was closed.
- *N morte(s) aconteceram antes do contador conhecer este personagem* means N deaths happened before the app first saw this character. They're included in the total but have no details in the list.

### Editing a count

*Editar contagem…* lets you set any number. The game is still read normally: new deaths are added on top of your number. While a count is edited:

- the app shows a **CONTAGEM EDITADA** badge and the game's real count plus your adjustment (*Jogo: 47 · ajuste manual: +3*);
- the overlay number uses the *edited* color.

*Voltar à contagem do jogo* removes the adjustment. The game's own counter is never modified.

### Dark Souls II: Scholar of the First Sin

Dark Souls II is recognized the same way: start the game (`DarkSoulsII.exe`) and load a character. There is nothing to link or configure.

- Each character shows a game tag in the list (*DSR* or *DS2*), and characters from different games are never mixed, even with the same name and slot.
- The count is the game's own total death counter for the character, the same number the save keeps, so deaths from before you installed the app are included.
- The level, NG+ cycle and play time come from the game too. The play time is the one shown in the game's load menu (to the second).
- Only **Scholar of the First Sin** (64-bit, the version Steam sells) is supported. The original Dark Souls II from 2014 is a 32-bit program with a different memory layout: the app shows *Versão do jogo não suportada (DS2 de 32 bits)* instead of guessing.

### Other actions

- *Exportar CSV…* saves the character's death list as a spreadsheet file (`;`-separated, UTF-8, opens in Excel).
- *Excluir personagem…* removes the character and its history **from this app only**. The game save is untouched. If you load that character again, it comes back without the old history.

## 5. The death list

The *Mortes* tab shows one row per death, newest first:

| Column | Content |
|---|---|
| *#* | The game's death number for this character |
| *Data / hora* | When it happened |
| *Área* | The map you were on (e.g. *Undead Burg / Undead Parish*, *Anor Londo*). Hover for map id and coordinates. |
| *Última fogueira* | The last bonfire you rested at |
| *Causa* | Cause of death |
| *Tempo de jogo* | In-game play time at that moment |

- **Cause:** two causes are filled in automatically:
  - **Bosses.** If a boss health bar was on screen when you died, the cause is that boss (e.g. *Chefe: Ornstein & Smough*), whatever finished you off: the boss itself, its adds, or a fall from its arena. All 26 bosses with a health bar are recognized. Bosses without one (the Hellkite on the Undead Burg bridge, for example) are not.
  - **Falls.** Otherwise, a drop of more than 10 m just before dying is marked *Queda*.

  The game doesn't keep "which enemy killed you" anywhere that can be read safely, so for everything else **click the cause cell** and type it or pick a suggestion (common enemies, traps, poison, invaders…). Automatic causes can be corrected the same way.
- **Right-click a row** to edit the cause, edit the area, or delete that record (deleting a record doesn't change the count).
- **Grey rows** (*Não registrada (contador fechado)*) are deaths that happened while the counter was closed. They count, but have no details.
- **Filter:** type in the box above the list to filter by area, bonfire or cause.
- *Por área* / *Por causa* show where and how you die the most, with percentages.

Areas are the game's maps, so a few names cover two regions (*Blighttown / Quelaag's Domain*, *Demon Ruins / Lost Izalith*). Dark Souls II works the same way (*The Lost Bastille / Sinners' Rise / Belfry Luna*, *Harvest Valley / Earthen Peak*…); a map the app doesn't know yet shows as *Mapa mAA_BB*. If a death landed on the wrong map, fix it with right-click → *Editar área*.

<a id="sekiro"></a>

## Sekiro: Shadows Die Twice

The counter also recognizes **Sekiro** (`sekiro.exe`, version 1.06, the current Steam version; older versions probably work but haven't been tested). Just open the game: everything above works the same way, with these differences.

- **Real deaths only.** Sekiro's own death counter goes up when the death screen appears, not when you go down. Resurrecting doesn't change the count; dying for real (choosing *Die*, or going down with no resurrection left) does. This is the counter the game itself keeps, the same one other death counters such as DSDeaths show.
- **One character per save slot.** Sekiro has no character names, so each slot is one character, shown as *Personagem do slot N* and tagged *Sekiro*. Its line shows the play time and the NG+ cycle (there's no character level).
- **Area** is the map you were on: *Hirata Estate*, *Ashina Outskirts* (also the Dilapidated Temple), *Ashina Castle*, *Ashina Reservoir*, *Abandoned Dungeon*, *Sunken Valley / Ashina Depths* (Sunken Valley, Gun Fort, Poison Pool…), *Ashina Depths (Hidden Forest / Mibu Village)*, *Senpou Temple, Mt. Kongo* and *Fountainhead Palace*.
- **Last idol:** instead of *Última fogueira* the list has *Último ídolo (área)*: the area of the Sculptor's Idol you'll come back to (the game's respawn point). The game keeps that idol only as a number, so the app shows its area, not the idol's name; hover the cell to see the number.
- **No automatic fall detection:** in Sekiro big drops are harmless and only bottomless pits hurt, so *Queda* is never filled in automatically. The cause suggestions list Sekiro's bosses and mini-bosses.

<a id="hollow-knight"></a>

## Hollow Knight

The counter also recognizes **Hollow Knight** (`hollow_knight.exe`, the current Steam version, built with Unity 6 in 2026). Just open the game and load a save. Hollow Knight works differently from the FromSoftware games:

- **The app does the counting.** Hollow Knight doesn't keep a death counter anywhere, so the app watches the game and adds one each time the Knight dies. The count lives only in the app (*Contada pelo app* under the number): deaths while the counter is closed are **not** counted, and the first time you play a save with the app open the count starts at 0. If you already died before, set your number with *Editar contagem…*; new deaths are added on top of it.
- **What counts as a death:** every time the Knight dies — the last mask is lost (to an enemy, a boss, spikes or acid), including the final death of a Steel Soul save, and also deaths in dreams (dream bosses, the Hall of Gods, the Pantheons), which wake you up instead of leaving a shade. Those are recorded with the cause *Sonho* when no boss is known.
- **What doesn't count:** spikes, acid or pits that only cost a mask and send you back to the last safe spot; quitting to the menu; loading a save (even one saved while the Knight was dying). One death is counted once, however long the death animation and the trip back to the bench take.
- **One character per save slot.** Each of the 4 save slots is a character, shown as *Slot N* and tagged *HK*. A Steel Soul save shows as *Slot N (Steel Soul)* and is kept apart from a normal save that used the same slot before. Starting a new game in a slot (the play time goes back near zero) starts a new entry.
- **Area** is the game's own map area (*Forgotten Crossroads*, *Greenpath*, *City of Tears*, *Godhome*…). **Último banco (área)** shows the area of the bench you'll respawn at.
- **Cause:** dying in a boss arena while that boss is still undefeated records the boss (*Chefe: Hornet Protector*, *Chefe: Soul Master*…); dream bosses, the Colosseum trials and every Godhome fight are recognized too. Other deaths have no automatic cause; the suggestions list Hollow Knight's bosses.
- No fall detection, character level or NG+ (Hollow Knight has none).
- Use a windowed or borderless window for the overlay (in the game's video options).
- Not yet confirmed in a real play session. Old versions of the game (1.4.3.2 and earlier, which speedrunners use) are not supported: *Diagnóstico* says so. If the count doesn't go up when you die, send the *Diagnóstico* text in a bug report.

## 6. Settings

*Configurações* tab:

| Option | What it does |
|---|---|
| *Executável do jogo* / *Procurar…* / *Detectar na Steam* | Path used by *Iniciar Dark Souls* |
| *Iniciar com o Windows (minimizado na bandeja)* | Start with Windows, in the tray |
| *Fechar a janela mantém o contador rodando na bandeja* | The window's X button hides to the tray instead of quitting |
| *Atalho Ctrl+Shift+D mostra/oculta o overlay* | Enables the global hotkey |
| *Mostrar a contagem por cima do jogo* | Overlay on/off |
| *Só quando o jogo estiver em primeiro plano* | Hide the overlay when the game isn't the active window |
| *Esconder no menu principal do jogo* | Hide on the title screen |
| *Mostrar o nome do personagem* | Adds the character name above the count |
| *Texto antes do número* | The label, e.g. `Mortes: ` or `Deaths: ` |
| *Fonte* / *Outra fonte instalada* / *Negrito* / *Tamanho* | Font settings (any installed font works) |
| *Ajustar à resolução do jogo* | Sizes are designed for 1080p and scaled to the game window |
| *Posição* / *Distância horizontal* / *Distância vertical* | Which corner, and how far from it |
| *Contorno* | Outline thickness |
| *Cor normal* / *Cor quando a contagem foi editada* / *Cor do contorno* | Colors |
| *Pré-visualizar o overlay agora* | Shows the overlay immediately (over the game window if it's open, otherwise over your monitor) while you adjust it |
| *Deixar a janela do jogo sem bordas* | Makes the game window borderless and the size of your monitor |
| *Gravar a contagem num arquivo de texto para o OBS* | Writes the count to a text file that OBS can show (off by default, see [Streaming](#7-streaming-obs)) |
| *Texto no OBS* | What goes in that file: `{mortes}` becomes the count and `{nome}` the character name |
| *Arquivo* / *Escolher arquivo…* / *Usar o padrão* / *Abrir pasta* / *Copiar caminho* | Where the file is written (default `%APPDATA%\DSDeathCounter\obs.txt`) |
| *Abrir pasta de dados* | Opens the folder where your data is stored |

## 7. Streaming (OBS)

The overlay is drawn in its own window on top of the game, not inside the game, so:

- **Display Capture** in OBS shows the overlay;
- **Game Capture** and **Window Capture** of the game **don't**.

With Game Capture or Window Capture, use the OBS text file instead: the app keeps the current count in a small text file, and an OBS text source shows it. You place and style it in OBS like any other source.

### Setting up the text source

1. In the app, open *Configurações* → *OBS (lives)* and tick *Gravar a contagem num arquivo de texto para o OBS*. The line *Gravando: …* shows what is in the file.
2. Optional: change *Texto no OBS* (default `Mortes: {mortes}`), for example to `Deaths: {mortes}`.
3. Click *Copiar caminho* to copy the file's location. By default it is `%APPDATA%\DSDeathCounter\obs.txt`; *Escolher arquivo…* picks another `.txt` file and *Usar o padrão* goes back to the default.
4. In OBS, in the **Sources** dock, click **+** and choose **Text (GDI+)**. Give it a name (e.g. "Deaths") and click **OK**.
5. In the properties window, tick **Read from file**, click **Browse** next to **Text File (UTF-8)** and pick the file (you can paste the copied path into the file dialog). Click **OK**.
6. Choose font, size, color and outline in that same window. Drag the source to where you want it, and keep it above the game capture in the Sources list.

The file changes a moment after each death (OBS checks it about once a second). It shows the character being played; with the game closed, it keeps showing the last character you played.

Placeholders for *Texto no OBS*:

| Placeholder | Becomes |
|---|---|
| `{mortes}` (or `{deaths}`) | The death count, the same number the app shows (including manual edits) |
| `{nome}` (or `{name}`) | The character's name |

Anything else is written as typed: `{nome}: {mortes} deaths` becomes `Chosen Undead: 42 deaths`.

If the app shows *Não foi possível gravar o arquivo*, the chosen folder no longer exists or can't be written to: pick another file or click *Usar o padrão*.

## 8. Your data

Everything is stored in one file:

```
%APPDATA%\DSDeathCounter\data.json
```

That file holds settings, characters and the full death history. To back it up or move to another PC, copy that file (with the app closed). Uninstalling the app keeps it.

If the OBS text file is on, `obs.txt` is in the same folder by default. It only holds the text shown in OBS and is rewritten as needed, so it doesn't need a backup.

## 9. Troubleshooting

**"O jogo está rodando como administrador"**: Windows doesn't let a normal program read a program running as administrator. Run the game normally, or run the counter as administrator too.

**The overlay doesn't show:**
- check the game's display mode (windowed/borderless, see [3](#3-while-you-play));
- press `Ctrl+Shift+D` (it may be toggled off);
- the overlay hides when the game isn't the active window, or on the title screen (both configurable);
- test it with *Pré-visualizar o overlay agora*.

**The overlay doesn't show in OBS:** Game Capture and Window Capture don't capture it. Use the [OBS text file](#7-streaming-obs).

**The count went down:** you restored an older copy of your save. The counter follows the game's own number.

**A character shows up twice:** a character is identified by slot + name, so a character that moved to another slot (for example, by copying save files around) is seen as new. Delete the duplicate in the app if you want.

**Dark Souls II says *Versão do jogo não suportada (DS2 de 32 bits)*:** you are running the original 2014 Dark Souls II. Only Scholar of the First Sin is supported.

**Bloodborne isn't detected:** the counter looks for the port's game process, `bbport.exe`, and was checked against port v0.5 with Bloodborne 1.09. Other ports, emulators and game versions aren't supported. If *Diagnóstico* shows *NÃO ENCONTRADO*, the game data wasn't recognized; please send that text in a bug report.

**Hollow Knight: the count didn't go up:** the app has to be open when you die, since the game keeps no counter (see [Hollow Knight](#hollow-knight)). If *Diagnóstico* shows *Classes:* with something *não encontrado* or *não suportado*, the game version wasn't recognized; please send that text in a bug report.

**Something looks wrong:** open *Diagnóstico*, click *Copiar* and paste the text into a [bug report](https://github.com/Theuzin69/ds-death-counter/issues/new/choose). It shows the game version, the addresses found and the values being read (deaths, map, position, boss health bars…).

**My antivirus complained:** see the [SmartScreen/antivirus section](../README.md#smartscreen). The app reads another program's memory, which some antivirus heuristics dislike. It never writes to the game.

## 10. Uninstalling

- **Installer version:** Windows Settings › Apps › *DS Death Counter* › Uninstall.
- **Portable version:** quit it from the tray icon and delete the folder. If you enabled *Iniciar com o Windows*, turn it off first in *Configurações* (or in Task Manager › Startup).

Your history in `%APPDATA%\DSDeathCounter` is kept in both cases; delete that folder if you want it gone.
