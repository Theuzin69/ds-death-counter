# Changelog

All notable changes to this project are listed here. The format follows [Keep a Changelog](https://keepachangelog.com/) and the project uses [Semantic Versioning](https://semver.org/).

*Todas as mudanças relevantes do projeto ficam registradas aqui (em inglês e português).*

## [1.1.0] - Unreleased

### Added / Adicionado

- OBS text file for streamers (off by default): the app writes the current character's count to a UTF-8 text file that an OBS **Text (GDI+)** source shows with **Read from file**, so the count appears with Game Capture and Window Capture too. Configurable text with `{mortes}` (count) and `{nome}` (character name), and file location (default `%APPDATA%\DSDeathCounter\obs.txt`). The file is only rewritten when the text changes. Setup steps are in the user guide, section 7.
  *Arquivo de texto para o OBS, para quem faz live (desligado por padrão): o app grava a contagem do personagem atual num arquivo de texto UTF-8 que uma fonte **Texto (GDI+)** do OBS mostra com **Carregar do arquivo**, então a contagem aparece também com Captura de jogo e Captura de janela. Texto configurável com `{mortes}` (contagem) e `{nome}` (nome do personagem), e local do arquivo (padrão `%APPDATA%\DSDeathCounter\obs.txt`). O arquivo só é regravado quando o texto muda. O passo a passo está no guia do usuário, seção 7.*

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

[1.1.0]: https://github.com/Theuzin69/ds-death-counter/releases/tag/v1.1.0
[1.0.0]: https://github.com/Theuzin69/ds-death-counter/releases/tag/v1.0.0
