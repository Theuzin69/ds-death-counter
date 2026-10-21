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
| **Cause of death** | Falls are detected from the player's height history. The game doesn't keep "who killed you" anywhere that can be read reliably and safely, so other causes are picked by the user from suggestions. |
| **Data in JSON** | One readable file in `%APPDATA%`, written atomically so a crash can't corrupt it. |

## Step 5: testing

What the AI checked on the development PC:

- the project compiles with MSVC with **no errors and no warnings**;
- `tests/scan_test` runs the app's own scanner against the game executable on disk and compares the result with the addresses verified by hand;
- `tests/overlay_test` draws the overlay, which was captured on screen and inspected (normal and "edited" colors);
- the main window was rendered with sample data and captured to check the layout;
- the installer was installed and uninstalled silently to confirm it leaves nothing behind.

What the human checked in the real game:

- the deaths of existing saves were picked up correctly;
- a new death in Undead Burg was counted and recorded with the correct area.

Every release is built by GitHub Actions from the public code, with checksums and build provenance attestations (see the [README](../README.md#smartscreen)).

## Elden Ring (added after 1.0.0)

Elden Ring support was built later, in its own Claude Code session (each new game got a parallel session). It came with a constraint Dark Souls Remastered doesn't have: **Easy Anti-Cheat (EAC)**.

### The anti-cheat decision

Launched normally through Steam, Elden Ring runs under EAC. A program that opens the protected game process is exactly what anti-cheat watches for, even when it only reads. Since the app promises no ban risk, the rule is strict: **while EAC is running, no handle to `eldenring.exe` is ever opened**, not even to query its version.

- EAC is detected without touching the game: the process list (`EasyAntiCheat*.exe`, `start_protected_game.exe`), the state of the `EasyAntiCheat_EOS` and `EasyAntiCheat` services, and the game's parent process (EAC's launcher starts the game).
- The game is opened only after two checks, two seconds apart, find no sign of EAC. The check repeats every two seconds; if EAC appears, the handle is closed at once and the app stays on the save file for the rest of that run of the game. If the process list can't be read, it assumes EAC.

### Counting from the save file

The save, `ER0000.sl2`, is a BND4 container and, unlike Dark Souls III's, it isn't encrypted: ten character slots (an MD5 plus 0x280000 bytes each) and a `USER_DATA010` section with the load screen summaries (name, level, play time) and the Steam id.

Inside a slot the game writes dozens of structures one after another, several of variable size, so the death counter has no fixed offset. The save layout documented by [er-save-manager](https://github.com/Hapfel1/er-save-manager) and [ER-Save-Editor](https://github.com/ClayAmore/ER-Save-Editor) puts it 3 bytes into a 0x1D-byte block between the tutorial data and the event flags (0x1BF99F bytes). Claude made the reader find it with an anchor and a proof:

- **anchor:** the tutorial data header (`uint16 174, uint16 1, uint32 size`) right before the block;
- **proof:** from each candidate, walk the rest of the slot (event flags, the five world state blocks, position, NetMan, weather, time) and check that the walk ends exactly on the account's Steam id. A wrong start never lands there. Exactly one candidate must pass; otherwise the slot is reported as not understood, never guessed;
- **second opinion:** an independent parse from the start of the slot (item table, stats, inventory…) must end on the same tutorial header. When it does, NG+ and the bloodstain (the map where the character last died) are read too.

This was checked on the developer's real save, a 1.02-era file with two characters, read without modifying it. It gave 400 and 324 deaths, with names, levels, play time and Steam id matching the summaries, and both parses agreed.

How the game writes the file was simulated with `tests/er_save_test --simulate`. The test copies the save to `%TEMP%` and acts as the game on the copy:

1. it writes a death (new count, new bloodstain, new MD5);
2. it leaves a half-written file (MD5 not updated yet);
3. it finishes the write with a second death.

The watcher checks the file's timestamp twice a second without opening it. It reads only after the file has been stable for 0.7 s, holds the file for a few milliseconds and checks every slot's MD5, retrying a torn read. The character being played is the slot the game rewrote. It reported the right character, count and area at every step and ignored the half-written file.

### Reading memory when EAC is off

Offline, with EAC disabled (or with Seamless Co-op), the app reads memory like it does for Dark Souls Remastered, verified statically on the 1.02.0 executable (`tests/scan_test`):

- `GameDataMan` is resolved by **five independent code sites**. Two were found for this project: the game's own "add to the death counter" function (a saturating add to `GameDataMan+0x94`, at exe+0x250100 in 1.02) and the "advance play time" function (`+0xA0`, capped at 999:59:59). The app reads both offsets from those instructions at runtime. The other three signatures come from SoulSplitter and DSDeaths. All five match exactly once and point to the same address.
- `WorldChrMan` and the player's offset inside it (0x18468 in 1.02, 0x1E508 in later patches) come from one instruction sequence. HP comes from the character module chain (+0x190 → +0 → +0x138), the same one the game's own "is alive" checks use. The position and map id offsets per version come from SoulSplitter; the 1.02 values are visible in the code that sets them.
- The save slot isn't read from memory. The character in memory is matched with its save slot by name, level and play time, so both modes land on the same profile (`er|slot|name`).
- If the signatures stop matching after a game update, the app falls back to the save file.

### Area names

Elden Ring map ids are `mAA_BB_CC_DD`. Legacy dungeons and dungeons have their own map (`m10_00` Stormveil Castle, `m30_04` Murkwater Catacombs…), the open world is a grid of 256 m tiles (`m60_XX_YY_00`) and the DLC's Realm of Shadow is `m61`. Names come from the [Smithbox](https://github.com/vawser/Smithbox) map id list (MIT) for the base game and from the place names for the DLC.

- **Granularity:** one name per legacy dungeon and dungeon, and one per open world tile, written as *Region - landmarks*; DLC tiles get their region only. The statistics group Elden Ring deaths by region.
- **Last Site of Grace:** the save stores it as an entity id whose digits encode its map tile (`1042362950` → `m60_42_36`), so the app shows the grace's area. Grace names would need the game's encrypted data archives, which this project doesn't extract.

### What was verified without the game

- Static checks on `eldenring.exe` 1.02.0 (signatures, offsets, the death and HP code) and on the real save (above).
- The real app was run against stand-in processes named `eldenring.exe` and `start_protected_game.exe` (tiny programs that do nothing), and the app's open handles were listed with `NtQuerySystemInformation`:
  - with the fake EAC launcher running, the app had no handle to the fake game;
  - without it, after the two checks, it had exactly one handle, with `PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE`;
  - when the fake launcher appeared, the handle was gone within three seconds and didn't come back for that run of the game.
- Not verified: the real game running (it wasn't launched during development), the current Steam version 1.17 (no copy of its executable or saves), and how quickly the game writes the save after a death.

## Known limitations

- The killer (enemy/boss) isn't detected automatically, except falls.
- Exclusive fullscreen may hide the overlay; windowed/borderless always works.
- The interface is in Portuguese only (English is planned).
- Support for the 1.03.x Steam builds relies on offsets published by the community tools above, plus the runtime version detection. If something is off on your version, the *Diagnóstico* tab shows what's being read; please open an issue.
- Elden Ring with Easy Anti-Cheat counts a few seconds late (after the game saves) and records no falls; the overlay can't tell the title screen from gameplay. The current Steam version (1.17) and the timing of the game's save after a death still need to be confirmed in the game.

## Tools

[Claude Code](https://www.anthropic.com/claude-code) with Claude Opus 5.5 · Visual Studio 2022 (MSVC) · [Dear ImGui](https://github.com/ocornut/imgui) · [nlohmann/json](https://github.com/nlohmann/json) · Python + [Capstone](https://www.capstone-engine.org/) (analysis only, not shipped) · [Inno Setup](https://jrsoftware.org/isinfo.php) · GitHub Actions

## Why disclose all this?

Because people should know how the software they run was made, especially software that reads a game's memory. AI wrote the code, a human set the goals and tested it, and everything is open for anyone to review, improve or question.
