# Como o DS Death Counter foi feito

[English](how-it-was-made.md) · **Português (Brasil)**

> [!IMPORTANT]
> **Uso de IA.** O DS Death Counter foi feito com inteligência artificial. Praticamente todo o código-fonte, a engenharia reversa da memória do jogo, os testes, o instalador, a automação de releases e esta documentação foram produzidos pelo **Claude**, um modelo de IA da Anthropic (*Claude Opus 5.5*), trabalhando pelo **Claude Code**, o agente de programação da Anthropic. O projeto foi idealizado, dirigido e testado no jogo por **MatheusMuzzi**.

Esta página explica quem fez o quê, como o app foi construído passo a passo, como foi verificado e quais são os limites dele. O objetivo é transparência total: é um programa que lê a memória de outro programa, e você merece saber exatamente como ele surgiu. O código é aberto, então tudo pode ser revisado.

## Quem fez o quê

| | MatheusMuzzi (humano) | Claude (IA) |
|---|---|---|
| Ideia e propósito | ✔ | |
| Requisitos e prioridades | ✔ | |
| Decisões (linguagem, licença, documentação, publicação) | ✔ | propôs as opções |
| Pesquisa dos endereços de memória do jogo | | ✔ |
| Engenharia reversa do executável do jogo | | ✔ |
| Arquitetura e todo o código-fonte | | ✔ |
| Testes automáticos, instalador, automação de releases | | ✔ |
| Documentação (incluindo esta página) | revisou | ✔ |
| Testes no jogo de verdade | ✔ | |

O projeto inteiro foi feito numa única conversa no Claude Code, em português. A IA usou ferramentas no PC do desenvolvedor (compilador, Python, um desmontador de código, capturas de tela das próprias janelas) para construir e conferir o trabalho.

## A ideia

Um contador de mortes automático, para streamers ou para quem simplesmente quer saber quantas vezes, e onde, morreu mais num jogo. Começando pelo Dark Souls Remastered.

Os requisitos originais eram:

1. Abrir o app uma vez e vinculá-lo ao Dark Souls; a partir daí, as mortes são contadas sempre que o jogo estiver aberto.
2. O contador sobe sozinho toda vez que o jogador morre.
3. Se o jogo for fechado, a contagem para.
4. Quando o jogo é reaberto, a contagem continua de onde parou.
5. A contagem aparece como overlay por cima do jogo.
6. *(opcional)* Uma lista de todas as mortes com a área e o que matou o jogador (inimigo, chefe, buraco…).
7. *(opcional)* Uma contagem separada para cada save.
8. *(opcional)* A contagem pode ser editada no app; quando editada, isso aparece no app e o número muda de cor no overlay.

Mais duas restrições: um executável/instalador normal de Windows que qualquer pessoa possa usar, e **nenhum risco de ban na Steam**.

## Passo 1: onde o jogo guarda o número de mortes?

Os jogos da FromSoftware guardam um **contador de mortes por personagem** numa estrutura do jogo normalmente chamada de `GameDataMan`, e ele fica salvo no save. Ler esse contador, em vez de tentar detectar as mortes de outro jeito, resolve quase de graça os requisitos 2, 3, 4 e 7: o número é sempre exato, sobrevive ao jogo ser fechado e cada personagem tem o seu.

A comunidade de Souls documenta isso há anos. O Claude leu o código de ferramentas públicas para reunir os endereços (nenhum código delas foi copiado; endereços de memória são fatos sobre o jogo):

- [DSDeaths](https://github.com/quidrex/DSDeaths): contador de mortes em `GameDataMan + 0x98`.
- [SoulSplitter / SoulMemory](https://github.com/FrankvdStam/SoulSplitter): assinaturas de bytes que localizam `GameDataMan`, `GameMan`, `WorldChrMan`; endereços de tempo de jogo, NG+ e slot do save; diferenças entre versões.
- [DSR-Gadget](https://github.com/JKAnderson/DSR-Gadget) e [DS-Gadget](https://github.com/JKAnderson/DS-Gadget): estrutura dos dados do personagem (nome, nível), última fogueira, posição do jogador e como os endereços mudaram entre atualizações do jogo.

## Passo 2: conferindo os endereços no jogo de verdade

Em vez de confiar nesses números às cegas, o Claude escreveu pequenos scripts em Python com o desmontador [Capstone](https://www.capstone-engine.org/) para analisar o `DarkSoulsRemastered.exe` diretamente (só lendo o arquivo, sem modificá-lo):

- cada assinatura de bytes precisava aparecer em **exatamente um** lugar do código;
- assinaturas tiradas de **projetos diferentes** para a mesma estrutura tinham que levar ao **mesmo endereço**, e levaram, para `GameMan`, `GameDataMan` e `WorldChrMan`;
- a análise mostrou que o executável usado no desenvolvimento era o da versão original **1.01**, enquanto a Steam distribui a **1.03.x**, e que alguns endereços mudam entre elas. Por isso o app **detecta a versão do jogo** e, quando possível, lê os endereços específicos da versão direto do código do jogo enquanto roda, em vez de deixá-los fixos.

## Passo 3: descobrindo a área (não documentada em lugar nenhum)

Nenhuma ferramenta pública documentava onde o Dark Souls Remastered guarda o **mapa atual**. O Claude desmontou as pequenas funções ao lado do código que guarda a *última posição estável* do jogador (o ponto em chão firme que o jogo memoriza) e achou um par de funções que guarda um **id de mapa** de 4 bytes (`mAA_BB`, ex.: `m10_01` = Undead Burg / Undead Parish) logo ao lado, em `GameMan + 0xBC0` na 1.01.

Como é o mapa do último chão firme, ele também responde bem a "onde eu morri?" em quedas em buracos. O app encontra esse campo pelo padrão do código enquanto roda, então se adapta a outras versões do jogo. O teste no jogo confirmou: uma morte no Undead Burg foi registrada com a área certa.

## Passo 4: decisões de projeto

| Decisão | Por quê |
|---|---|
| **C++ / Win32** | Um executável pequeno (~1 MB), sem nada extra para instalar, que abre na hora e quase não gasta recursos rodando na bandeja. |
| **Acesso somente leitura** | O processo do jogo é aberto só com permissão de leitura. O app nunca escreve nele, não injeta DLL e não instala hooks. É a restrição de "sem risco de ban". |
| **Usar o contador do próprio jogo** | Contagem exata, que sobrevive a reinícios, conta mortes com o app fechado e é separada por personagem. |
| **Edição guardada como ajuste** | *mostrado = contagem do jogo + ajuste*. O jogo nunca é tocado e "editado" fica sempre explícito (requisito 8). |
| **Overlay como janela separada** | Uma janela transparente, que não captura cliques e fica sempre no topo, desenhada com GDI+ e que acompanha a janela do jogo. Nada é desenhado dentro do jogo. A contrapartida é que a tela cheia exclusiva pode escondê-la, por isso o app tem um botão de "janela sem bordas". |
| **Interface com Dear ImGui em Direct3D 11** | Uma interface compacta e bonita, sem frameworks pesados. |
| **Personagem = slot do save + nome** | Os dados do jogo não têm um id único de personagem. Se o tempo de jogo voltar a zero, é sinal de um personagem recriado com o mesmo nome. |
| **Causa da morte** | Quedas são detectadas pelo histórico de altura do jogador, e chefes pela barra de vida do chefe (adicionado depois da 1.0.0, veja o [passo 6](#passo-6-quem-te-matou-depois-da-100)). O jogo não guarda "qual inimigo te matou" num lugar que dê para ler de forma confiável e segura, então as outras causas o usuário escolhe nas sugestões. |
| **Dados em JSON** | Um arquivo legível em `%APPDATA%`, gravado de forma atômica para um travamento não corrompê-lo. |

## Passo 5: testes

O que a IA conferiu no PC de desenvolvimento:

- o projeto compila no MSVC **sem erros e sem avisos**;
- `tests/scan_test` roda o scanner do próprio app contra o executável do jogo no disco e compara com os endereços conferidos manualmente;
- `tests/boss_cause_test` confere a tabela barra de chefe → causa (também roda no GitHub Actions);
- `tests/overlay_test` desenha o overlay, que foi capturado na tela e inspecionado (cores normal e "editada");
- a janela principal foi desenhada com dados de exemplo e capturada para conferir o layout;
- o instalador foi instalado e desinstalado em modo silencioso para confirmar que não deixa sobras.

O que o humano conferiu no jogo de verdade:

- as mortes dos saves que já existiam foram lidas corretamente;
- uma morte nova no Undead Burg foi contada e registrada com a área certa.

A detecção de chefes (passo 6) ainda não foi testada no jogo.

Cada release é compilado pelo GitHub Actions a partir do código público, com somas de verificação e atestados de procedência (veja o [README](../README.pt-BR.md#smartscreen)).

## Dark Souls II: Scholar of the First Sin

O suporte ao Dark Souls II veio depois, numa sessão separada do Claude Code, com o mesmo método e as mesmas regras: o executável do jogo só foi lido como arquivo, nada da pasta do jogo foi executado ou extraído, e o app continua chamando só `ReadProcessMemory`.

**Qual executável.** O `DarkSoulsII.exe` disponível para o desenvolvimento é o *Scholar of the First Sin* (SotFS), 64 bits, versão de arquivo 1.0.2.0 (o patch 1.02, módulo de tamanho `0x20B6000`). A Steam vende hoje o SotFS 1.03, que não estava disponível, então o app acha tudo por assinaturas de bytes em vez de endereços fixos. O Dark Souls II original, de 2014, é um programa de 32 bits com outra organização. O app detecta um processo de 32 bits (`IsWow64Process`) e avisa que não é suportado em vez de chutar.

**Estrutura principal.** Quase tudo sai de um global só, o `GameManagerImp`. Duas assinaturas de projetos diferentes ([SoulSplitter](https://github.com/FrankvdStam/SoulSplitter) e [DS2S-META](https://github.com/Nordgaren/DS2S-META)) batem exatamente uma vez cada e levam ao mesmo endereço, `exe+0x160B8D0`. O endereço fixo do DSDeaths (`0x16148F0`) não é usado em lugar nenhum da 1.02, então deve ser o da 1.03. Isso confirma que a 1.03 mudou os globais de lugar, e é por isso que o app usa assinaturas.

**O contador de mortes.** As ferramentas públicas discordavam. O DSDeaths lê `PlayerParam + 0x104` (com `PlayerParam = [[GameManagerImp] + 0xD0] + 0x490`), e o DS2S-META chama `PlayerParam + 0x1A4` de "TotalDeaths". A função que carrega o `PlayerParam` do save explica o porquê. `+0x1A4` é um total único, limitado a 99.999.999. `+0x104` é o primeiro de um vetor de 20 contadores do mesmo tipo. Para tirar a dúvida, o Claude decifrou cópias dos saves de DS2 do próprio desenvolvedor (cópias só de leitura numa pasta temporária; os saves usam uma chave AES conhecida publicamente). Nos três personagens (463, 381 e 17 mortes) os dois campos têm o mesmo número, e o nome, as almas e a soul memory estão onde o código diz. O app conta pelo total (`+0x1A4`) e mostra o outro na aba *Diagnóstico* para comparar no jogo.

**Nome, slot e tempo de jogo.** Foram achados seguindo o código que grava os resumos dos slots do menu de carregar:

- nome em `[[GameManagerImp + 0xA8] + 0xC0] + 0x24`, classe em `+0x64`, ciclo de NG em `+0x68`, guardado como 1 na primeira jornada;
- o slot ativo em `[[GameManagerImp + 0xA8] + 0xD8] + 0x1368`;
- dez resumos de 0x1F0 bytes, com o tempo de jogo em segundos em `+0x1CC`.

O DS2 não tem um contador de tempo correndo. A própria função *GetPlayTimeSeconds* do jogo soma o tempo desde que o resumo foi gravado, medido com `QueryPerformanceCounter`, e o app faz igual: esse contador vale para o sistema inteiro, então a marca de tempo do jogo significa o mesmo nos dois processos. Os dados do menu decifrados bateram com esse formato (91h27m e 20h19m).

**Área e fogueira.**

- **Mapa atual.** O gerenciador de mapas (`GameManagerImp + 0x38`) guarda o índice do mapa atual em `+0x170`, que aponta para uma tabela de ids de mapa (`mAA_BB`, em `0xAABB0000`).
- **Nomes das áreas.** Vêm dos ids de área da lista de fogueiras do DS2S-META, então um mapa sem fogueira aparece como *Mapa mAA_BB*.
- **Última fogueira.** Fica em `[GameManagerImp + 0x70] + 0x16C`, com os nomes da mesma lista (licença MIT).

**Assinaturas conferidas.** O app usa 13 assinaturas. Cada uma tem que bater exatamente uma vez no executável 1.02, e os deslocamentos que elas contêm são lidos do código em tempo real. Se uma não for encontrada (num patch futuro, por exemplo), o app usa o valor da 1.02 e a aba *Diagnóstico* marca com `*`. O `tests/scan_test` reconhece o `DarkSoulsII.exe` e compara cada valor com os conferidos à mão. Não conferidos estaticamente: a posição do jogador (`[PlayerCtrl + 0x100] + 0x80`, do SoulSplitter, usada só na detecção de quedas) e a regra de "está no mundo" (o `PlayerCtrl` existe, como no DS2S-META). Os dois estão na lista para confirmar no jogo.
<a id="sekiro"></a>

## Adicionando o Sekiro: Shadows Die Twice

O suporte ao Sekiro veio depois, do mesmo jeito, numa sessão separada do Claude Code (outras sessões adicionavam outros jogos em paralelo). O executável disponível era o `sekiro.exe` **1.06**, a versão atual da Steam. Essa cópia já estava sem a camada de DRM da Steam, então o código dela é exatamente o que a versão 1.06 da Steam executa depois de carregada. Ela só foi lida como arquivo para análise, nunca executada.

**O contador de mortes.** O DSDeaths lê o contador do Sekiro em `GameDataMan + 0x90`. Em vez de confiar nisso, o Claude seguiu a função de script do próprio jogo, `AddDeathCount`: a tabela de funções de script do jogo liga esse nome a uma função pequena que termina num `+1` com limite em `GameDataMan + 0x90`. O jogo usa o mesmo `+1` para dois contadores que sobraram do multiplayer (`+0x80` e `+0x84`), então só o formato do código não diferencia um do outro. Em tempo real, o app segue o nome `AddDeathCount` pela tabela até o contador certo.

**Ressurreição ou morte de verdade?** O único código que chama `AddDeathCount` é o tratador de morte do jogador. Ele é registrado no evento de morte do jogador (entidade 10000), e é também quem faz as verificações da penalidade de morte e mostra o texto grande de morte na tela. Ressuscitar não passa por ele. Então o contador do jogo, e o número que o app mostra, conta **só as mortes de verdade**: morrer para valer, não cair e ressuscitar. Essa conclusão vem da leitura do código e ainda precisa ser confirmada no jogo. Daria para separar as duas coisas (HP chegar a 0 sem o contador subir é uma ressurreição), mas o app não conta ressurreições.

**Todo o resto** também foi lido do código do jogo. Cada estrutura é encontrada em tempo real por uma assinatura, e os deslocamentos são tirados das próprias instruções:

| Dado | Onde (1.06) | Como foi confirmado |
|---|---|---|
| Tempo de jogo | `GameDataMan + 0x9C` | o código do relógio soma o tempo de cada quadro ali e limita em 999:59:59 |
| Ciclo de NG+ | `GameDataMan + 0x70` | aumenta de um em um, com limite em 9999 |
| Slot do save | `GameMan + 0xAE4` | a rotina de salvar usa esse valor quando nenhum slot é informado, e recusa valores acima de 9 (10 slots) |
| No mundo, HP, posição | `WorldChrMan + 0x88` (jogador) → módulos `+0x1FF8` → módulo de dados `+0x18` (HP `+0x130`, máximo `+0x134`) e módulo de física `+0x68` (posição `+0x70`) | o `SetHp` do jogo (que limita pelo HP máximo logo depois), o código que monta os módulos (módulo 3 = dados, 13 = física) e a função que devolve a posição |
| Mapa atual | `FieldArea + 0x28` (índice do bloco) na lista de blocos do mundo em `[FieldArea + 0x18] + 0x20` (itens de 0xB0 bytes, id do mapa em `+0x8`) | o código por trás da função de script `GetCurrentMapAreaNo` |
| Último ídolo (ponto de retorno) | `GameMan + 0xB4C`, substituído por `+0xB50` enquanto este for ≥ 0 | gravado pela função de script `SetReturnPointEntityId` e ao descansar; lido pelo código de renascimento |

Os nomes dos mapas vieram de cruzar os ids dos mapas com as flags de evento dos Ídolos do Escultor, que usam a mesma numeração (`1AAB…` pertence ao mapa `mAA_0B`; por exemplo, os ídolos do Ashina Castle, `1111000x`, ficam no `m11_01`). O ponto de retorno é um id de entidade, que diz o mapa do ídolo mas não qual ídolo é. Os nomes dos ídolos ficam nos arquivos de dados do jogo, que não foram extraídos, então o app mostra a área do ídolo.

As assinaturas foram escritas do zero para este app; as ferramentas da comunidade (algumas são GPL) serviram só como fonte de fatos. Cada assinatura aparece exatamente uma vez na 1.06. O `GameDataMan` é confirmado por três trechos de código independentes, e o `GameMan`, o `WorldChrMan` e o `FieldArea` por dois cada. Se dois deles discordarem, aquela função é desligada e a aba *Diagnóstico* mostra *DIVERGENTE*, em vez de ler o lugar errado. O `tests/scan_test` reconhece o `sekiro.exe` e confere cada assinatura com os endereços achados à mão na 1.06.

O Sekiro não tem nome nem nível de personagem, então um personagem do Sekiro é identificado pelo slot do save, e as chaves dele no `data.json` começam com `sekiro|`, para nunca se misturarem com personagens do Dark Souls. O Sekiro não tem dano de queda fora dos abismos, então a heurística de queda fica desligada para ele.
## Bloodborne (port de PC)

O suporte ao Bloodborne é para um programa específico, o **Bloodborne PC Windows port** v0.5. Antes de tudo, o Claude descobriu o que é esse programa usando só os arquivos dele: listas de arquivos, metadados PE (importações, seções, assinaturas, manifestos), o conteúdo empacotado do launcher e os scripts Python dele, que são texto puro. Nada dele foi executado. É um carregador, o `bbport.exe`, compilado com MinGW, sem assinatura digital e sem compactação. Ele roda direto o código x86-64 de PS4 do próprio jogo (`eboot.bin`), com a parte gráfica tirada do projeto [shadPS4](https://github.com/shadps4-emu/shadPS4). O launcher é um app PyInstaller + Qt que inicia o port pelo `run.bat`.

O que importa para o contador é que o port coloca a imagem inteira do jogo num bloco só da memória, num endereço fixo na v0.5 (`0x800000000` = endereço 0 do jogo). Assim, o mesmo método do Dark Souls Remastered funciona:

- Os scripts do jogo chamam uma função Lua chamada `AddDeathCount` quando a mensagem de morte aparece. O código dela soma 1 num campo do `GameDataMan` do jogo (deslocamento `0x88` na 1.09). Uma vizinha, `AddTrueDeathCount`, incrementa o `0x84`; nenhum script a chama, então o app só mostra esse valor no *Diagnóstico*.
- O tempo de jogo (`0x94`, em milissegundos, com limite de 999:59:59 como nos outros jogos da FromSoftware), o ciclo de NG+ (`GetClearCount`, `0x68`) e o nível (`GetLocalPlayerSoulLv`, dados do jogador em `GameDataMan + 0x8`, nível em `+0x90`) vêm de outras funções, cada uma com a sua assinatura de bytes. As quatro assinaturas aparecem exatamente uma vez e apontam para o mesmo `GameDataMan`. Os patches de desempenho opcionais do port não mexem em nenhum desses bytes.
- O nome do personagem não tem assinatura própria no código. Um arquivo de save guarda o mesmo bloco de dados do jogador, e os atributos batem campo a campo com os deslocamentos usados pelo código, o que coloca o nome `0x18` bytes depois do nível (`+0xA8`). O app só usa o nome para separar personagens e o valida durante a leitura.
- Não existe um ponteiro de "jogador no mundo": o app considera que você está no jogo quando o tempo de jogo está correndo, porque ele para na tela de título.

O `tests/bb_scan_test` roda o analisador num arquivo `eboot.bin` e compara o resultado com esses valores. O `tests/bb_fake_port` é um processo substituto chamado `bbport.exe`. Ele mapeia a imagem do jogo do mesmo jeito, nunca executa nada dela e segue um roteiro curto (tela de título, um personagem, três mortes, volta à tela de título). O `tests/bb_live_test` se conecta a esse substituto como o app faz. Com ele foram conferidos a conexão, a busca alternativa da imagem em outro endereço, as mortes, o nome, o nível e o NG+, e o estado "no jogo". Ainda não foi testado numa sessão de jogo de verdade.
## Hollow Knight

O Hollow Knight não se parece em nada com os jogos da FromSoftware. É um jogo Unity: o `hollow_knight.exe` é um carregador pequeno, e a lógica do jogo é C# (`hollow_knight_Data/Managed/Assembly-CSharp.dll`) rodando no runtime Mono do Unity (`mono-2.0-bdwgc.dll`). A cópia instalada é a versão de 2026 da Steam (Unity 6000.0.61f1). O Claude só leu os arquivos dela, nunca rodou o jogo e não mudou nada na pasta do jogo.

**Não há contador para ler.** O código do jogo foi decompilado a partir de uma cópia do `Assembly-CSharp.dll` (com o ILSpy) para procurar um. O `PlayerData`, a classe gravada no save, tem cerca de 1700 campos e nenhum conta mortes. Então o próprio app conta as mortes e guarda o total por slot de save no `data.json` (chaves `hk|slot|Slot N`). Essa é a grande diferença para os outros jogos: mortes com o contador fechado não têm como ser conhecidas, e o usuário pode definir um número inicial à mão.

**O que é uma morte, no código do próprio jogo.**

- `HeroController.TakeDamage` tira as máscaras e, quando `PlayerData.health` chega a 0, inicia `HeroController.Die`. Esse é o único jeito de o Cavaleiro morrer. Espinhos e ácido (`DieFromHazard`) só tiram uma máscara e devolvem o Cavaleiro ao último ponto seguro, a não ser que fosse a última máscara; aí quem roda é o `Die`.
- O `Die` olha primeiro a zona do mapa: em `DREAM_WORLD` e `GODS_GLORY` (sonhos e Godhome) ele só mostra o efeito de morte e o jogo acorda o Cavaleiro, sem sombra. Em qualquer outro lugar ele liga `HeroControllerStates.dead`, transforma um save Steel Soul (`permadeathMode` 1) em morto (2) e chama `GameManager.PlayerDead`, que salva o jogo na hora e depois carrega o banco (ou a tela *PermaDeath* no Steel Soul).
- `HeroController.Respawn` desliga o `dead` e enche as máscaras.

Então o app conta uma morte quando a vida vai de mais que 0 para 0, ou quando o `dead` liga, com um save sendo jogado. Depois disso ele espera o renascimento para poder contar de novo. Um save carregado do menu (mesmo um gravado no meio da morte) só define o ponto de partida, e o menu, as telas de carregamento e sair do jogo zeram essa referência. Mortes de verdade e mortes em sonho/Godhome contam. Os campos da sombra (`shadeScene`, `geoPool`, `soulLimited`) não foram usados, porque sonhos, o Coliseu e o Steel Soul não deixam sombra do jeito normal. O `tests/hk_death_test` repete essas situações (morte normal, sonho, espinhos, Steel Soul, menu e recarregar, vida ilegível…).

**Lendo o Mono sem mexer nele.** Não há endereços fixos: os objetos ficam no heap do Mono. O app faz o que os autosplitters de speedrun (o autosplitter de Hollow Knight do LiveSplit e a biblioteca `asr`) fazem, escrito do zero:

1. Achar o `mono-2.0-bdwgc.dll` no processo do jogo e ler as funções exportadas dele. Os offsets das estruturas internas do Mono não ficam fixos no código: o app lê esses valores de pequenas funções exportadas que os usam, como `mono_class_get_name` (`mov rax,[rcx+0x48]; ret`), `mono_class_get_fields`, `mono_vtable_get_static_field_data`, `mono_class_vtable` e `mono_image_init` (que monta o cache de classes de cada assembly). A lista global de assemblies carregados vem de `mono_assembly_foreach`. O `tests/hk_mono_test` roda isso no arquivo da DLL e compara os 16 valores com os encontrados à mão no disassembly (Capstone).
2. Percorrer os assemblies carregados até o `Assembly-CSharp`, e o cache de classes dele até as classes `GameManager`, `PlayerData`, `HeroController`, `HeroControllerStates` e `SceneManager`.
3. Achar cada campo pelo nome, para que atualizações do jogo que mudam os campos de lugar não atrapalhem: `GameManager._instance` (estático), `playerData`, `gameState`, `sceneName`, `profileID` (slot do save, 1 a 4) e o herói; `PlayerData.health`, `permadeathMode`, `respawnScene` e as flags de "chefe derrotado"; `HeroControllerStates.dead`; o `mapZone` da cena. O `UnityEngine.Object.m_CachedPtr` diz se o objeto do herói ainda existe.

A área é a zona do mapa (o mesmo valor que o `Die` confere), com os nomes das áreas do jogo. A causa é o chefe da arena quando a cena é uma arena de chefe conhecida e a flag de derrota desse chefe ainda está desligada; as cenas de chefes de sonho, as provações do Coliseu e as cenas de chefe do Godhome sempre contam como aquela luta. Os nomes das cenas foram conferidos com a lista de cenas do jogo, e as flags com os campos do `PlayerData`. O banco vem do `PlayerData.respawnScene`, mostrado como a área dele.

Tudo isso é análise estática mais testes unitários; ainda falta confirmar no jogo.

## Passo 6: quem te matou? (depois da 1.0.0)

O requisito 6 também pedia *o que* matou o jogador, e a versão 1.0.0 só reconhecia quedas. A pesquisa seguinte voltou ao executável atrás de algum lugar onde o jogo guarde quem matou e que dê para ler com segurança.

**Inimigos comuns: não dá para ler de forma confiável.** O jogo aumenta o contador de mortes a partir do script de eventos (uma função chamada `AddDeathCount`), que não sabe nada sobre o atacante. O código que aplica dano é protegido pela ofuscação anti-adulteração do jogo, e nenhum campo guardado de "último atacante" apareceu: os golpes circulam como mensagens de evento passageiras (usadas pela condição "atingido por" dos scripts de evento) e por um conjunto de registros de golpes recentes cujo significado não dá para confirmar sem conectar um depurador ao jogo rodando, o que a promessa de somente leitura não permite. Chutar "o inimigo mais próximo" erraria com arqueiros, grupos e veneno, então o app não chuta.

**Chefes: dá para ler, e com exatidão.** A barra de vida do chefe é outra história. Os scripts de evento mostram a barra com a instrução 2003:11 (*mostrar barra de vida do chefe*: liga/desliga, personagem, slot, id do nome), e o jogo a guarda na estrutura `MenuMan`: dois slots (duas barras, como em Ornstein & Smough ou nas Bell Gargoyles) de 0x14 bytes em `MenuMan + 0x1050` na 1.01, cada um começando pelo id do nome do chefe e pelo identificador (handle) do personagem dele. A barra é apagada quando o chefe morre (o script a desliga) e quando os scripts de evento reiniciam, que é o que acontece no carregamento depois de uma morte. Então "havia uma barra na tela no último instante em que o jogador estava vivo" quer dizer "morreu naquela luta", não importa quem deu o último golpe.

Como foi conferido:

- A estrutura foi achada seguindo a função de script do próprio jogo `SetBossGauge` até a implementação, e confirmada pela função que apaga as barras, pelo construtor que as inicializa e pelo nome da classe (RTTI) do objeto que as guarda, `NS_FRPG::MenuMan`.
- Enquanto roda, o app lê o endereço do `MenuMan` e a posição da barra direto de duas funções pequenas do jogo (uma apaga as barras, a outra devolve um slot). Se as duas forem encontradas, precisam concordar; se discordarem, ou se nenhuma for encontrada, a detecção de chefe se desliga em vez de chutar. O `tests/scan_test` compara o resultado com os valores da 1.01 conferidos manualmente.
- Os ids de nome vêm dos arquivos do próprio jogo: todas as instruções 2003:11 de `event/*.emevd` foram decodificadas (28 ids diferentes: 27 barras com nome, cobrindo as 26 lutas contra chefes, mais um id em New Londo que não tem nome nos arquivos do jogo) e cruzadas com a tabela de nomes de NPC em `msg/ENGLISH/item.msgbnd`. Cada chefe é registrado com o mesmo texto das sugestões de causa (*Dragon Slayer Ornstein* e *Executioner Smough* viram *Chefe: Ornstein & Smough*), para as estatísticas "por causa" juntarem causas automáticas e escolhidas à mão. O `tests/boss_cause_test` confere essa tabela.

Luta contra chefe tem prioridade sobre a detecção de queda: cair da ponte do Taurus Demon ou nos buracos do Bed of Chaos conta como morte para aquele chefe. Chefes sem barra de vida (como o Hellkite Dragon na ponte do Undead Burg) não são detectados.

Tudo isso vem da análise estática do executável da 1.01. Duas coisas ainda precisam do jogo de verdade: confirmar que a barra ainda está ativa no último instante em que o jogador está vivo, e que os mesmos padrões de código existem nas versões 1.03.x. A aba *Diagnóstico* mostra os valores das barras de chefe ao vivo para facilitar as duas conferências.

## Limitações conhecidas

- Inimigos comuns não são detectados automaticamente; chefes com barra de vida (Dark Souls Remastered) e quedas (jogos Dark Souls), sim. Chefes sem barra de vida (como o Hellkite Dragon) também não.
- Em tela cheia exclusiva o overlay pode não aparecer; em janela/sem bordas ele sempre funciona.
- A interface está só em português (inglês está nos planos).
- Dark Souls II: só o Scholar of the First Sin (64 bits) é suportado; a versão atual da Steam (1.03) é encontrada pelas assinaturas, mas ainda não foi confirmada no jogo.
- O Bloodborne (port de PC) é experimental: sem área, lamparina ou detecção de quedas, personagens separados só pelo nome, e ainda não foi confirmado numa sessão de jogo de verdade.
- O suporte às versões 1.03.x da Steam se apoia nos endereços publicados pelas ferramentas da comunidade citadas acima, mais a detecção de versão em tempo real. Se algo estiver estranho na sua versão, a aba *Diagnóstico* mostra o que está sendo lido; por favor, abra uma issue.
- O Hollow Knight não guarda contador de mortes, então o app só conta as mortes que acontecem com ele aberto. Foi conferido no código do jogo e no runtime Mono (versão de 2026 da Steam), mas ainda não numa sessão de jogo de verdade; a versão antiga 1.4.3.2 (com um runtime Mono mais antigo) não é suportada.
- O Sekiro foi conferido só no executável 1.06; versões anteriores não foram testadas. Num primeiro teste no jogo, as mortes, o slot do save, o tempo de jogo, a área e o ídolo foram lidos corretamente; a posição do jogador veio errada (só afeta as coordenadas mostradas em cada morte), e ainda falta confirmar que ressuscitar não conta. Os nomes dos ídolos não são conhecidos, só a área do ídolo.

## Ferramentas

[Claude Code](https://www.anthropic.com/claude-code) com Claude Opus 5.5 · Visual Studio 2022 (MSVC) · [Dear ImGui](https://github.com/ocornut/imgui) · [nlohmann/json](https://github.com/nlohmann/json) · Python + [Capstone](https://www.capstone-engine.org/) (só para análise, não vai no app) · [Inno Setup](https://jrsoftware.org/isinfo.php) · GitHub Actions

## Por que contar tudo isso?

Porque as pessoas devem saber como foi feito o software que elas usam, principalmente um que lê a memória de um jogo. A IA escreveu o código, um humano definiu os objetivos e testou, e tudo está aberto para qualquer um revisar, melhorar ou questionar.
