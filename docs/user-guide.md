# User guide

**English** · [Português (Brasil)](guia-do-usuario.md)

The app's interface is in Brazilian Portuguese. This guide quotes the on-screen labels in *italics* and explains them in English.

- [1. Installing](#1-installing)
- [2. First launch](#2-first-launch)
- [3. While you play](#3-while-you-play)
- [Elden Ring](#elden-ring)
- [4. Characters and counts](#4-characters-and-counts)
- [5. The death list](#5-the-death-list)
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

1. **Link to the game** (*Vincular ao jogo*). The app looks for Dark Souls Remastered in your Steam libraries. If it doesn't find it, click *Procurar DarkSoulsRemastered.exe…* and select the game's exe. This path is only used by the *Iniciar Dark Souls* (launch game) button: the counter recognizes the running game by itself either way.
2. **Always on** (*Sempre ligado*). Keep *Iniciar com o Windows, minimizado na bandeja* checked. The app then starts with Windows in the system tray, so your deaths are counted whenever you open the game, without you having to remember to open the counter.

Click *Começar* (start).

## 3. While you play

### Status line

At the top of the window:

| Status | Meaning |
|---|---|
| *Aguardando o Dark Souls Remastered ou o Elden Ring abrir…* | No supported game is running. *Iniciar Dark Souls* launches Dark Souls Remastered. |
| *Dark Souls encontrado, preparando a leitura…* | The game was found and the counter is locating its data (takes a moment). |
| *Conectado ao jogo (versão X). Carregue um personagem.* | Connected; you're on the title screen. |
| *Contando as mortes de NAME (slot N)* | Counting. |
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

## Elden Ring

The counter also recognizes **Elden Ring** (`eldenring.exe`), with nothing to set up. Each character gets its own count and history, as in Dark Souls Remastered. In the character list, Elden Ring characters are tagged *Elden Ring* and Dark Souls Remastered ones *DSR*.

### Easy Anti-Cheat decides how the count is read

| How the game was started | What the counter does |
|---|---|
| Normally through Steam (Easy Anti-Cheat on) | It **never opens the game process**. It reads the death counter from the save file, `%APPDATA%\EldenRing\<steamid>\ER0000.sl2`, which the game writes after every death. The count changes **a few seconds after you die**, once the game has saved. |
| Without Easy Anti-Cheat (offline launch with EAC disabled, Seamless Co-op) | It reads the game's memory, read-only, like Dark Souls Remastered. The count changes instantly, and each death gets the exact area and fall detection. |

The counter looks for EAC without touching the game:

- running `EasyAntiCheat*.exe` or `start_protected_game.exe` processes;
- the *EasyAntiCheat* and *EasyAntiCheat_EOS* Windows services;
- whether `start_protected_game.exe` (EAC's launcher) started the game.

It checks twice, two seconds apart, before it opens the game, and keeps checking every two seconds after that. If EAC appears, the counter drops its access at once and stays on the save file until the game is closed. If another game with EAC is running at the same time, Elden Ring is treated as protected too, which only means the count comes from the save file.

The save file is opened read-only and shared, for a few milliseconds after each write, and is never modified.

### Status line (Elden Ring)

| Status | Meaning |
|---|---|
| *Elden Ring encontrado, verificando o Easy Anti-Cheat antes de ler…* | The game is running; the counter is making sure EAC is off before touching it (a few seconds). |
| *Elden Ring sem Easy Anti-Cheat, preparando a leitura da memória…* | No EAC; the counter is locating the game's data. |
| *Contando as mortes de NAME (slot N)* | Counting from memory (no EAC). |
| *Contando as mortes de NAME (slot N) pelo arquivo de save* | EAC is on; counting from the save file. |
| *Elden Ring com Easy Anti-Cheat: aguardando o jogo salvar para saber qual é o personagem* | You have more than one character and the game hasn't written the save since the counter started, so it doesn't know yet which one you're playing. It finds out at the next save (for example when you rest at a grace or die). |

### What each death records

- *Área*: legacy dungeons and dungeons by name (*Stormveil Castle*, *Murkwater Catacombs*…). In the open world, the 256 m map square you were in, written as *Region - landmarks* (for example *Limgrave - Church of Elleh, Stranded Graveyard Exit*). In the Realm of Shadow (DLC) only the region is named. With EAC on, the area is where the game left your bloodstain, which is where you died. Hover to see the map id (`m60_42_36_00`).
- *Última graça (área)*: the area of the last Site of Grace you rested at. The save keeps the grace as a map object, not by name, so the column shows its area.
- *Por área* groups Elden Ring deaths by region (*Limgrave*, *Liurnia of the Lakes*, *Stormveil Castle*…).
- *Queda* (fall) is only detected when reading memory (no EAC), because it needs the player's position.
- The cause suggestions include the main Elden Ring and Shadow of the Erdtree bosses.

### Tips

- Use the game's own **Borderless Windowed** screen mode so the overlay can draw over it. The *Deixar a janela do jogo sem bordas* button is disabled while EAC is running: the counter stays completely passive.
- With EAC on, the counter can't tell the title screen from gameplay, so *Esconder no menu principal do jogo* doesn't apply; the overlay shows whenever the game window is active.
- The *Iniciar Dark Souls* button only launches Dark Souls Remastered. Start Elden Ring from Steam.
- Seamless Co-op saves (`ER0000.co2`) are only used when the game runs without EAC.

## 4. Characters and counts

The *Personagens* tab lists every character the counter has seen (*Saves / personagens*). The character currently being played is marked with a green dot and listed first.

- **How characters are told apart:** by save slot and character name. If you delete a character and create a new one with the same name in the same slot, the counter notices that the play time went back to zero and starts a new count.
- **Where the number comes from:** the game keeps a death counter per character inside the save. The app reads it, so the count is exact even for deaths that happened before you installed the app, or while it was closed.
- *N morte(s) aconteceram antes do contador conhecer este personagem* means N deaths happened before the app first saw this character. They're included in the total but have no details in the list.

### Editing a count

*Editar contagem…* lets you set any number. The game is still read normally: new deaths are added on top of your number. While a count is edited:

- the app shows a **CONTAGEM EDITADA** badge and the game's real count plus your adjustment (*Jogo: 47 · ajuste manual: +3*);
- the overlay number uses the *edited* color.

*Voltar à contagem do jogo* removes the adjustment. The game's own counter is never modified.

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

- **Cause:** falls are detected automatically (a drop of more than 10 m just before dying) and marked *Queda*. The game doesn't store "who killed you" anywhere that can be read safely, so for everything else **click the cause cell** and type it or pick a suggestion (bosses, common enemies, traps, poison, invaders…).
- **Right-click a row** to edit the cause, edit the area, or delete that record (deleting a record doesn't change the count).
- **Grey rows** (*Não registrada (contador fechado)*) are deaths that happened while the counter was closed. They count, but have no details.
- **Filter:** type in the box above the list to filter by area, bonfire or cause.
- *Por área* / *Por causa* show where and how you die the most, with percentages.

Areas are the game's maps, so a few names cover two regions (*Blighttown / Quelaag's Domain*, *Demon Ruins / Lost Izalith*). If a death landed on the wrong map, fix it with right-click → *Editar área*.

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
| *Abrir pasta de dados* | Opens the folder where your data is stored |

## 7. Streaming (OBS)

The overlay is drawn in its own window on top of the game, not inside the game, so:

- **Display Capture** in OBS shows the overlay;
- **Game Capture** and **Window Capture** of the game **don't**.

A dedicated OBS text source (a file with the current count) is planned.

## 8. Your data

Everything is stored in one file:

```
%APPDATA%\DSDeathCounter\data.json
```

That file holds settings, characters and the full death history. To back it up or move to another PC, copy that file (with the app closed). Uninstalling the app keeps it.

## 9. Troubleshooting

**"O jogo está rodando como administrador"**: Windows doesn't let a normal program read a program running as administrator. Run the game normally, or run the counter as administrator too.

**The overlay doesn't show:**
- check the game's display mode (windowed/borderless, see [3](#3-while-you-play));
- press `Ctrl+Shift+D` (it may be toggled off);
- the overlay hides when the game isn't the active window, or on the title screen (both configurable);
- test it with *Pré-visualizar o overlay agora*.

**The count went down:** you restored an older copy of your save. The counter follows the game's own number.

**A character shows up twice:** a character is identified by slot + name, so a character that moved to another slot (for example, by copying save files around) is seen as new. Delete the duplicate in the app if you want.

**Something looks wrong:** open *Diagnóstico*, click *Copiar* and paste the text into a [bug report](https://github.com/Theuzin69/ds-death-counter/issues/new/choose). It shows the game version, the addresses found and the values being read (deaths, map, position…).

**Elden Ring: the count changes a few seconds after I die:** expected while Easy Anti-Cheat is on. The counter waits for the game to write the save (see [Elden Ring](#elden-ring)).

**Elden Ring: *aguardando o jogo salvar*:** you have several characters and the counter is waiting for the game's next save to know which one you're playing. Rest at a grace or keep playing; it shows up at the next save.

**Elden Ring: the count doesn't change at all:** open *Diagnóstico*. The *Save:* lines show which file is read, when it was last read, and what each slot holds. If a slot says the death counter was not found, a game update changed the save format: please report it with the diagnostics text.

**My antivirus complained:** see the [SmartScreen/antivirus section](../README.md#smartscreen). The app reads another program's memory, which some antivirus heuristics dislike. It never writes to the game.

## 10. Uninstalling

- **Installer version:** Windows Settings › Apps › *DS Death Counter* › Uninstall.
- **Portable version:** quit it from the tray icon and delete the folder. If you enabled *Iniciar com o Windows*, turn it off first in *Configurações* (or in Task Manager › Startup).

Your history in `%APPDATA%\DSDeathCounter` is kept in both cases; delete that folder if you want it gone.
