# Changelog

All notable changes to this project are listed here. The format follows [Keep a Changelog](https://keepachangelog.com/) and the project uses [Semantic Versioning](https://semver.org/).

*Todas as mudanças relevantes do projeto ficam registradas aqui (em inglês e português).*

## [Unreleased]

### Added / Adicionado

- **Elden Ring support.** The counter recognizes `eldenring.exe` and keeps one count and history per save slot, next to the Dark Souls Remastered characters; each character shows which game it belongs to.
  *Suporte ao Elden Ring. O contador reconhece o `eldenring.exe` e mantém uma contagem e um histórico por slot do save, junto dos personagens do Dark Souls Remastered; cada personagem mostra de qual jogo é.*
- **Easy Anti-Cheat safe.** While EAC is running (the normal Steam launch), the app never opens the game process: it reads the death counter from the save file (`ER0000.sl2`), which the game writes after every death, so the count shows up a few seconds after you die. Only when the game runs without EAC (offline launch, Seamless Co-op) does it read the game's memory, read-only, like DSR. EAC is detected from the process list, the EAC services and the game's launcher, without touching the game.
  *Seguro com o Easy Anti-Cheat. Com o EAC rodando (o jeito normal pela Steam), o app nunca abre o processo do jogo: lê o contador de mortes do arquivo de save (`ER0000.sl2`), que o jogo grava depois de cada morte, então a contagem aparece alguns segundos depois da morte. Só quando o jogo roda sem o EAC (modo offline, Seamless Co-op) ele lê a memória, somente leitura, como no DSR. O EAC é detectado pela lista de processos, pelos serviços do EAC e pelo lançador do jogo, sem tocar no jogo.*
- Elden Ring deaths record the area (legacy dungeons, dungeons and open-world regions, from the map id, or from the bloodstain in save-file mode) and the area of the last Site of Grace. *Por área* groups Elden Ring deaths by region.
  *As mortes no Elden Ring registram a área (dungeons, masmorras e regiões do mundo aberto, pelo id do mapa, ou pela mancha de sangue no modo de arquivo de save) e a área da última graça. A aba "Por área" agrupa as mortes do Elden Ring por região.*
- `tests/er_save_test` (reads a save and simulates the game writing deaths on a temporary copy); `tests/scan_test` also checks `eldenring.exe`.
  *`tests/er_save_test` (lê um save e simula o jogo gravando mortes numa cópia temporária); o `tests/scan_test` também confere o `eldenring.exe`.*

### Changed / Alterado

- Profiles store their game (`"game"`, `"dsr"` for existing data); Elden Ring keys are prefixed (`er|slot|name`), so existing Dark Souls Remastered data keeps working unchanged.
  *Os perfis guardam o jogo (`"game"`, `"dsr"` nos dados existentes); as chaves do Elden Ring têm prefixo (`er|slot|nome`), então os dados do Dark Souls Remastered continuam funcionando sem mudança.*

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
