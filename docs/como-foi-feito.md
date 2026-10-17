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

## Bloodborne (port de PC)

O suporte ao Bloodborne é para um programa específico, o **Bloodborne PC Windows port** v0.5. Antes de tudo, o Claude descobriu o que é esse programa usando só os arquivos dele: listas de arquivos, metadados PE (importações, seções, assinaturas, manifestos), o conteúdo empacotado do launcher e os scripts Python dele, que são texto puro. Nada dele foi executado. É um carregador, o `bbport.exe`, compilado com MinGW, sem assinatura digital e sem compactação. Ele roda direto o código x86-64 de PS4 do próprio jogo (`eboot.bin`), com a parte gráfica tirada do projeto [shadPS4](https://github.com/shadps4-emu/shadPS4). O launcher é um app PyInstaller + Qt que inicia o port pelo `run.bat`.

O que importa para o contador é que o port coloca a imagem inteira do jogo num bloco só da memória, num endereço fixo na v0.5 (`0x800000000` = endereço 0 do jogo). Assim, o mesmo método do Dark Souls Remastered funciona:

- Os scripts do jogo chamam uma função Lua chamada `AddDeathCount` quando a mensagem de morte aparece. O código dela soma 1 num campo do `GameDataMan` do jogo (deslocamento `0x88` na 1.09). Uma vizinha, `AddTrueDeathCount`, incrementa o `0x84`; nenhum script a chama, então o app só mostra esse valor no *Diagnóstico*.
- O tempo de jogo (`0x94`, em milissegundos, com limite de 999:59:59 como nos outros jogos da FromSoftware), o ciclo de NG+ (`GetClearCount`, `0x68`) e o nível (`GetLocalPlayerSoulLv`, dados do jogador em `GameDataMan + 0x8`, nível em `+0x90`) vêm de outras funções, cada uma com a sua assinatura de bytes. As quatro assinaturas aparecem exatamente uma vez e apontam para o mesmo `GameDataMan`. Os patches de desempenho opcionais do port não mexem em nenhum desses bytes.
- O nome do personagem não tem assinatura própria no código. Um arquivo de save guarda o mesmo bloco de dados do jogador, e os atributos batem campo a campo com os deslocamentos usados pelo código, o que coloca o nome `0x18` bytes depois do nível (`+0xA8`). O app só usa o nome para separar personagens e o valida durante a leitura.
- Não existe um ponteiro de "jogador no mundo": o app considera que você está no jogo quando o tempo de jogo está correndo, porque ele para na tela de título.

O `tests/bb_scan_test` roda o analisador num arquivo `eboot.bin` e compara o resultado com esses valores. O `tests/bb_fake_port` é um processo substituto chamado `bbport.exe`. Ele mapeia a imagem do jogo do mesmo jeito, nunca executa nada dela e segue um roteiro curto (tela de título, um personagem, três mortes, volta à tela de título). O `tests/bb_live_test` se conecta a esse substituto como o app faz. Com ele foram conferidos a conexão, a busca alternativa da imagem em outro endereço, as mortes, o nome, o nível e o NG+, e o estado "no jogo". Ainda não foi testado numa sessão de jogo de verdade.

## Limitações conhecidas

- Quem matou (inimigo/chefe) não é detectado automaticamente, exceto quedas.
- Em tela cheia exclusiva o overlay pode não aparecer; em janela/sem bordas ele sempre funciona.
- A interface está só em português (inglês está nos planos).
- O Bloodborne (port de PC) é experimental: sem área, lamparina ou detecção de quedas, personagens separados só pelo nome, e ainda não foi confirmado numa sessão de jogo de verdade.
- O suporte às versões 1.03.x da Steam se apoia nos endereços publicados pelas ferramentas da comunidade citadas acima, mais a detecção de versão em tempo real. Se algo estiver estranho na sua versão, a aba *Diagnóstico* mostra o que está sendo lido; por favor, abra uma issue.

## Ferramentas

[Claude Code](https://www.anthropic.com/claude-code) com Claude Opus 5.5 · Visual Studio 2022 (MSVC) · [Dear ImGui](https://github.com/ocornut/imgui) · [nlohmann/json](https://github.com/nlohmann/json) · Python + [Capstone](https://www.capstone-engine.org/) (só para análise, não vai no app) · [Inno Setup](https://jrsoftware.org/isinfo.php) · GitHub Actions

## Por que contar tudo isso?

Porque as pessoas devem saber como foi feito o software que elas usam, principalmente um que lê a memória de um jogo. A IA escreveu o código, um humano definiu os objetivos e testou, e tudo está aberto para qualquer um revisar, melhorar ou questionar.
