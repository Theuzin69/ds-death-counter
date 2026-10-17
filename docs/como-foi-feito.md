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
| **Causa da morte** | Quedas são detectadas pelo histórico de altura do jogador. O jogo não guarda "quem te matou" num lugar que dê para ler de forma confiável e segura, então as outras causas o usuário escolhe nas sugestões. |
| **Dados em JSON** | Um arquivo legível em `%APPDATA%`, gravado de forma atômica para um travamento não corrompê-lo. |

## Passo 5: testes

O que a IA conferiu no PC de desenvolvimento:

- o projeto compila no MSVC **sem erros e sem avisos**;
- `tests/scan_test` roda o scanner do próprio app contra o executável do jogo no disco e compara com os endereços conferidos manualmente;
- `tests/overlay_test` desenha o overlay, que foi capturado na tela e inspecionado (cores normal e "editada");
- a janela principal foi desenhada com dados de exemplo e capturada para conferir o layout;
- o instalador foi instalado e desinstalado em modo silencioso para confirmar que não deixa sobras.

O que o humano conferiu no jogo de verdade:

- as mortes dos saves que já existiam foram lidas corretamente;
- uma morte nova no Undead Burg foi contada e registrada com a área certa.

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

## Limitações conhecidas

- Quem matou (inimigo/chefe) não é detectado automaticamente, exceto quedas.
- Em tela cheia exclusiva o overlay pode não aparecer; em janela/sem bordas ele sempre funciona.
- A interface está só em português (inglês está nos planos).
- Dark Souls II: só o Scholar of the First Sin (64 bits) é suportado; a versão atual da Steam (1.03) é encontrada pelas assinaturas, mas ainda não foi confirmada no jogo.
- O suporte às versões 1.03.x da Steam se apoia nos endereços publicados pelas ferramentas da comunidade citadas acima, mais a detecção de versão em tempo real. Se algo estiver estranho na sua versão, a aba *Diagnóstico* mostra o que está sendo lido; por favor, abra uma issue.

## Ferramentas

[Claude Code](https://www.anthropic.com/claude-code) com Claude Opus 5.5 · Visual Studio 2022 (MSVC) · [Dear ImGui](https://github.com/ocornut/imgui) · [nlohmann/json](https://github.com/nlohmann/json) · Python + [Capstone](https://www.capstone-engine.org/) (só para análise, não vai no app) · [Inno Setup](https://jrsoftware.org/isinfo.php) · GitHub Actions

## Por que contar tudo isso?

Porque as pessoas devem saber como foi feito o software que elas usam, principalmente um que lê a memória de um jogo. A IA escreveu o código, um humano definiu os objetivos e testou, e tudo está aberto para qualquer um revisar, melhorar ou questionar.
