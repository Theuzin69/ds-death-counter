# Changelog

All notable changes to this project are listed here. The format follows [Keep a Changelog](https://keepachangelog.com/) and the project uses [Semantic Versioning](https://semver.org/).

*Todas as mudanças relevantes do projeto ficam registradas aqui (em inglês e português).*

## [Unreleased]

### New games / Jogos novos

- **Dark Souls III** (checked against the 1.15.0 executable): the game's own death counter per character, plus name, save slot, level, NG+ cycle, play time, current map and last bonfire (shown by the map it belongs to), with fall detection and Dark Souls III's bosses as cause suggestions. Read-only, like the other games. The current Steam build (1.15.1 / 1.15.2) is found by the same byte signatures but hasn't been confirmed in-game yet. See the [user guide](docs/user-guide.md#dark-souls-iii).
  *Suporte ao **Dark Souls III** (conferido no executável 1.15.0): o contador de mortes que o próprio jogo guarda por personagem, mais nome, slot do save, nível, ciclo de NG+, tempo de jogo, mapa atual e última fogueira (mostrada pelo mapa dela), com detecção de quedas e os chefes do Dark Souls III nas sugestões de causa. Só leitura, como nos outros jogos. A versão atual da Steam (1.15.1 / 1.15.2) é encontrada pelas mesmas assinaturas de bytes, mas ainda não foi confirmada no jogo. Veja o [guia do usuário](docs/guia-do-usuario.md#dark-souls-iii).*
- **Elden Ring**, safe with Easy Anti-Cheat. While EAC is running (the normal Steam launch) the app never opens the game process: it reads the death counter from the save file (`ER0000.sl2`), which the game writes after every death, so the count shows up a few seconds after you die. Without EAC (offline launch, Seamless Co-op) it reads the game's memory, read-only, like the other games, with the area of each death and fall detection. EAC is detected from the process list, the EAC services and the game's launcher, without touching the game. Deaths record the area (legacy dungeons, dungeons and open-world regions) and the area of the last Site of Grace; *Por área* groups Elden Ring deaths by region. Signatures checked on the 1.02.0 executable; tested in the game. See the [user guide](docs/user-guide.md#elden-ring).
  *Suporte ao **Elden Ring**, seguro com o Easy Anti-Cheat. Com o EAC rodando (o jeito normal pela Steam), o app nunca abre o processo do jogo: lê o contador de mortes do arquivo de save (`ER0000.sl2`), que o jogo grava depois de cada morte, então a contagem aparece alguns segundos depois da morte. Sem o EAC (modo offline, Seamless Co-op) ele lê a memória do jogo, só leitura, como nos outros jogos, com a área de cada morte e detecção de quedas. O EAC é detectado pela lista de processos, pelos serviços do EAC e pelo lançador do jogo, sem tocar no jogo. As mortes registram a área (dungeons, masmorras e regiões do mundo aberto) e a área da última graça; a aba "Por área" agrupa as mortes do Elden Ring por região. Assinaturas conferidas no executável 1.02.0; testado no jogo. Veja o [guia do usuário](docs/guia-do-usuario.md#elden-ring).*
- Tests: `scan_test` accepts `DarkSoulsIII.exe` and checks all 14 signatures and the 1.15.0 values, and `eldenring.exe` (all signatures and the 1.02.0 values); new `er_save_test` reads an Elden Ring save and, with `--simulate`, plays the game's save writes on a temporary copy.
  *Testes: o `scan_test` aceita o `DarkSoulsIII.exe` e confere as 14 assinaturas e os valores da 1.15.0, e o `eldenring.exe` (todas as assinaturas e os valores da 1.02.0); o novo `er_save_test` lê um save do Elden Ring e, com `--simulate`, simula as gravações do jogo numa cópia temporária.*

### Other / Outros

- Game modules can forbid opening their game (`GameModule::CheckAccess`) and count without the process; used by Elden Ring under Easy Anti-Cheat. Deaths of games other than Dark Souls Remastered keep the full map id, shown in the area tooltip.
  *Os módulos de jogo podem proibir abrir o jogo (`GameModule::CheckAccess`) e contar sem o processo; usado pelo Elden Ring com o Easy Anti-Cheat. As mortes dos jogos além do Dark Souls Remastered guardam o id completo do mapa, mostrado na dica da área.*

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
