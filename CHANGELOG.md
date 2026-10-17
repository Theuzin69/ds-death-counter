# Changelog

All notable changes to this project are listed here. The format follows [Keep a Changelog](https://keepachangelog.com/) and the project uses [Semantic Versioning](https://semver.org/).

*Todas as mudanças relevantes do projeto ficam registradas aqui (em inglês e português).*

## [Unreleased]

### Added / Adicionado

- **Sekiro: Shadows Die Twice** support (1.06, the current Steam version; older versions untested). The counter recognizes `sekiro.exe` by itself and reads the game's own death counter, which only goes up on real deaths: resurrecting doesn't count. Each save slot is a character (Sekiro has no names); the list shows the area and the area of the last Sculptor's Idol, with Sekiro's bosses as cause suggestions. See the [user guide](docs/user-guide.md#sekiro).
  *Suporte ao **Sekiro: Shadows Die Twice** (1.06, a versão atual da Steam; versões anteriores não testadas). O contador reconhece o `sekiro.exe` sozinho e lê o contador de mortes do próprio jogo, que só sobe nas mortes de verdade: ressuscitar não conta. Cada slot de save é um personagem (o Sekiro não tem nomes); a lista mostra a área e a área do último Ídolo do Escultor, com os chefes do Sekiro nas sugestões de causa. Veja o [guia do usuário](docs/guia-do-usuario.md#sekiro).*
- The character list shows which game each character belongs to (*DSR* / *Sekiro*). Existing data is kept as Dark Souls Remastered.
  *A lista de personagens mostra de qual jogo é cada personagem (DSR / Sekiro). Os dados existentes continuam como Dark Souls Remastered.*
- `tests/scan_test` also accepts `sekiro.exe` and checks every Sekiro signature.
  *O `tests/scan_test` também aceita o `sekiro.exe` e confere todas as assinaturas do Sekiro.*

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
