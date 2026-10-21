# Changelog

All notable changes to this project are listed here. The format follows [Keep a Changelog](https://keepachangelog.com/) and the project uses [Semantic Versioning](https://semver.org/).

*Todas as mudanças relevantes do projeto ficam registradas aqui (em inglês e português).*

## [Unreleased]

### Added / Adicionado

- **Dark Souls III support** (`DarkSoulsIII.exe`, verified on 1.15.0): automatic death count from the game's own per-character counter, separate count and history per save/character, current area (map), last bonfire (shown by map), play time, fall detection and DS3 boss suggestions for the cause. Same read-only access as for Dark Souls Remastered. The current Steam build (1.15.1 / 1.15.2) still needs to be confirmed in-game.
  *Suporte ao Dark Souls III (`DarkSoulsIII.exe`, conferido na 1.15.0): contagem automática de mortes pelo contador do próprio jogo, contagem e histórico separados por save/personagem, área atual (mapa), última fogueira (mostrada pelo mapa), tempo de jogo, detecção de quedas e sugestões de causa com os chefes do DS3. Mesmo acesso somente leitura do Dark Souls Remastered. A versão atual da Steam (1.15.1 / 1.15.2) ainda precisa ser confirmada no jogo.*
- Each character shows which game it belongs to (*DSR* / *DS3*), and the Diagnostics tab shows the game and how many code signatures were found.
  *Cada personagem mostra de qual jogo é (*DSR* / *DS3*), e a aba Diagnóstico mostra o jogo e quantas assinaturas do código foram encontradas.*

### Changed / Alterado

- Saved data gets a `game` field per character (`"dsr"` for existing data, which keeps working unchanged); Dark Souls III characters are stored under keys starting with `ds3|`.
  *Os dados salvos ganham um campo `game` por personagem (`"dsr"` para os dados que já existiam, que continuam funcionando sem mudança); personagens do Dark Souls III ficam em chaves que começam com `ds3|`.*

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
