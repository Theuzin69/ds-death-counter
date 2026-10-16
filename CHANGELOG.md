# Changelog

All notable changes to this project are listed here. The format follows [Keep a Changelog](https://keepachangelog.com/) and the project uses [Semantic Versioning](https://semver.org/).

*Todas as mudanças relevantes do projeto ficam registradas aqui (em inglês e português).*

## [Unreleased]

### Added / Adicionado

- Automatic boss detection: a death while a boss health bar is on screen is recorded with that boss as the cause (e.g. *Chefe: Ornstein & Smough*), covering all 26 bosses that have a health bar. It reads the game's own boss bar data, located from the game's code for each version, so it stays read-only. A fall during a boss fight counts as a death to that boss. Not yet confirmed in-game.
  *Detecção automática de chefe: morrer com a barra de vida de um chefe na tela registra aquele chefe como causa (ex.: “Chefe: Ornstein & Smough”), para os 26 chefes que têm barra de vida. Lê os dados da barra do próprio jogo, localizados a partir do código de cada versão, então continua somente leitura. Uma queda durante a luta conta como morte para o chefe. Ainda não confirmado no jogo.*
- The *Diagnóstico* tab shows the boss health bars currently on screen.
  *A aba Diagnóstico mostra as barras de chefe que estão na tela.*

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

[Unreleased]: https://github.com/Theuzin69/ds-death-counter/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/Theuzin69/ds-death-counter/releases/tag/v1.0.0
