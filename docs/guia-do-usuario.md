# Guia do usuário

[English](user-guide.md) · **Português (Brasil)**

- [1. Instalação](#1-instalação)
- [2. Primeira vez](#2-primeira-vez)
- [3. Enquanto você joga](#3-enquanto-você-joga)
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
| *Aguardando o Dark Souls Remastered abrir…* | O jogo não está aberto. *Iniciar Dark Souls* abre o jogo. |
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
| *Gravar a contagem num arquivo de texto para o OBS* | Grava a contagem num arquivo de texto que o OBS mostra (desligado por padrão, veja [Lives](#7-lives-obs)) |
| *Texto no OBS* | O que vai no arquivo: `{mortes}` vira a contagem e `{nome}`, o nome do personagem |
| *Arquivo* / *Escolher arquivo…* / *Usar o padrão* / *Abrir pasta* / *Copiar caminho* | Onde o arquivo é gravado (padrão `%APPDATA%\DSDeathCounter\obs.txt`) |
| *Abrir pasta de dados* | Abre a pasta onde seus dados ficam |

## 7. Lives (OBS)

O overlay é desenhado numa janela própria por cima do jogo, e não dentro do jogo. Por isso:

- **Captura de tela** (Display Capture) no OBS mostra o overlay;
- **Captura de jogo** (Game Capture) e **Captura de janela** do jogo **não mostram**.

Com Captura de jogo ou Captura de janela, use o arquivo de texto para o OBS: o app mantém a contagem atual num pequeno arquivo de texto, e uma fonte de texto do OBS mostra esse arquivo. Você posiciona e estiliza a fonte no OBS como qualquer outra.

### Configurando a fonte de texto

1. No app, abra *Configurações* → *OBS (lives)* e marque *Gravar a contagem num arquivo de texto para o OBS*. A linha *Gravando: …* mostra o que está no arquivo.
2. Opcional: mude o *Texto no OBS* (padrão `Mortes: {mortes}`), por exemplo para `{nome}: {mortes} mortes`.
3. Clique em *Copiar caminho* para copiar o local do arquivo. O padrão é `%APPDATA%\DSDeathCounter\obs.txt`; *Escolher arquivo…* escolhe outro arquivo `.txt` e *Usar o padrão* volta para o padrão.
4. No OBS, no painel **Fontes**, clique em **+** e escolha **Texto (GDI+)**. Dê um nome (ex.: "Mortes") e clique em **OK**.
5. Na janela de propriedades, marque **Carregar do arquivo**, clique em **Procurar** ao lado de **Arquivo de texto (UTF-8)** e escolha o arquivo (dá para colar o caminho copiado na janela de seleção). Clique em **OK**.
6. Escolha fonte, tamanho, cor e contorno nessa mesma janela. Arraste a fonte para onde quiser e deixe-a acima da captura do jogo na lista de Fontes.

O arquivo muda logo depois de cada morte (o OBS confere o arquivo mais ou menos uma vez por segundo). Ele mostra o personagem que está sendo jogado; com o jogo fechado, continua mostrando o último personagem jogado.

Marcadores do *Texto no OBS*:

| Marcador | Vira |
|---|---|
| `{mortes}` (ou `{deaths}`) | A contagem de mortes, o mesmo número que o app mostra (incluindo edições manuais) |
| `{nome}` (ou `{name}`) | O nome do personagem |

O resto é gravado do jeito que você digitou: `{nome}: {mortes} mortes` vira `Chosen Undead: 42 mortes`.

Se o app mostrar *Não foi possível gravar o arquivo*, a pasta escolhida não existe mais ou não aceita gravação: escolha outro arquivo ou clique em *Usar o padrão*.

## 8. Seus dados

Tudo fica num único arquivo:

```
%APPDATA%\DSDeathCounter\data.json
```

Ele guarda configurações, personagens e o histórico completo. Para fazer backup ou levar para outro PC, copie esse arquivo (com o app fechado). Desinstalar o app não apaga o arquivo.

Se o arquivo de texto para o OBS estiver ligado, o `obs.txt` fica na mesma pasta (no local padrão). Ele só guarda o texto mostrado no OBS e é regravado quando preciso, então não precisa de backup.

## 9. Problemas comuns

**"O jogo está rodando como administrador"**: o Windows não deixa um programa comum ler um programa aberto como administrador. Abra o jogo normalmente, ou abra o contador como administrador também.

**O overlay não aparece:**
- confira o modo de tela do jogo (janela/sem bordas, veja o [item 3](#3-enquanto-você-joga));
- aperte `Ctrl+Shift+D` (ele pode estar desligado);
- o overlay some quando o jogo não é a janela ativa e no menu principal (as duas coisas são configuráveis);
- teste com *Pré-visualizar o overlay agora*.

**O overlay não aparece no OBS:** a Captura de jogo e a Captura de janela não pegam o overlay. Use o [arquivo de texto para o OBS](#7-lives-obs).

**A contagem diminuiu:** você restaurou uma cópia antiga do save. O contador segue o número do próprio jogo.

**Um personagem aparece duas vezes:** o personagem é identificado por slot + nome, então se ele mudou de slot (por exemplo, copiando arquivos de save), é visto como novo. Exclua a duplicata no app, se quiser.

**Algo parece errado:** abra *Diagnóstico*, clique em *Copiar* e cole o texto num [relato de bug](https://github.com/Theuzin69/ds-death-counter/issues/new/choose). Ele mostra a versão do jogo, os endereços encontrados e os valores lidos (mortes, mapa, posição…).

**O antivírus reclamou:** veja a [seção sobre SmartScreen/antivírus](../README.pt-BR.md#smartscreen). O app lê a memória de outro programa, o que alguns antivírus estranham. Ele nunca escreve no jogo.

## 10. Desinstalar

- **Versão instalada:** Configurações do Windows › Aplicativos › *DS Death Counter* › Desinstalar.
- **Versão portátil:** feche pelo ícone da bandeja (*Sair*) e apague a pasta. Se você ativou *Iniciar com o Windows*, desative antes em *Configurações* (ou no Gerenciador de Tarefas › Inicializar).

O histórico em `%APPDATA%\DSDeathCounter` é mantido nos dois casos; apague essa pasta se quiser removê-lo.
