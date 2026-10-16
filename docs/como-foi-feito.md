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

- Inimigos comuns não são detectados automaticamente; chefes com barra de vida e quedas, sim. Chefes sem barra de vida (como o Hellkite Dragon) também não.
- Em tela cheia exclusiva o overlay pode não aparecer; em janela/sem bordas ele sempre funciona.
- A interface está só em português (inglês está nos planos).
- O suporte às versões 1.03.x da Steam se apoia nos endereços publicados pelas ferramentas da comunidade citadas acima, mais a detecção de versão em tempo real. Se algo estiver estranho na sua versão, a aba *Diagnóstico* mostra o que está sendo lido; por favor, abra uma issue.

## Ferramentas

[Claude Code](https://www.anthropic.com/claude-code) com Claude Opus 5.5 · Visual Studio 2022 (MSVC) · [Dear ImGui](https://github.com/ocornut/imgui) · [nlohmann/json](https://github.com/nlohmann/json) · Python + [Capstone](https://www.capstone-engine.org/) (só para análise, não vai no app) · [Inno Setup](https://jrsoftware.org/isinfo.php) · GitHub Actions

## Por que contar tudo isso?

Porque as pessoas devem saber como foi feito o software que elas usam, principalmente um que lê a memória de um jogo. A IA escreveu o código, um humano definiu os objetivos e testou, e tudo está aberto para qualquer um revisar, melhorar ou questionar.
