# Contributing

**English** · [Português](#contribuindo)

Thanks for your interest! Bug reports, game-version reports, translations and code are all welcome.

## Reporting a bug

Use the [bug report form](https://github.com/Theuzin69/ds-death-counter/issues/new/choose). The most useful thing you can include is the text from the app's **Diagnóstico** tab (click *Copiar*): it shows the game version, the addresses found and the values being read.

## Building

Requirements: Windows 10/11 x64 and Visual Studio 2019/2022 with the *Desktop development with C++* workload (or the Build Tools).

```bat
build.bat                 :: -> build\DSDeathCounter.exe
tests\build_tests.bat     :: -> build\tests\scan_test.exe, overlay_test.exe, bb_*.exe
```

- `build\tests\scan_test.exe "C:\path\to\DarkSoulsRemastered.exe"` (or `DarkSoulsII.exe`) runs the app's signature scanner against a game executable on disk; the game is picked from the file name. Use it to check a game version without launching it.
- `build\tests\scan_test.exe "C:\path\to\DarkSoulsRemastered.exe"` runs the app's signature scanner against a game executable on disk. Use it to check a game version without launching it. It also takes `sekiro.exe` (the game is picked by the file name); game-specific code lives in `src/games/`.
tests\build_tests.bat     :: -> build\tests\scan_test.exe, boss_cause_test.exe, overlay_test.exe
```

- `build\tests\scan_test.exe "C:\path\to\DarkSoulsRemastered.exe"` runs the app's signature scanner against a game executable on disk. Use it to check a game version without launching it.
- `build\tests\boss_cause_test.exe` checks the boss health bar → cause of death table (also run by CI).
- `build\tests\overlay_test.exe` shows the overlay for six seconds.
- `build\tests\bb_scan_test.exe "C:\path\to\eboot.bin"` runs the Bloodborne scanner against the `eboot.bin` file the Bloodborne PC port loads.
- `build\tests\fake_port\bbport.exe "C:\path\to\eboot.bin"` is a stand-in for the port (it maps the game image and never runs it); run `build\tests\bb_live_test.exe` next to it to watch the live reader.

Installer: install [Inno Setup 6](https://jrsoftware.org/isinfo.php), then run `ISCC.exe installer\DSDeathCounter.iss` (output in `dist\`).

## Project layout

```
src/game_reader.*    read-only access to the game process and the byte-signature scanner
src/games/*          other games (bloodborne.*: Bloodborne PC port)
src/death_tracker.*  turns game snapshots into per-character counts and death records
src/data_store.*     settings and history (JSON, %APPDATA%\DSDeathCounter\data.json)
src/overlay.*        click-through layered window drawn over the game (GDI+)
src/app*.cpp         main window (Win32 + Direct3D 11 + Dear ImGui), tray, hotkey
src/ds1_data.*       map names, bonfire names, cause suggestions
src/games/           the other games: registry (games.*), GameModule interface, one module per game (ds2.*, ds2_data.cpp)
src/platform.*       start with Windows, Steam lookup, game window helpers, dialogs
src/version.h        the version number (single source)
installer/           Inno Setup script
tests/               small test programs
tools/make_icon.py   regenerates res/app.ico
third_party/         Dear ImGui and nlohmann/json (vendored, MIT)
```

## Ground rules

- **Read-only, always.** The app must never write to the game process, inject code or install hooks. That's what keeps it ban-safe. Pull requests that change this won't be accepted.
- Match the style of the surrounding code (C++20, MSVC, no warnings).
- User-facing text is in Brazilian Portuguese for now. If you add strings, keep them easy to translate later.

## Publishing a release (maintainers)

1. Update `src/version.h` (`DSDC_VERSION_*` and `DSDC_VERSION_STRING`).
2. Add a section for the new version to `CHANGELOG.md` (`## [x.y.z] - date`).
3. Commit, then tag and push:
   ```bash
   git tag v1.2.3
   git push origin main v1.2.3
   ```
4. GitHub Actions builds the app and the installer, creates the portable zip and `SHA256SUMS.txt`, attests the build provenance and publishes the release with the changelog section as notes. Tags with a suffix (e.g. `v1.3.0-beta.1`) become pre-releases. The workflow fails if the tag doesn't match `src/version.h`.

---

## Contribuindo

Obrigado pelo interesse! Relatos de bug, relatos de versão do jogo, traduções e código são bem-vindos.

- **Bug:** use o [formulário de bug](https://github.com/Theuzin69/ds-death-counter/issues/new/choose) e cole o texto da aba **Diagnóstico** (botão *Copiar*).
- **Compilar:** Visual Studio 2019/2022 com *Desenvolvimento para desktop com C++*; rode `build.bat`. Testes em `tests\build_tests.bat`.
- **Regra principal:** o app é **somente leitura**. Nada de escrever na memória do jogo, injetar código ou instalar hooks; é isso que evita ban.
- **Publicar versão:** atualize `src/version.h` e o `CHANGELOG.md`, crie a tag `vX.Y.Z` e envie (`git push origin main vX.Y.Z`). O GitHub Actions compila e publica o release sozinho.
