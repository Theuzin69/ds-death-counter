# Guia do usuário

[English](user-guide.md) · **Português (Brasil)**

- [1. Instalação](#1-instalação)
- [2. Primeira vez](#2-primeira-vez)
- [3. Enquanto você joga](#3-enquanto-você-joga)
- [Elden Ring](#elden-ring)
- [4. Personagens e contagens](#4-personagens-e-contagens)
- [5. A lista de mortes](#5-a-lista-de-mortes)
- [6. Configurações](#6-configurações)
- [7. Lives (OBS)](#7-lives-obs)
- [8. Seus dados](#8-seus-dados)
- [9. Problemas comuns](#9-problemas-comuns)
- [10. Desinstalar](#10-desinstalar)

---

## 1. Instalação

Baixe na [página de Releases](https://github.com/Theuzin69/ds-death-counter/releases/latest):

- **Instalador** (`DSDeathCounter-Setup-x.y.z.exe`, recomendado): instala em `%LOCALAPPDATA%\Programs\DSDeathCounter` só para o seu usuário (sem pedir administrador), cria atalho no menu Iniciar (e, se quiser, na área de trabalho) e oferece *Iniciar com o Windows*.
- **Portátil** (`DSDeathCounter-x.y.z-portable.zip`): descompacte onde quiser e abra o `DSDeathCounter.exe`.

### O aviso do Windows SmartScreen

O app não tem assinatura digital, então na primeira vez o Windows mostra **"O Windows protegeu o computador"**. Clique em **Mais informações → Executar assim mesmo**. O navegador também pode dizer que o arquivo "não é baixado com frequência"; escolha **Manter**. Isso é esperado em projetos pequenos sem assinatura e não é uma detecção de vírus. O [README](../README.pt-BR.md#smartscreen) explica como conferir que o arquivo baixado é o oficial (soma de verificação e atestado da compilação).

## 2. Primeira vez

A tela *Bem-vindo ao DS Death Counter* pede duas coisas:

1. **Vincular ao jogo.** O app procura o Dark Souls Remastered nas bibliotecas da Steam. Se não achar, clique em *Procurar DarkSoulsRemastered.exe…* e selecione o executável do jogo. O caminho só é usado pelo botão *Iniciar Dark Souls*: o contador reconhece o jogo aberto sozinho de qualquer jeito.
2. **Sempre ligado.** Deixe marcado *Iniciar com o Windows, minimizado na bandeja*. Assim o app abre junto com o Windows, fica na bandeja, e suas mortes são contadas sempre que você abrir o jogo, sem precisar lembrar de abrir o contador.

Clique em *Começar*.

## 3. Enquanto você joga

### Linha de status

No topo da janela:

| Status | Significado |
|---|---|
| *Aguardando o Dark Souls Remastered ou o Elden Ring abrir…* | Nenhum jogo compatível está aberto. *Iniciar Dark Souls* abre o Dark Souls Remastered. |
| *Dark Souls encontrado, preparando a leitura…* | O jogo foi encontrado e o contador está localizando os dados (leva um instante). |
| *Conectado ao jogo (versão X). Carregue um personagem.* | Conectado; você está no menu principal. |
| *Contando as mortes de NOME (slot N)* | Contando. |
| *O jogo está rodando como administrador…* | O Windows bloqueou o acesso. Veja [Problemas comuns](#9-problemas-comuns). |

### O overlay

A contagem do personagem atual aparece num canto da janela do jogo (padrão: canto superior esquerdo, `Mortes: 123`) e acompanha a janela quando você move ou redimensiona.

- **Modo de tela:** o overlay é uma janela transparente separada, então ele sempre funciona em **Janela** ou **Janela sem bordas**. Em **Tela cheia** exclusiva ele pode não aparecer, ou piscar, dependendo do Windows e do driver de vídeo.
- **Visual de tela cheia sem tela cheia exclusiva:** coloque o jogo em modo Janela na resolução do monitor e use *Configurações › Janela do jogo › Deixar a janela do jogo sem bordas*. O app só tira a moldura da janela; nada dentro do jogo é alterado.
- **Mostrar/ocultar:** `Ctrl+Shift+D`, ou o menu do ícone na bandeja.
- Por padrão o overlay some quando você sai do jogo com alt-tab e no menu principal do jogo.
- **Contagem editada** aparece em outra cor (laranja, por padrão).

### Ícone na bandeja

Fechar a janela mantém o contador rodando na bandeja. Passe o mouse no ícone para ver a contagem atual. Botão direito: *Abrir*, *Mostrar overlay* e *Sair*.

## Elden Ring

O contador também reconhece o **Elden Ring** (`eldenring.exe`), sem nada para configurar. Cada personagem ganha a sua contagem e o seu histórico, como no Dark Souls Remastered. Na lista de personagens, os do Elden Ring aparecem marcados como *Elden Ring* e os do Dark Souls Remastered como *DSR*.

### O Easy Anti-Cheat decide como a contagem é lida

| Como o jogo foi aberto | O que o contador faz |
|---|---|
| Pela Steam, do jeito normal (Easy Anti-Cheat ligado) | **Não abre o processo do jogo de jeito nenhum.** Lê o contador de mortes do arquivo de save, `%APPDATA%\EldenRing\<steamid>\ER0000.sl2`, que o jogo grava depois de cada morte. A contagem muda **alguns segundos depois da morte**, quando o jogo termina de salvar. |
| Sem o Easy Anti-Cheat (modo offline com o EAC desativado, Seamless Co-op) | Lê a memória do jogo, só leitura, como no Dark Souls Remastered. A contagem muda na hora, e cada morte ganha a área exata e a detecção de queda. |

O contador procura o EAC sem tocar no jogo:

- processos `EasyAntiCheat*.exe` ou `start_protected_game.exe` rodando;
- os serviços do Windows *EasyAntiCheat* e *EasyAntiCheat_EOS*;
- se foi o `start_protected_game.exe` (o lançador do EAC) que abriu o jogo.

Ele confere duas vezes, com dois segundos de intervalo, antes de abrir o jogo, e continua conferindo a cada dois segundos. Se o EAC aparecer, o contador larga o acesso na hora e fica no arquivo de save até o jogo fechar. Se outro jogo com EAC estiver aberto ao mesmo tempo, o Elden Ring também é tratado como protegido; isso só quer dizer que a contagem vem do arquivo de save.

O arquivo de save é aberto só para leitura e compartilhado, por alguns milissegundos depois de cada gravação, e nunca é alterado.

### Linha de status (Elden Ring)

| Status | Significado |
|---|---|
| *Elden Ring encontrado, verificando o Easy Anti-Cheat antes de ler…* | O jogo está aberto; o contador está confirmando que o EAC está desligado antes de tocar nele (alguns segundos). |
| *Elden Ring sem Easy Anti-Cheat, preparando a leitura da memória…* | Sem EAC; o contador está localizando os dados do jogo. |
| *Contando as mortes de NOME (slot N)* | Contando pela memória (sem EAC). |
| *Contando as mortes de NOME (slot N) pelo arquivo de save* | EAC ligado; contando pelo arquivo de save. |
| *Elden Ring com Easy Anti-Cheat: aguardando o jogo salvar para saber qual é o personagem* | Você tem mais de um personagem e o jogo ainda não gravou o save desde que o contador abriu, então ele ainda não sabe qual você está jogando. Ele descobre na próxima gravação (por exemplo, ao descansar numa graça ou morrer). |

### O que cada morte registra

- *Área*: dungeons e masmorras pelo nome (*Stormveil Castle*, *Murkwater Catacombs*…). No mundo aberto, o quadrado de 256 m do mapa em que você estava, escrito como *Região - lugares* (por exemplo *Limgrave - Church of Elleh, Stranded Graveyard Exit*). No Reino das Sombras (DLC) aparece só a região. Com o EAC ligado, a área é onde o jogo deixou a sua mancha de sangue, ou seja, onde você morreu. Passe o mouse para ver o id do mapa (`m60_42_36_00`).
- *Última graça (área)*: a área da última graça em que você descansou. O save guarda a graça como um objeto do mapa, não pelo nome, então a coluna mostra a área dela.
- *Por área* agrupa as mortes do Elden Ring por região (*Limgrave*, *Liurnia of the Lakes*, *Stormveil Castle*…).
- A *Queda* só é detectada lendo a memória (sem EAC), porque precisa da posição do personagem.
- As sugestões de causa incluem os principais chefes do Elden Ring e do Shadow of the Erdtree.

### Dicas

- Use o modo de tela **Janela sem bordas** do próprio jogo para o overlay conseguir aparecer por cima. O botão *Deixar a janela do jogo sem bordas* fica desligado enquanto o EAC estiver rodando: o contador fica totalmente passivo.
- Com o EAC ligado, o contador não sabe diferenciar o menu principal do jogo em si, então *Esconder no menu principal do jogo* não se aplica; o overlay aparece sempre que a janela do jogo estiver ativa.
- O botão *Iniciar Dark Souls* só abre o Dark Souls Remastered. Abra o Elden Ring pela Steam.
- Os saves do Seamless Co-op (`ER0000.co2`) só são usados quando o jogo roda sem o EAC.

## 4. Personagens e contagens

A aba *Personagens* lista todos os personagens que o contador já viu (*Saves / personagens*). O que está sendo jogado agora tem uma bolinha verde e fica no topo.

- **Como os personagens são separados:** pelo slot do save e pelo nome. Se você apagar um personagem e criar outro com o mesmo nome no mesmo slot, o contador percebe que o tempo de jogo voltou a zero e começa uma contagem nova.
- **De onde vem o número:** o jogo guarda um contador de mortes por personagem dentro do save. O app lê esse contador, então o número é exato mesmo para mortes de antes de instalar o app ou de quando ele estava fechado.
- *N morte(s) aconteceram antes do contador conhecer este personagem*: N mortes aconteceram antes de o app ver esse personagem pela primeira vez. Elas entram no total, mas sem detalhes na lista.

### Editar a contagem

*Editar contagem…* deixa você definir qualquer número. O jogo continua sendo lido normalmente: as próximas mortes somam a partir do seu número. Enquanto a contagem estiver editada:

- o app mostra o selo **CONTAGEM EDITADA** com a contagem real do jogo e o seu ajuste (*Jogo: 47 · ajuste manual: +3*);
- o número do overlay usa a cor de "editada".

*Voltar à contagem do jogo* remove o ajuste. O contador do próprio jogo nunca é alterado.

### Outras ações

- *Exportar CSV…* salva a lista de mortes do personagem como planilha (separada por `;`, UTF-8, abre no Excel).
- *Excluir personagem…* apaga o personagem e o histórico **só deste app**. O save do jogo não é tocado. Se você carregar esse personagem de novo, ele volta sem o histórico antigo.

## 5. A lista de mortes

A aba *Mortes* tem uma linha por morte, da mais recente para a mais antiga:

| Coluna | Conteúdo |
|---|---|
| *#* | Número da morte no contador do jogo |
| *Data / hora* | Quando aconteceu |
| *Área* | O mapa em que você estava (ex.: *Undead Burg / Undead Parish*, *Anor Londo*). Passe o mouse para ver o id do mapa e as coordenadas. |
| *Última fogueira* | A última fogueira em que você descansou |
| *Causa* | Causa da morte |
| *Tempo de jogo* | Tempo de jogo naquele momento |

- **Causa:** quedas são detectadas sozinhas (perda de mais de 10 m de altura logo antes de morrer) e marcadas como *Queda*. O jogo não guarda "quem te matou" num lugar que dê para ler com segurança; para o resto, **clique na célula da causa** e digite ou escolha uma sugestão (chefes, inimigos comuns, armadilhas, veneno, invasores…).
- **Botão direito numa linha** para editar a causa, editar a área ou excluir aquele registro (excluir um registro não muda a contagem).
- **Linhas cinzas** (*Não registrada (contador fechado)*) são mortes que aconteceram com o contador fechado. Elas contam, mas não têm detalhes.
- **Filtro:** digite na caixa acima da lista para filtrar por área, fogueira ou causa.
- *Por área* / *Por causa* mostram onde e como você mais morre, com porcentagens.

As áreas são os mapas do jogo, então alguns nomes juntam duas regiões (*Blighttown / Quelaag's Domain*, *Demon Ruins / Lost Izalith*). Se uma morte caiu no mapa errado, corrija com botão direito → *Editar área*.

## 6. Configurações

| Opção | O que faz |
|---|---|
| *Executável do jogo* / *Procurar…* / *Detectar na Steam* | Caminho usado por *Iniciar Dark Souls* |
| *Iniciar com o Windows (minimizado na bandeja)* | Abre junto com o Windows, na bandeja |
| *Fechar a janela mantém o contador rodando na bandeja* | O X da janela esconde na bandeja em vez de fechar |
| *Atalho Ctrl+Shift+D mostra/oculta o overlay* | Liga o atalho global |
| *Mostrar a contagem por cima do jogo* | Liga/desliga o overlay |
| *Só quando o jogo estiver em primeiro plano* | Esconde o overlay quando o jogo não é a janela ativa |
| *Esconder no menu principal do jogo* | Esconde no menu principal |
| *Mostrar o nome do personagem* | Mostra o nome acima da contagem |
| *Texto antes do número* | O rótulo, ex.: `Mortes: ` ou `Deaths: ` |
| *Fonte* / *Outra fonte instalada* / *Negrito* / *Tamanho* | Fonte (qualquer fonte instalada funciona) |
| *Ajustar à resolução do jogo* | Os tamanhos são pensados para 1080p e ajustados ao tamanho da janela do jogo |
| *Posição* / *Distância horizontal* / *Distância vertical* | Qual canto, e a que distância dele |
| *Contorno* | Espessura do contorno |
| *Cor normal* / *Cor quando a contagem foi editada* / *Cor do contorno* | Cores |
| *Pré-visualizar o overlay agora* | Mostra o overlay na hora (sobre o jogo, se estiver aberto; senão sobre o monitor) enquanto você ajusta |
| *Deixar a janela do jogo sem bordas* | Tira a borda da janela do jogo e a deixa do tamanho do monitor |
| *Abrir pasta de dados* | Abre a pasta onde seus dados ficam |

## 7. Lives (OBS)

O overlay é desenhado numa janela própria por cima do jogo, e não dentro do jogo. Por isso:

- **Captura de tela** (Display Capture) no OBS mostra o overlay;
- **Captura de jogo** (Game Capture) e **Captura de janela** do jogo **não mostram**.

Está nos planos uma fonte de texto para o OBS (um arquivo com a contagem atual).

## 8. Seus dados

Tudo fica num único arquivo:

```
%APPDATA%\DSDeathCounter\data.json
```

Ele guarda configurações, personagens e o histórico completo. Para fazer backup ou levar para outro PC, copie esse arquivo (com o app fechado). Desinstalar o app não apaga o arquivo.

## 9. Problemas comuns

**"O jogo está rodando como administrador"**: o Windows não deixa um programa comum ler um programa aberto como administrador. Abra o jogo normalmente, ou abra o contador como administrador também.

**O overlay não aparece:**
- confira o modo de tela do jogo (janela/sem bordas, veja o [item 3](#3-enquanto-você-joga));
- aperte `Ctrl+Shift+D` (ele pode estar desligado);
- o overlay some quando o jogo não é a janela ativa e no menu principal (as duas coisas são configuráveis);
- teste com *Pré-visualizar o overlay agora*.

**A contagem diminuiu:** você restaurou uma cópia antiga do save. O contador segue o número do próprio jogo.

**Um personagem aparece duas vezes:** o personagem é identificado por slot + nome, então se ele mudou de slot (por exemplo, copiando arquivos de save), é visto como novo. Exclua a duplicata no app, se quiser.

**Algo parece errado:** abra *Diagnóstico*, clique em *Copiar* e cole o texto num [relato de bug](https://github.com/Theuzin69/ds-death-counter/issues/new/choose). Ele mostra a versão do jogo, os endereços encontrados e os valores lidos (mortes, mapa, posição…).

**Elden Ring: a contagem muda alguns segundos depois da morte:** é o esperado com o Easy Anti-Cheat ligado. O contador espera o jogo gravar o save (veja [Elden Ring](#elden-ring)).

**Elden Ring: *aguardando o jogo salvar*:** você tem vários personagens e o contador está esperando a próxima gravação do jogo para saber qual você está jogando. Descanse numa graça ou continue jogando; ele aparece na próxima gravação.

**Elden Ring: a contagem não muda de jeito nenhum:** abra *Diagnóstico*. As linhas *Save:* mostram qual arquivo é lido, quando foi a última leitura e o que cada slot tem. Se algum slot disser que o contador de mortes não foi localizado, uma atualização do jogo mudou o formato do save: por favor, avise mandando o texto do diagnóstico.

**O antivírus reclamou:** veja a [seção sobre SmartScreen/antivírus](../README.pt-BR.md#smartscreen). O app lê a memória de outro programa, o que alguns antivírus estranham. Ele nunca escreve no jogo.

## 10. Desinstalar

- **Versão instalada:** Configurações do Windows › Aplicativos › *DS Death Counter* › Desinstalar.
- **Versão portátil:** feche pelo ícone da bandeja (*Sair*) e apague a pasta. Se você ativou *Iniciar com o Windows*, desative antes em *Configurações* (ou no Gerenciador de Tarefas › Inicializar).

O histórico em `%APPDATA%\DSDeathCounter` é mantido nos dois casos; apague essa pasta se quiser removê-lo.
