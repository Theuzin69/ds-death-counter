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

## Elden Ring (adicionado depois da 1.0.0)

O suporte ao Elden Ring foi feito depois, numa sessão própria do Claude Code (cada jogo novo ganhou uma sessão em paralelo). Ele trouxe uma restrição que o Dark Souls Remastered não tem: o **Easy Anti-Cheat (EAC)**.

### A decisão sobre o antitrapaça

Aberto do jeito normal pela Steam, o Elden Ring roda com o EAC. Um programa que abre o processo do jogo protegido é exatamente o que o antitrapaça vigia, mesmo que ele só leia. Como o app promete não arriscar banimento, a regra é rígida: **enquanto o EAC estiver rodando, nenhum handle para o `eldenring.exe` é aberto**, nem para saber a versão.

- O EAC é detectado sem tocar no jogo: pela lista de processos (`EasyAntiCheat*.exe`, `start_protected_game.exe`), pelo estado dos serviços `EasyAntiCheat_EOS` e `EasyAntiCheat` e pelo processo que abriu o jogo (o lançador do EAC é quem inicia o jogo).
- O jogo só é aberto depois de duas verificações, com dois segundos de intervalo, sem nenhum sinal do EAC. A verificação se repete a cada dois segundos; se o EAC aparecer, o handle é fechado na hora e o app fica no arquivo de save até o jogo fechar. Se a lista de processos não puder ser lida, o app considera que o EAC está ativo.

### Contando pelo arquivo de save

O save, `ER0000.sl2`, é um contêiner BND4 e, diferente do Dark Souls III, não é criptografado: dez slots de personagem (um MD5 mais 0x280000 bytes cada) e uma seção `USER_DATA010` com o resumo da tela de carregar (nome, nível, tempo de jogo) e o Steam id.

Dentro de um slot o jogo grava dezenas de estruturas uma depois da outra, várias de tamanho variável, então o contador de mortes não tem posição fixa. O formato documentado pelo [er-save-manager](https://github.com/Hapfel1/er-save-manager) e pelo [ER-Save-Editor](https://github.com/ClayAmore/ER-Save-Editor) coloca o contador 3 bytes dentro de um bloco de 0x1D bytes entre os dados de tutorial e as flags de evento (0x1BF99F bytes). O Claude fez o leitor encontrá-lo com uma âncora e uma prova:

- **âncora:** o cabeçalho dos dados de tutorial (`uint16 174, uint16 1, uint32 tamanho`), logo antes do bloco;
- **prova:** a partir de cada candidato, o leitor percorre o resto do slot (flags de evento, os cinco blocos de estado do mundo, posição, NetMan, clima, hora) e confere se o caminho termina exatamente no Steam id da conta. Um começo errado nunca chega lá. Só um candidato pode passar; senão o slot é marcado como não entendido, nunca adivinhado;
- **segunda opinião:** uma leitura independente desde o início do slot (tabela de itens, atributos, inventário…) tem que terminar no mesmo cabeçalho de tutorial. Quando termina, o NG+ e a mancha de sangue (o mapa onde o personagem morreu por último) também são lidos.

Isso foi conferido no save real do desenvolvedor, um arquivo da época da 1.02 com dois personagens, lido sem nenhuma alteração. Deu 400 e 324 mortes, com nomes, níveis, tempo de jogo e Steam id batendo com os resumos, e as duas leituras concordaram.

A forma como o jogo grava o arquivo foi simulada com `tests/er_save_test --simulate`. O teste copia o save para o `%TEMP%` e faz o papel do jogo na cópia:

1. grava uma morte (contagem nova, mancha de sangue nova, MD5 novo);
2. deixa um arquivo gravado pela metade (MD5 ainda não atualizado);
3. termina a gravação com uma segunda morte.

O vigia confere a data do arquivo duas vezes por segundo sem abri-lo. Ele só lê depois que o arquivo fica 0,7 s sem mudar, segura o arquivo por alguns milissegundos e confere o MD5 de cada slot, tentando de novo se pegar uma gravação pela metade. O personagem em jogo é o slot que o jogo regravou. Ele informou o personagem, a contagem e a área certos em cada passo e ignorou o arquivo pela metade.

### Lendo a memória quando o EAC está desligado

No modo offline, com o EAC desativado (ou com o Seamless Co-op), o app lê a memória como faz no Dark Souls Remastered, com tudo conferido no executável 1.02.0 (`tests/scan_test`):

- o `GameDataMan` é localizado por **cinco trechos de código independentes**. Dois foram encontrados para este projeto: a própria função do jogo que "soma no contador de mortes" (uma soma com limite em `GameDataMan+0x94`, em exe+0x250100 na 1.02) e a que "avança o tempo de jogo" (`+0xA0`, com limite em 999:59:59). O app lê os dois deslocamentos dessas instruções durante a execução. As outras três assinaturas vêm do SoulSplitter e do DSDeaths. As cinco aparecem uma única vez e apontam para o mesmo endereço;
- o `WorldChrMan` e a posição do jogador dentro dele (0x18468 na 1.02, 0x1E508 em versões mais novas) saem de uma mesma sequência de instruções. O HP vem da cadeia de módulos do personagem (+0x190 → +0 → +0x138), a mesma usada pelas verificações de "está vivo" do próprio jogo. Os deslocamentos de posição e id do mapa por versão vêm do SoulSplitter; os da 1.02 aparecem no código que os grava;
- o slot do save não é lido da memória. O personagem na memória é ligado ao slot do save pelo nome, nível e tempo de jogo, então os dois modos caem no mesmo perfil (`er|slot|nome`);
- se as assinaturas pararem de bater depois de uma atualização do jogo, o app passa a usar o arquivo de save.

### Nomes das áreas

Os ids de mapa do Elden Ring são `mAA_BB_CC_DD`. Dungeons e masmorras têm mapa próprio (`m10_00` Stormveil Castle, `m30_04` Murkwater Catacombs…), o mundo aberto é uma grade de quadrados de 256 m (`m60_XX_YY_00`) e o Reino das Sombras do DLC é `m61`. Os nomes vêm da lista de mapas do [Smithbox](https://github.com/vawser/Smithbox) (MIT) para o jogo base e dos nomes dos lugares para o DLC.

- **Granularidade:** um nome por dungeon e masmorra, e um por quadrado do mundo aberto, escrito como *Região - lugares*; os quadrados do DLC têm só a região. As estatísticas agrupam as mortes do Elden Ring por região.
- **Última graça:** o save guarda a graça como um id de entidade cujos dígitos dizem o quadrado do mapa (`1042362950` → `m60_42_36`), então o app mostra a área da graça. Os nomes das graças exigiriam os arquivos de dados criptografados do jogo, que este projeto não extrai.

### O que foi conferido sem o jogo

- Verificações estáticas no `eldenring.exe` 1.02.0 (assinaturas, deslocamentos, o código de morte e de HP) e no save real (acima).
- O app de verdade foi executado com processos substitutos chamados `eldenring.exe` e `start_protected_game.exe` (programinhas que não fazem nada), e os handles abertos pelo app foram listados com `NtQuerySystemInformation`:
  - com o lançador falso do EAC rodando, o app não tinha nenhum handle para o jogo falso;
  - sem ele, depois das duas verificações, tinha exatamente um handle, com `PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE`;
  - quando o lançador falso apareceu, o handle sumiu em até três segundos e não voltou durante aquela execução do jogo.
- Não conferido: o jogo de verdade rodando (ele não foi aberto durante o desenvolvimento), a versão atual da Steam, 1.17 (sem cópia do executável ou dos saves), e quanto tempo o jogo leva para gravar o save depois de uma morte.

## Limitações conhecidas

- Quem matou (inimigo/chefe) não é detectado automaticamente, exceto quedas.
- Em tela cheia exclusiva o overlay pode não aparecer; em janela/sem bordas ele sempre funciona.
- A interface está só em português (inglês está nos planos).
- O suporte às versões 1.03.x da Steam se apoia nos endereços publicados pelas ferramentas da comunidade citadas acima, mais a detecção de versão em tempo real. Se algo estiver estranho na sua versão, a aba *Diagnóstico* mostra o que está sendo lido; por favor, abra uma issue.
- No Elden Ring com o Easy Anti-Cheat a contagem chega alguns segundos depois (quando o jogo salva) e quedas não são detectadas; o overlay não sabe diferenciar o menu principal do jogo. A versão atual da Steam (1.17) e o tempo que o jogo leva para salvar depois de uma morte ainda precisam ser confirmados no jogo.

## Ferramentas

[Claude Code](https://www.anthropic.com/claude-code) com Claude Opus 5.5 · Visual Studio 2022 (MSVC) · [Dear ImGui](https://github.com/ocornut/imgui) · [nlohmann/json](https://github.com/nlohmann/json) · Python + [Capstone](https://www.capstone-engine.org/) (só para análise, não vai no app) · [Inno Setup](https://jrsoftware.org/isinfo.php) · GitHub Actions

## Por que contar tudo isso?

Porque as pessoas devem saber como foi feito o software que elas usam, principalmente um que lê a memória de um jogo. A IA escreveu o código, um humano definiu os objetivos e testou, e tudo está aberto para qualquer um revisar, melhorar ou questionar.
