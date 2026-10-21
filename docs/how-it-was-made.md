# How DS Death Counter was made

**English** · [Português (Brasil)](como-foi-feito.md)

> [!IMPORTANT]
> **AI disclosure.** DS Death Counter was built with artificial intelligence. Practically all of the source code, the reverse-engineering of the game's memory, the tests, the installer, the release automation and this documentation were produced by **Claude**, an AI model by Anthropic (*Claude Opus 5.5*), working through **Claude Code**, Anthropic's coding agent. The project was conceived, directed and tested in the game by **MatheusMuzzi**.

This page explains who did what, how the app was built step by step, how it was checked, and what its limits are. The goal is full transparency: this is software that reads another program's memory, and you deserve to know exactly how it came to be. The code is open, so all of it can be reviewed.

## Who did what

| | MatheusMuzzi (human) | Claude (AI) |
|---|---|---|
| Idea and purpose | ✔ | |
| Requirements and priorities | ✔ | |
| Decisions (language, license, docs, publishing) | ✔ | proposed options |
| Research of game memory offsets | | ✔ |
| Reverse-engineering of the game executable | | ✔ |
| Architecture and all source code | | ✔ |
| Automated tests, installer, CI/release workflow | | ✔ |
| Documentation (this page included) | reviewed | ✔ |
| Testing in the real game | ✔ | |

The whole project was done in a single conversation in Claude Code, in Portuguese. The AI used tools on the developer's PC (compiler, Python, a disassembler, screenshots of its own windows) to build and check its work.

## The idea

An automatic death counter, for streamers or for anyone who simply wants to know how many times, and where, they died the most in a game. Starting with Dark Souls Remastered.

The original requirements were:

1. Open the app once and link it to Dark Souls; from then on, deaths are counted whenever the game is open.
2. The counter goes up automatically every time the player dies.
3. If the game is closed, counting stops.
4. When the game is reopened, counting resumes where it stopped.
5. The count is shown as an overlay on top of the game.
6. *(optional)* A list of all deaths showing the area and what killed the player (enemy, boss, fall…).
7. *(optional)* A separate count for each save.
8. *(optional)* Counts can be edited in the app; an edited count is flagged in the app and changes color in the overlay.

Plus two constraints: a normal Windows executable/installer anyone can use, and **no risk of a Steam ban**.

## Step 1: where does the game keep the death count?

FromSoftware games keep a **per-character death counter** in a game structure usually called `GameDataMan`, and it is stored in the save file. Reading that counter, instead of trying to detect deaths some other way, gives requirements 2, 3, 4 and 7 almost for free: the number is always exact, it survives closing the game, and each character has its own.

The Souls community has documented this for years. Claude read the source code of public tools to collect the offsets (no code from them was copied; offsets are facts about the game):

- [DSDeaths](https://github.com/quidrex/DSDeaths): death counter at `GameDataMan + 0x98`.
- [SoulSplitter / SoulMemory](https://github.com/FrankvdStam/SoulSplitter): byte signatures that locate `GameDataMan`, `GameMan`, `WorldChrMan`; play time, NG+ and save slot offsets; version differences.
- [DSR-Gadget](https://github.com/JKAnderson/DSR-Gadget) and [DS-Gadget](https://github.com/JKAnderson/DS-Gadget): character data layout (name, level), last bonfire, player position, and how offsets moved between game patches.

## Step 2: checking the offsets against the real game

Instead of trusting those numbers blindly, Claude wrote small Python scripts using the [Capstone](https://www.capstone-engine.org/) disassembler to analyze `DarkSoulsRemastered.exe` directly (reading the file only, never modifying it):

- every byte signature had to match **exactly one** place in the code;
- signatures taken from **different projects** for the same structure had to resolve to the **same address**, and they did for `GameMan`, `GameDataMan` and `WorldChrMan`;
- the analysis showed that the executable available for development was the original **1.01** release, while Steam distributes **1.03.x**, and that a few offsets differ between them. So the app **detects the game version** and, where possible, reads the version-specific offsets straight from the game's code at runtime instead of hard-coding them.

## Step 3: finding the area (not documented anywhere)

No public tool documented where Dark Souls Remastered keeps the **current map**. Claude disassembled the small functions next to the code that stores the player's *last stable position* (the spot on solid ground the game remembers) and found a matching setter/getter pair that stores a 4-byte **map id** (`mAA_BB`, e.g. `m10_01` = Undead Burg / Undead Parish) right beside it, at `GameMan + 0xBC0` in 1.01.

Because it's the map of the last solid ground, it is also a good answer to "where did I die?" for falls into pits. The app locates this field by its code pattern at runtime, so it adapts to other game versions. The in-game test confirmed it: a death in Undead Burg was recorded with the right area.

## Step 4: design decisions

| Decision | Why |
|---|---|
| **C++ / Win32** | One small (~1 MB) executable, no runtime to install, starts instantly and uses almost nothing while running in the tray. |
| **Read-only access** | The game process is opened with read permission only. The app never writes to it, injects no DLL and installs no hooks. That's the "no ban risk" constraint. |
| **Use the game's own counter** | Exact counts, survives restarts, counts deaths that happen while the app is closed, separate per character. |
| **Edits stored as an adjustment** | *shown = game count + adjustment*. The game is never touched, and "edited" is always explicit (requirement 8). |
| **Overlay as a separate window** | A transparent, click-through, always-on-top window drawn with GDI+ that follows the game window. Nothing is drawn inside the game. The trade-off is that exclusive fullscreen can hide it, so the app offers a "borderless window" button. |
| **UI with Dear ImGui on Direct3D 11** | A compact, good-looking interface without heavy frameworks. |
| **Character = save slot + name** | The game data doesn't have a unique character id. Play time going back to zero reveals a recreated character with the same name. |
| **Cause of death** | Falls are detected from the player's height history, and bosses from the boss health bar (added after 1.0.0, see [step 6](#step-6-who-killed-you-after-100)). The game doesn't keep "which enemy killed you" anywhere that can be read reliably and safely, so other causes are picked by the user from suggestions. |
| **Data in JSON** | One readable file in `%APPDATA%`, written atomically so a crash can't corrupt it. |

## Step 5: testing

What the AI checked on the development PC:

- the project compiles with MSVC with **no errors and no warnings**;
- `tests/scan_test` runs the app's own scanner against the game executable on disk and compares the result with the addresses verified by hand;
- `tests/boss_cause_test` checks the boss health bar → cause table (also run by GitHub Actions);
- `tests/overlay_test` draws the overlay, which was captured on screen and inspected (normal and "edited" colors);
- the main window was rendered with sample data and captured to check the layout;
- the installer was installed and uninstalled silently to confirm it leaves nothing behind.

What the human checked in the real game:

- the deaths of existing saves were picked up correctly;
- a new death in Undead Burg was counted and recorded with the correct area.

Boss detection (step 6) has not been tested in the game yet.

Every release is built by GitHub Actions from the public code, with checksums and build provenance attestations (see the [README](../README.md#smartscreen)).

## Dark Souls II: Scholar of the First Sin

Dark Souls II support was added later, in a separate Claude Code session, with the same method and the same rules: the game's executable was only read as a file, nothing from the game folder was run or extracted, and the app still only calls `ReadProcessMemory`.

**Which executable.** The `DarkSoulsII.exe` available for development is *Scholar of the First Sin* (SotFS), 64-bit, file version 1.0.2.0 (the 1.02 patch, module size `0x20B6000`). Steam sells SotFS 1.03 today, which wasn't available, so the app finds everything through byte signatures instead of fixed addresses. The original 2014 Dark Souls II is a 32-bit program with a different layout. The app detects a 32-bit process (`IsWow64Process`) and says it isn't supported instead of guessing.

**Main structure.** Almost everything hangs off one global, `GameManagerImp`. Two signatures from different projects ([SoulSplitter](https://github.com/FrankvdStam/SoulSplitter) and [DS2S-META](https://github.com/Nordgaren/DS2S-META)) each match exactly once and resolve to the same address, `exe+0x160B8D0`. DSDeaths' fixed address (`0x16148F0`) isn't referenced anywhere in 1.02, so it must be the 1.03 address. That confirms 1.03 moved the globals, which is why the app uses signatures.

**The death counter.** The public tools disagreed. DSDeaths reads `PlayerParam + 0x104` (with `PlayerParam = [[GameManagerImp] + 0xD0] + 0x490`), while DS2S-META names `PlayerParam + 0x1A4` "TotalDeaths". The function that loads `PlayerParam` from the save explains why. `+0x1A4` is a single total, clamped to 99,999,999. `+0x104` is the first of an array of 20 such counters. To settle it, Claude decrypted copies of the developer's own DS2 save files (read-only copies in a temporary folder; the saves use a publicly known AES key). For all three characters (463, 381 and 17 deaths) both fields hold the same number, and the character name, souls and soul memory sit where the code says. The app counts with the total (`+0x1A4`) and shows the other one in the *Diagnóstico* tab so it can be compared in-game.

**Name, slot and play time.** These were found by following the code that writes the load menu's slot summaries:

- name at `[[GameManagerImp + 0xA8] + 0xC0] + 0x24`, class at `+0x64`, NG cycle at `+0x68`, stored as 1 for the first playthrough;
- the active save slot at `[[GameManagerImp + 0xA8] + 0xD8] + 0x1368`;
- ten 0x1F0-byte summaries, with the play time in seconds at `+0x1CC`.

DS2 has no running play-time counter. The game's own *GetPlayTimeSeconds* adds the time since the summary was last stored, measured with `QueryPerformanceCounter`, and the app does the same: that counter is system-wide, so the game's time stamp means the same in both processes. The decrypted menu data matched this layout (91h27m and 20h19m).

**Area and bonfire.**

- **Current map.** The map manager (`GameManagerImp + 0x38`) keeps the current map index at `+0x170`, which points into a table of map ids (`mAA_BB`, packed as `0xAABB0000`).
- **Area names.** They come from the area ids in DS2S-META's bonfire list, so a map without a bonfire shows as *Mapa mAA_BB*.
- **Last bonfire.** It's at `[GameManagerImp + 0x70] + 0x16C`, with names from the same list (MIT license).

**Signatures checked.** The app uses 13 signatures. Each one must match exactly once in the 1.02 executable, and the offsets they contain are read from the code at runtime. If one isn't found (for example in a future patch), the 1.02 value is used and the *Diagnóstico* tab marks it with `*`. `tests/scan_test` recognizes `DarkSoulsII.exe` and compares every value with the ones checked by hand. Not verified statically: the player position (`[PlayerCtrl + 0x100] + 0x80`, from SoulSplitter, only used for fall detection) and the "in the world" rule (`PlayerCtrl` exists, as in DS2S-META). Both are on the list to confirm in the game.
<a id="sekiro"></a>

## Adding Sekiro: Shadows Die Twice

Sekiro support was added later in the same way, by a separate Claude Code session (other sessions were adding other games in parallel). The executable available was `sekiro.exe` **1.06**, the current Steam version. That copy already had Steam's DRM layer removed, so its code is exactly what the Steam 1.06 build runs once it's loaded. It was only read as a file for analysis, never run.

**The death counter.** DSDeaths reads Sekiro's counter at `GameDataMan + 0x90`. Instead of trusting that, Claude followed the game's own `AddDeathCount` script function: the game's script binding table pairs that name with a small function that ends in a saturating `+1` on `GameDataMan + 0x90`. The game uses the same `+1` helper for two leftover multiplayer counters (`+0x80` and `+0x84`), so code shape alone can't tell them apart. At runtime the app follows the `AddDeathCount` name through the binding table to the right one.

**Resurrection or real death?** The only code that calls `AddDeathCount` is the game's player-death handler. It is registered on the death event of the player (entity 10000), and it is also the handler that runs the death-penalty checks and the big on-screen death text. Resurrecting doesn't go through it. So the game's counter, and the number the app shows, counts **real deaths only**: dying for good, not going down and resurrecting. This conclusion comes from reading the code and still has to be confirmed in the game. The two could in principle be told apart (HP reaching 0 while the counter stays put is a resurrection), but the app doesn't count resurrections.

**Everything else** was also read from the game's code. Every structure is located at runtime by a signature, and the offsets are taken from the instructions themselves:

| Data | Where (1.06) | How it was confirmed |
|---|---|---|
| Play time | `GameDataMan + 0x9C` | the timer code adds the frame time there and caps it at 999:59:59 |
| NG+ cycle | `GameDataMan + 0x70` | incremented by one and capped at 9999 |
| Save slot | `GameMan + 0xAE4` | the save routine falls back to it when no slot is given, and rejects values above 9 (10 slots) |
| In world, HP, position | `WorldChrMan + 0x88` (player) → modules `+0x1FF8` → data module `+0x18` (HP `+0x130`, max `+0x134`) and physics module `+0x68` (position `+0x70`) | the game's `SetHp` (which clamps to the max HP right after it), the module setup code (module 3 = data, 13 = physics) and the position getter |
| Current map | `FieldArea + 0x28` (block index) into the world block list at `[FieldArea + 0x18] + 0x20` (0xB0-byte entries, map id at `+0x8`) | the code behind the `GetCurrentMapAreaNo` script function |
| Last idol (respawn point) | `GameMan + 0xB4C`, overridden by `+0xB50` while that is ≥ 0 | set by the `SetReturnPointEntityId` script function and when resting; read by the respawn code |

Map names came from matching map ids with the Sculptor's Idol event flags, which use the same numbering (`1AAB…` belongs to map `mAA_0B`, e.g. the Ashina Castle idols `1111000x` are in `m11_01`). The respawn point is an entity id, which tells the idol's map but not which idol it is. Idol names live in the game's data archives, which weren't extracted, so the app shows the idol's area instead.

The signatures were written from scratch for this app; the community tools (some are GPL) were used for facts only. Each signature matches exactly once in 1.06. `GameDataMan` is confirmed by three independent pieces of code, and `GameMan`, `WorldChrMan` and `FieldArea` by two each. If two of them ever disagree, that feature is switched off and the *Diagnóstico* tab says *DIVERGENTE* instead of reading the wrong place. `tests/scan_test` recognizes `sekiro.exe` and checks every signature against the addresses found by hand in 1.06.

Sekiro has no character names or levels, so a Sekiro character is identified by its save slot, and its keys in `data.json` start with `sekiro|` so they never mix with Dark Souls characters. Sekiro has no fall damage outside bottomless pits, so the fall heuristic is off for it.
## Bloodborne (PC port)

Bloodborne support targets one specific program, the **Bloodborne PC Windows port** v0.5. Before anything else, Claude worked out what that program is using only its files: file listings, PE metadata (imports, sections, signatures, manifests), the launcher's packaged contents and its plain Python scripts. Nothing from it was run. It turned out to be a loader, `bbport.exe`, built with MinGW, unsigned and not packed. It runs the game's own PS4 x86-64 code (`eboot.bin`) directly, with graphics code taken from the [shadPS4](https://github.com/shadps4-emu/shadPS4) project. The launcher is a PyInstaller + Qt app that starts it through `run.bat`.

What matters for the counter is that the port puts the whole game image in one block of memory, at a fixed address in v0.5 (`0x800000000` = game address 0). So the same approach as for Dark Souls Remastered works:

- The game's scripts call a Lua function named `AddDeathCount` when the death message appears. Its native code adds 1 to a field of the game's `GameDataMan` (offset `0x88` in 1.09). A neighbouring `AddTrueDeathCount` increments `0x84`; no script calls it, so the app only shows it in *Diagnóstico*.
- The play time (`0x94`, milliseconds, capped at 999:59:59 like the other FromSoftware games), the NG+ cycle (`GetClearCount`, `0x68`) and the level (`GetLocalPlayerSoulLv`, player data at `GameDataMan + 0x8`, level `+0x90`) come from other functions with their own byte signatures. All four signatures match exactly once and point to the same `GameDataMan`. The port's optional performance patches don't touch any of those bytes.
- The character name has no code signature of its own. A save file stores the same player data block, and its stats line up field by field with the offsets used by the code, which puts the name `0x18` bytes after the level (`+0xA8`). The app only uses the name to tell characters apart, and it validates it while reading.
- There is no "player is in the world" pointer: the app treats "play time is running" as in game, because play time stops on the title screen.

`tests/bb_scan_test` runs the scanner on an `eboot.bin` file and compares the result with these values. `tests/bb_fake_port` is a stand-in process named `bbport.exe`. It maps the game image the same way, never runs any of it, and plays a short script (title screen, a character, three deaths, back to the title screen). `tests/bb_live_test` connects to that stand-in like the app does. It was used to check the connection, the fallback search for the image at another address, the deaths, name, level and NG+, and the in-game state. It hasn't been tested in a real play session yet.
## Step 6: who killed you? (after 1.0.0)

Requirement 6 also asked for *what* killed the player, and version 1.0.0 only recognized falls. The follow-up research went back to the executable, looking for any place where the game keeps the killer and that can be read safely.

**Ordinary enemies: not reliably readable.** The game raises the death counter from its event script (a function called `AddDeathCount`), which knows nothing about the attacker. The code that applies damage is protected by the game's anti-tamper obfuscation, and no stored "last attacker" field turned up: hits are passed around as short-lived event messages (used by the event scripts' "damaged by" condition) and a pool of recent-hit records whose meaning can't be confirmed without attaching a debugger to the running game, which the read-only promise rules out. Guessing "the nearest enemy" would get archers, groups and poison wrong, so the app doesn't guess.

**Bosses: readable and exact.** The boss health bar is a different story. The event scripts show it with instruction 2003:11 (*display boss health bar*: on/off, character, slot, name id), and the game keeps it in its `MenuMan` structure: two slots (two bars, as in Ornstein & Smough or the Bell Gargoyles) of 0x14 bytes at `MenuMan + 0x1050` in 1.01, each starting with the boss's NPC name id and its character handle. The bar is cleared when the boss dies (the script turns it off) and when the event scripts restart, which is what happens on the reload after a death. So "a bar was on screen at the last moment the player was alive" means "died in that boss fight", whatever landed the last hit.

How it was checked:

- The structure was found by following the game's own script function `SetBossGauge` to its implementation, then confirmed with the function that clears the bars, the constructor that initializes them, and the class name (RTTI) of the object that holds them, `NS_FRPG::MenuMan`.
- At runtime the app reads the `MenuMan` address and the bar offset straight from two small game functions (one clears the bars, the other returns a slot). If both are found they must agree; if they disagree, or neither is found, boss detection switches itself off instead of guessing. `tests/scan_test` compares the result with the 1.01 values verified by hand.
- The name ids come from the game's own files: every 2003:11 instruction in `event/*.emevd` was decoded (28 different name ids: 27 bars with a name, covering the 26 boss fights, plus one id in New Londo that has no name in the game files) and matched with the NPC name table in `msg/ENGLISH/item.msgbnd`. Each boss is recorded with the same wording as the cause suggestions (*Dragon Slayer Ornstein* and *Executioner Smough* both become *Chefe: Ornstein & Smough*), so the "by cause" statistics group automatic and hand-picked causes together. `tests/boss_cause_test` checks that table.

A boss fight wins over the fall heuristic: falling off Taurus Demon's bridge or into Bed of Chaos' pits counts as a death to that boss. Bosses without a health bar (the Hellkite Dragon on the Undead Burg bridge, for example) are not detected.

All of this comes from static analysis of the 1.01 executable. Two things still need the real game: that the bar is still set at the last moment the player is alive, and that the same code patterns exist in the 1.03.x builds. The *Diagnóstico* tab shows the live boss bar values to make both easy to check.

## Known limitations

- Ordinary enemies aren't detected automatically; bosses with a health bar and falls are. Bosses without a health bar (such as the Hellkite Dragon) aren't either.
- Exclusive fullscreen may hide the overlay; windowed/borderless always works.
- The interface is in Portuguese only (English is planned).
- Dark Souls II: only Scholar of the First Sin (64-bit) is supported; the current Steam build (1.03) is found by signatures but hasn't been confirmed in-game yet.
- Bloodborne (PC port) is experimental: no area, lamp or fall detection, characters are told apart by name only, and it hasn't been confirmed in a real play session yet.
- Support for the 1.03.x Steam builds relies on offsets published by the community tools above, plus the runtime version detection. If something is off on your version, the *Diagnóstico* tab shows what's being read; please open an issue.
- Sekiro was checked against the 1.06 executable only; older versions haven't been tested. The in-game behavior (resurrections not counted, slot switching, area and idol) still has to be confirmed by playing. Idol names aren't known, only the idol's area.

## Tools

[Claude Code](https://www.anthropic.com/claude-code) with Claude Opus 5.5 · Visual Studio 2022 (MSVC) · [Dear ImGui](https://github.com/ocornut/imgui) · [nlohmann/json](https://github.com/nlohmann/json) · Python + [Capstone](https://www.capstone-engine.org/) (analysis only, not shipped) · [Inno Setup](https://jrsoftware.org/isinfo.php) · GitHub Actions

## Why disclose all this?

Because people should know how the software they run was made, especially software that reads a game's memory. AI wrote the code, a human set the goals and tested it, and everything is open for anyone to review, improve or question.
