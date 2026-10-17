# Changelog

All notable changes to this project are listed here. The format follows [Keep a Changelog](https://keepachangelog.com/) and the project uses [Semantic Versioning](https://semver.org/).

*Todas as mudanças relevantes do projeto ficam registradas aqui (em inglês e português).*

## [Unreleased]

### Added / Adicionado

- Experimental Bloodborne support for the Bloodborne PC Windows port v0.5 (`bbport.exe`, Bloodborne 1.09): reads the game's own death counter, character name, level, NG+ cycle and play time, read-only like Dark Souls Remastered. No area or fall detection for Bloodborne yet; characters are told apart by name. Profiles now store which game they belong to (`game` in `data.json`; existing ones are Dark Souls Remastered).
  *Suporte experimental ao Bloodborne no Bloodborne PC Windows port v0.5 (`bbport.exe`, Bloodborne 1.09): lê o contador de mortes do próprio jogo, o nome, o nível, o ciclo de NG+ e o tempo de jogo, só lendo a memória, como no Dark Souls Remastered. Ainda sem área nem detecção de quedas no Bloodborne; os personagens são separados pelo nome. Os perfis agora guardam de qual jogo são (`game` no `data.json`; os já existentes são do Dark Souls Remastered).*
- Tests: `bb_scan_test` (scanner on an `eboot.bin` file), `bb_fake_port` + `bb_live_test` (live reader against a stand-in process that never runs game code).
  *Testes: `bb_scan_test` (analisador num arquivo `eboot.bin`), `bb_fake_port` + `bb_live_test` (leitor ao vivo contra um processo substituto que nunca executa código do jogo).*

## [1.0.0] - 2026-10-16

First public release. / Primeira versão pública.

### Added / Adicionado

- Automatic death counting for Dark Souls Remastered by reading the game's own per-character counter (read-only memory access, no injection). Game versions 1.01 and 1.03.x are detected automatically.
  *Contagem automática de mortes lendo o contador do próprio jogo (somente leitura, sem injeção). Versões 1.01 e 1.03.x detectadas automaticamente.*
- Separate count and history for each save/character; deaths that happened while the counter was closed are added when it reopens.
  *Contagem e histórico separados por save/personagem; mortes com o contador fechado entram quando ele volta.*
- Configurable overlay on top of the game (font, size, corner, colors, outline), `Ctrl+Shift+D` hotkey and a button to make the game window borderless.
  *Overlay configurável por cima do jogo (fonte, tamanho, canto, cores, contorno), atalho `Ctrl+Shift+D` e botão para deixar a janela do jogo sem bordas.*
- Death list with date/time, area (map), last bonfire, cause and play time; automatic fall detection; editable cause/area with suggestions; stats by area and by cause; CSV export.
  *Lista de mortes com data/hora, área (mapa), última fogueira, causa e tempo de jogo; detecção automática de quedas; causa/área editáveis com sugestões; estatísticas por área e por causa; exportação CSV.*
- Editable counts, flagged in the app and shown in a different overlay color.
  *Contagem editável, sinalizada no app e com outra cor no overlay.*
- System tray mode, start with Windows, single instance, Diagnostics tab.
  *Bandeja do sistema, iniciar com o Windows, instância única, aba de Diagnóstico.*
- Per-user installer (no admin rights) and portable zip, built and attested by GitHub Actions.
  *Instalador por usuário (sem administrador) e zip portátil, compilados e atestados pelo GitHub Actions.*

[1.0.0]: https://github.com/Theuzin69/ds-death-counter/releases/tag/v1.0.0
