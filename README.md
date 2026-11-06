# FilterCam - Grau B

Autor: Guilherme Menna Barreto.

Trabalho de Processamento Grafico: Fundamentos. Prototipo de um aplicativo para aplicar efeitos em fotos e em video ao vivo da webcam, desenvolvido em C++17 com OpenCV 4 e sua interface basica HighGUI.

## Base dos trabalhos anteriores

A organizacao segue as Listas 2 e 3: projeto CMake, codigo C++, callbacks de mouse, controles de teclado e comentarios em portugues. A conversao do clique da janela para coordenadas da imagem reaproveita o conceito de mapeamento de viewport das listas. Do Zelda foram aproveitadas a separacao entre entrada/loop e logica, a copia dos recursos para junto do executavel e cinco imagens transparentes ja existentes. O processamento e feito com OpenCV, conforme a recomendacao do enunciado.

## Estrutura

```text
FilterCam/
|-- CMakeLists.txt
|-- configurar.ps1
|-- .gitignore
|-- README.md
|-- assets/
|   |-- README.md
|   `-- stickers/       # cinco PNGs com alfa
|-- src/
|   |-- main.cpp        # entrada, argumentos e localizacao dos recursos
|   |-- Aplicativo.h
|   |-- Aplicativo.cpp  # foto, webcam, teclado, mouse e HighGUI
|   |-- Processamento.h
|   `-- Processamento.cpp # filtros, canais, aritmetica, alfa e PNG
`-- tests/
    `-- teste_processamento.cpp
```

## Requisitos para compilar no Windows

- Visual Studio 2022 ou Build Tools 2022, com Desenvolvimento para desktop com C++ e Windows SDK.
- CMake 3.16 ou superior, disponivel no PATH.
- Windows x64. O script usa o pacote oficial OpenCV 4.12.0 para MSVC.
- Acesso a internet na primeira configuracao, para baixar o OpenCV; depois a compilacao e a execucao funcionam offline.
- Webcam para o modo video. Fotos funcionam sem camera.

O OpenCV e extraido localmente em `.deps/`, sem instalacao global. Python, GLFW, GLAD e GLM nao sao necessarios para este trabalho.

## Compilacao e testes

No PowerShell, dentro da pasta deste projeto:

```powershell
powershell -ExecutionPolicy Bypass -File .\configurar.ps1
```

O comando baixa e verifica o SHA256 do pacote oficial, extrai a dependencia, configura o CMake, compila em Release e executa os testes. Nao altera a politica de execucao permanente do Windows.

Se ja tiver o OpenCV instalado:

```powershell
.\configurar.ps1 -OpenCVDir 'C:\opencv\build'
```

Tambem e possivel usar CMake diretamente:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DOpenCV_DIR="C:/opencv/build"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Para uma instalacao com outra estrutura, coloque as DLLs do OpenCV no PATH ou junto dos executaveis. O CMake copia automaticamente as DLLs da estrutura oficial `build/x64/vc*/bin`.

## Execucao

```powershell
.\build\Release\grau_b.exe
```

Pode abrir o executavel com duplo clique. Mantenha o console aberto: ele mostra as descricoes dos filtros e recebe os caminhos dos arquivos.

Uma foto inicial e um indice de webcam podem ser passados por argumento:

```powershell
.\build\Release\grau_b.exe 'C:\Fotos\paisagem.jpg'
.\build\Release\grau_b.exe --camera 1
```

Os recursos sao localizados junto ao executavel. O programa pode ser iniciado de outra pasta. No Windows, os argumentos de linha de comando usam a entrada Unicode (`wmain`). Os caminhos digitados no console aceitam espacos, acentos e aspas externas; o aplicativo configura o console para UTF-8.

## Controles

Clique na janela de imagem para que ela receba as teclas.

| Entrada | Acao |
|---|---|
| `F` | Solicita no console o caminho de uma foto do disco. Enter vazio cancela. |
| `V` | Inicia ou para a webcam. Ao parar, preserva o ultimo frame. |
| `O` | Solicita no console o caminho da segunda imagem das operacoes. |
| Barra Filtro | Seleciona original (`0`) ou um dos 10 filtros (`1` a `10`). |
| Barra Canal | Seleciona RGB (`0`), R (`1`), G (`2`) ou B (`3`). |
| Barra Parametro P | Escolhe o parametro entre 0 e 255. |
| Barra Operacao | Seleciona nenhuma (`0`), adicao (`1`), subtracao (`2`) ou blending (`3`). |
| `Enter` | Aplica o filtro/operacao selecionado com o parametro escolhido. |
| `1` a `5` | Seleciona Link, Princesa, Arvore, Arbusto ou Ponte. |
| Clique esquerdo | Posiciona o sticker selecionado no Modo Foto. |
| `0` | Desmarca o sticker, sem remover os ja posicionados. |
| `S` | Salva a foto resultante ou o frame atual como PNG. |
| `R` | Restaura a origem: remove efeito e stickers e volta a RGB. |
| `H` | Exibe os controles e a lista de filtros no console. |
| `Esc` | Fecha o aplicativo e libera a webcam. |

Ao carregar arquivos, volte ao console para digitar o caminho e depois clique novamente na imagem. Durante essa entrada, o video fica temporariamente sem atualizacao.

## Filtros

A selecao exibe a descricao e os parametros no console **antes** de aplicar. A janela mostra separadamente o efeito selecionado e o aplicado. Mudar filtro ou parametro exige Enter; mudar o canal atualiza imediatamente a imagem principal.

| Numero | Filtro | Operacao/parametro |
|---|---|---|
| 1 | Tons de cinza | Luminancia: `0.114 B + 0.587 G + 0.299 R`. Sem parametro. |
| 2 | Negativo | `255 - pixel`. Sem parametro. |
| 3 | Box Blur | Media local; kernel impar `k = 2 * floor(P/17) + 1`, de 1 a 31. |
| 4 | Gaussian Blur | Suavizacao gaussiana, mesmo tamanho de kernel; sigma automatico. |
| 5 | Mediana | Mediana local, mesmo tamanho de kernel. |
| 6 | Sharpen | Centro `1+4a` e quatro vizinhos `-a`, com `a = 2P/255`. |
| 7 | Laplaciano | Modulo da segunda derivada, kernel 3x3 e ganho `1+3P/255`. |
| 8 | Sobel | Magnitude dos gradientes x/y, kernel 3x3 e ganho `1+3P/255`. |
| 9 | Limiarizacao | Branco se intensidade `> P`, preto caso contrario. |
| 10 | Sepia | Matriz de tons quentes, com saturacao em 255. Sem parametro. |

`P` inicia em 128. Box, Gaussian e Mediana atendem a suavizacao; Sharpen atende ao realce. Os filtros 7 e 8 visualizam as bordas. Os filtros 1 e 9 usam luminancia no modo RGB. Nos demais casos, os filtros de vizinhanca processam todos os canais quando RGB esta selecionado.

Em R/G/B, apenas o canal escolhido entra no processamento. Sua intensidade e exibida em tons de cinza na **imagem principal**, sem os outros canais. O OpenCV armazena BGR internamente; a selecao R acessa o indice 2, G o indice 1 e B o indice 0. Sepia requer tres canais para produzir cor; em canal isolado, preserva o canal monocromatico.

## Operacoes com duas imagens

Carregue uma foto com `F` (ou inicie a webcam), carregue B com `O`, selecione a operacao na barra e pressione Enter. A segunda imagem e redimensionada para a resolucao da primeira. A selecao de canal vale para ambas.

| Operacao | Formula, por pixel |
|---|---|
| Adicao saturada | `min(A + B, 255)`; sem parametro. |
| Subtracao ponderada | `max(A - peso*B, 0)`, com `peso = P/255`. |
| Blending | `peso*A + (1-peso)*B`, com `peso = P/255`. |

As operacoes funcionam na foto e em cada frame da webcam, usando B como imagem estatica. A aritmetica satura no intervalo 0-255, evitando overflow de inteiros de 8 bits.

## Stickers e salvamento

Os cinco stickers estaticos com transparencia foram reaproveitados do Zelda; a origem de cada arquivo esta em `assets/README.md`. Selecione com `1` a `5` e clique na foto. O centro do sticker fica no clique, a proporcao e mantida e a parte que sair da imagem e recortada. A largura corresponde a aproximadamente 80 pixels da previa. A composicao usa `alfa*sticker + (1-alfa)*fundo`, com alfa entre 0 e 1.

Os stickers sao aplicados depois do filtro. A imagem original e preservada. `R` limpa todos os stickers e efeitos. Stickers ficam disponiveis somente no Modo Foto, como permitido pelo enunciado.

`S` grava em `build/Release/output/images/` na compilacao descrita. O nome inclui o instante em microssegundos para evitar substituir resultados anteriores. O PNG tem a resolucao da origem e contem o efeito aplicado e os stickers; os controles e textos da janela nao entram no arquivo. No video, e salvo somente o frame atual.

## Conferencia dos requisitos

| Requisito obrigatorio | Implementacao |
|---|---|
| Foto do disco | `Aplicativo::carregarFoto` / `lerImagem` |
| Webcam em tempo real; iniciar/parar | `Aplicativo::alternarWebcam` e loop com `VideoCapture::read` |
| 10 filtros, incluindo suavizacao e realce | `listaFiltros` / `aplicarFiltro` |
| RGB ou R/G/B com resultado na imagem principal | `selecionarCanal` / `processar` |
| 3 operacoes aritmeticas com duas imagens | `aplicarOperacao` |
| Descricao antes da aplicacao e parametros | `descreverSelecao` e Enter para confirmar a selecao |
| 5 stickers com alfa e posicionamento pelo mouse | `carregarStickers`, `clicar` e `sobreporSticker` |
| Salvar foto/frame | `Aplicativo::salvar` / `salvarPNG` |
| Resetar para original | `Aplicativo::resetar` |
| Organizacao de repositorio | CMake, README, `.gitignore`, fontes e recursos |

Cada filtro ou operacao substitui o anterior e parte da foto original ou do frame atual. Nao ha combinacao sequencial de filtros, GUI externa, animacao de stickers ou deteccao de face: os pontos extras nao fazem parte deste projeto.

## Validacao

Os testes usam imagens sinteticas para conferir todos os filtros em RGB/R/G/B e nos extremos de parametro, saturacao das operacoes, preservacao da origem, reset do processamento, composicao alfa, recorte nas bordas, os cinco PNGs reais e gravacao/leitura de PNG com acentos no caminho. A fixture geometrica gerada nos testes lembra a casa dos exercicios da Lista 2.

Na maquina de desenvolvimento, o script completo compilou em Release e passaram 412 verificacoes. A tentativa de abrir a webcam retornou dispositivo indisponivel; a captura real precisa ser conferida em um computador com camera. A ferramenta de captura de tela tambem nao conseguiu capturar a janela para a revisao visual automatica, portanto o roteiro interativo abaixo continua indicado.

```powershell
ctest --test-dir build -C Release --output-on-failure
```

Para conferir interativamente:

1. Abra uma foto com `F`; selecione um filtro, leia a descricao e aplique com Enter.
2. Selecione R, G e B; confira que a imagem principal exibe a intensidade do canal.
3. Carregue B com `O` e teste cada operacao, incluindo P=0 e P=255.
4. Selecione os cinco stickers e clique no centro e nas bordas da foto.
5. Salve com `S`, abra o PNG e confira a resolucao e a ausencia dos controles.
6. Pressione `R` e confira a foto original, sem stickers.
7. Inicie a webcam com `V`, aplique filtros e operacoes, salve um frame, resete e pare com `V`.

A validacao do video ao vivo exige uma webcam disponivel e autorizada pelo sistema. Se a camera estiver ocupada, desconectada ou bloqueada, o aplicativo exibe um erro e preserva a imagem anterior. Use `--camera 1` para tentar outro dispositivo.

## Entrega

Versione os fontes, CMake, script, READMEs e assets. `.gitignore` exclui `build/`, `.deps/` e saidas. O enunciado pede entrega por link de repositorio; publique esta pasta no seu GitHub e envie o link no Moodle conforme as orientacoes da disciplina. A publicacao e o envio nao sao feitos automaticamente pelo aplicativo.

## Referencias

- Enunciado Grau B, disponibilizado na pasta da disciplina.
- Listas 2 e 3 e Zelda 2D Game, fornecidos como base neste workspace.
- [OpenCV: HighGUI](https://docs.opencv.org/4.x/d7/dfc/group__highgui.html).
- [OpenCV: suavizacao e convolucao](https://docs.opencv.org/4.x/d4/d13/tutorial_py_filtering.html).
- [OpenCV: operacoes e blending](https://docs.opencv.org/4.x/d5/dc4/tutorial_adding_images.html).
- [Pacote oficial OpenCV 4.12.0](https://github.com/opencv/opencv/releases/tag/4.12.0).
