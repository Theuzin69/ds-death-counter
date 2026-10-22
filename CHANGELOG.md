# Changelog

All notable changes to this project are listed here. The format follows [Keep a Changelog](https://keepachangelog.com/) and the project uses [Semantic Versioning](https://semver.org/).

*Todas as mudanças relevantes do projeto ficam registradas aqui (em inglês e português).*

## [Unreleased]

### New games / Jogos novos

- **Hollow Knight** (current Steam version, Unity 6): the game keeps no death counter, so the app counts each death itself while it's open and keeps the total per save slot (Steel Soul saves apart). Counts every time the Knight dies, dream and Godhome deaths included; a spike or acid hit that only costs a mask, quitting and loading a save don't count. Records the area, the area of the last bench and, in boss arenas, the boss. Read-only through the game's Mono runtime; not yet confirmed in-game. Old versions (1.4.3.2 and earlier) aren't supported. See the [user guide](docs/user-guide.md#hollow-knight).
  *Suporte ao **Hollow Knight** (versão atual da Steam, Unity 6): o jogo não guarda contador de mortes, então o próprio app conta cada morte enquanto está aberto e guarda o total por slot de save (saves Steel Soul separados). Conta toda vez que o Cavaleiro morre, inclusive em sonhos e no Godhome; um espinho ou ácido que só tira uma máscara, sair do jogo e carregar um save não contam. Registra a área, a área do último banco e, nas arenas, o chefe. Só lê a memória, pelo runtime Mono do jogo; ainda não confirmado no jogo. Versões antigas (1.4.3.2 e anteriores) não são suportadas. Veja o [guia do usuário](docs/guia-do-usuario.md#hollow-knight).*

## [1.1.0] - 2026-10-21

Three more games, automatic boss detection in Dark Souls Remastered and an OBS text source. / Mais três jogos, detecção automática de chefes no Dark Souls Remastered e uma fonte de texto para o OBS.

### New games / Jogos novos

- **Dark Souls II: Scholar of the First Sin** (64-bit, checked against the 1.02 executable): the game's own death total per character, plus name, save slot, level, NG+ cycle, play time, current map and last bonfire. The original 32-bit Dark Souls II is detected and reported as not supported.
  *Suporte ao **Dark Souls II: Scholar of the First Sin** (64 bits, conferido no executável 1.02): o total de mortes que o próprio jogo guarda por personagem, mais nome, slot do save, nível, ciclo de NG+, tempo de jogo, mapa atual e última fogueira. O Dark Souls II original de 32 bits é detectado e o app avisa que não é suportado.*
- **Sekiro: Shadows Die Twice** (1.06, the current Steam version; older versions untested): reads the game's own death counter, which only goes up on real deaths (resurrecting doesn't count). Each save slot is a character (Sekiro has no names); the list shows the area and the area of the last Sculptor's Idol, with Sekiro's bosses as cause suggestions. See the [user guide](docs/user-guide.md#sekiro).
  *Suporte ao **Sekiro: Shadows Die Twice** (1.06, a versão atual da Steam; versões anteriores não testadas): lê o contador de mortes do próprio jogo, que só sobe nas mortes de verdade (ressuscitar não conta). Cada slot de save é um personagem (o Sekiro não tem nomes); a lista mostra a área e a área do último Ídolo do Escultor, com os chefes do Sekiro nas sugestões de causa. Veja o [guia do usuário](docs/guia-do-usuario.md#sekiro).*
- **Bloodborne** (experimental), running in the Bloodborne PC Windows port v0.5 (`bbport.exe`, Bloodborne 1.09): the game's own death counter, character name, level, NG+ cycle and play time, read-only. No area or fall detection for Bloodborne yet; characters are told apart by name.
  *Suporte **experimental** ao **Bloodborne** no Bloodborne PC Windows port v0.5 (`bbport.exe`, Bloodborne 1.09): o contador de mortes do próprio jogo, nome, nível, ciclo de NG+ e tempo de jogo, só lendo a memória. Ainda sem área nem detecção de quedas no Bloodborne; os personagens são separados pelo nome.*
- Each character shows which game it belongs to; characters of different games never mix. Data from 1.0.0 keeps working unchanged (it's Dark Souls Remastered).
  *Cada personagem mostra de qual jogo ele é; personagens de jogos diferentes nunca se misturam. Os dados da 1.0.0 continuam funcionando sem mudança (são do Dark Souls Remastered).*

### Boss detection / Detecção de chefes (Dark Souls Remastered)

- A death while a boss health bar is on screen is recorded with that boss as the cause (e.g. *Chefe: Ornstein & Smough*), covering all 26 bosses that have a health bar. It reads the game's own boss bar data, so it stays read-only. A fall during a boss fight counts as a death to that boss. Not yet confirmed in-game.
  *Morrer com a barra de vida de um chefe na tela registra aquele chefe como causa (ex.: “Chefe: Ornstein & Smough”), para os 26 chefes que têm barra de vida. Lê os dados da barra do próprio jogo, então continua somente leitura. Uma queda durante a luta conta como morte para o chefe. Ainda não confirmado no jogo.*

### OBS

- OBS text file for streamers (off by default): the app writes the current character's count to a UTF-8 text file that an OBS **Text (GDI+)** source shows with **Read from file**, so the count also appears with Game Capture and Window Capture. Configurable text with `{mortes}` (count) and `{nome}` (character name), and file location (default `%APPDATA%\DSDeathCounter\obs.txt`). Setup steps are in the user guide, section 7.
  *Arquivo de texto para o OBS, para quem faz live (desligado por padrão): o app grava a contagem do personagem atual num arquivo de texto UTF-8 que uma fonte **Texto (GDI+)** do OBS mostra com **Carregar do arquivo**, então a contagem aparece também com Captura de jogo e Captura de janela. Texto configurável com `{mortes}` (contagem) e `{nome}` (nome do personagem), e local do arquivo (padrão `%APPDATA%\DSDeathCounter\obs.txt`). O passo a passo está no guia do usuário, seção 7.*

### Other / Outros

- The *Diagnóstico* tab shows the attached game, every signature found for it and, in Dark Souls Remastered, the boss health bars on screen.
  *A aba Diagnóstico mostra o jogo conectado, cada assinatura encontrada e, no Dark Souls Remastered, as barras de chefe na tela.*
- Tests: `scan_test` accepts the Dark Souls II and Sekiro executables; new `boss_cause_test`, `bb_scan_test`, `bb_fake_port` and `bb_live_test`.
  *Testes: o `scan_test` aceita os executáveis do Dark Souls II e do Sekiro; novos `boss_cause_test`, `bb_scan_test`, `bb_fake_port` e `bb_live_test`.*

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
