#include "Aplicativo.h"

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <chrono>
#include <iostream>

static const std::vector<std::string> NOMES_STICKERS = {"Link", "Princesa", "Arvore", "Arbusto", "Ponte"};

// ---------------------------------------------------------------------------
// Tema e layout da interface (cores em BGR)
// ---------------------------------------------------------------------------
namespace
{
    const cv::Scalar MARINHO(75, 31, 11);       // #0B1F4B
    const cv::Scalar AZUL(235, 99, 37);         // #2563EB
    const cv::Scalar FUNDO(249, 242, 238);      // #EEF2F9
    const cv::Scalar BRANCO(255, 255, 255);
    const cv::Scalar BORDA(235, 222, 215);      // #D7DEEB
    const cv::Scalar TRILHA(240, 227, 220);     // #DCE3F0
    const cv::Scalar CINZA(139, 116, 100);      // #64748B
    const cv::Scalar CLARO(216, 184, 169);      // #A9B8D8
    constexpr int FONTE = cv::FONT_HERSHEY_DUPLEX;

    // Cartoes
    const cv::Rect CARTAO_FOTO(20, 76, 700, 480);
    const cv::Rect PAINEL(740, 76, 340, 480);

    // Controles do painel
    const cv::Rect TRILHA_FILTRO(760, 132, 300, 6);
    const cv::Rect ACERTO_FILTRO(750, 118, 320, 32);
    const cv::Rect SEG_CANAL(760, 186, 300, 30);
    const cv::Rect TRILHA_PARAM(760, 272, 300, 6);
    const cv::Rect ACERTO_PARAM(750, 258, 320, 32);
    const cv::Rect SEG_OPERACAO(760, 326, 300, 30);

    const char *ROTULOS_CANAL[4] = {"RGB", "R", "G", "B"};
    const char *ROTULOS_OPERACAO[4] = {"Nenhuma", "+", "-", "Blend"};

    struct Botao
    {
        cv::Rect area;
        const char *rotulo;
        int tecla;
        bool primario;
    };
    const Botao BOTOES[] = {
        {cv::Rect(760, 414, 300, 38), "APLICAR  [ENTER]", 13, true},
        {cv::Rect(760, 464, 96, 32), "Foto [F]", 'f', false},
        {cv::Rect(862, 464, 96, 32), "Webcam [V]", 'v', false},
        {cv::Rect(964, 464, 96, 32), "Img B [O]", 'o', false},
        {cv::Rect(760, 504, 96, 32), "Salvar [S]", 's', false},
        {cv::Rect(862, 504, 96, 32), "Reset [R]", 'r', false},
        {cv::Rect(964, 504, 96, 32), "Ajuda [H]", 'h', false},
    };

    cv::Rect cartaoSticker(int i) { return cv::Rect(20 + i * 72, 598, 60, 60); }

    // ---- Primitivas de desenho ----
    void preencherArredondado(cv::Mat &img, cv::Rect r, int raio, const cv::Scalar &cor)
    {
        raio = std::max(0, std::min({raio, r.width / 2, r.height / 2}));
        cv::rectangle(img, cv::Rect(r.x + raio, r.y, r.width - 2 * raio, r.height), cor, cv::FILLED);
        cv::rectangle(img, cv::Rect(r.x, r.y + raio, r.width, r.height - 2 * raio), cor, cv::FILLED);
        const int dir = r.x + r.width - raio - 1, baixo = r.y + r.height - raio - 1;
        cv::circle(img, cv::Point(r.x + raio, r.y + raio), raio, cor, cv::FILLED, cv::LINE_AA);
        cv::circle(img, cv::Point(dir, r.y + raio), raio, cor, cv::FILLED, cv::LINE_AA);
        cv::circle(img, cv::Point(r.x + raio, baixo), raio, cor, cv::FILLED, cv::LINE_AA);
        cv::circle(img, cv::Point(dir, baixo), raio, cor, cv::FILLED, cv::LINE_AA);
    }

    void cartao(cv::Mat &img, const cv::Rect &r, int raio, const cv::Scalar &fundo, const cv::Scalar &borda)
    {
        preencherArredondado(img, r, raio, borda);
        preencherArredondado(img, cv::Rect(r.x + 1, r.y + 1, r.width - 2, r.height - 2), std::max(0, raio - 1), fundo);
    }

    cv::Size medir(const std::string &s, double escala, int espessura = 1)
    {
        int base = 0;
        return cv::getTextSize(s, FONTE, escala, espessura, &base);
    }

    void texto(cv::Mat &img, const std::string &s, cv::Point origem, double escala,
               const cv::Scalar &cor, int espessura = 1)
    {
        cv::putText(img, s, origem, FONTE, escala, cor, espessura, cv::LINE_AA);
    }

    void textoCentralizado(cv::Mat &img, const std::string &s, const cv::Rect &r, double escala,
                           const cv::Scalar &cor, int espessura = 1)
    {
        const cv::Size t = medir(s, escala, espessura);
        texto(img, s, cv::Point(r.x + (r.width - t.width) / 2, r.y + (r.height + t.height) / 2),
              escala, cor, espessura);
    }

    void textoADireita(cv::Mat &img, const std::string &s, int xDireita, int y, double escala,
                       const cv::Scalar &cor, int espessura = 1)
    {
        texto(img, s, cv::Point(xDireita - medir(s, escala, espessura).width, y), escala, cor, espessura);
    }

    void desenharSlider(cv::Mat &img, const cv::Rect &trilha, int valor, int maximo)
    {
        const int xPolegar = trilha.x + cvRound(static_cast<double>(trilha.width) * valor / maximo);
        const int yCentro = trilha.y + trilha.height / 2;
        preencherArredondado(img, trilha, 3, TRILHA);
        if (xPolegar > trilha.x)
            preencherArredondado(img, cv::Rect(trilha.x, trilha.y, xPolegar - trilha.x, trilha.height), 3, AZUL);
        cv::circle(img, cv::Point(xPolegar, yCentro), 10, BORDA, cv::FILLED, cv::LINE_AA);
        cv::circle(img, cv::Point(xPolegar, yCentro), 9, AZUL, cv::FILLED, cv::LINE_AA);
        cv::circle(img, cv::Point(xPolegar, yCentro), 5, BRANCO, cv::FILLED, cv::LINE_AA);
    }

    void desenharSegmentos(cv::Mat &img, const cv::Rect &r, const char *const *rotulos, int n, int ativo)
    {
        preencherArredondado(img, r, 8, TRILHA);
        const int largura = r.width / n;
        for (int i = 0; i < n; ++i)
        {
            const cv::Rect seg(r.x + i * largura + 2, r.y + 2, largura - 4, r.height - 4);
            if (i == ativo) preencherArredondado(img, seg, 6, AZUL);
            textoCentralizado(img, rotulos[i], seg, 0.45, i == ativo ? BRANCO : MARINHO);
        }
    }

    void desenharBotao(cv::Mat &img, const Botao &b)
    {
        if (b.primario)
        {
            preencherArredondado(img, b.area, 10, AZUL);
            textoCentralizado(img, b.rotulo, b.area, 0.55, BRANCO);
        }
        else
        {
            cartao(img, b.area, 8, BRANCO, BORDA);
            textoCentralizado(img, b.rotulo, b.area, 0.4, MARINHO);
        }
    }

    int valorSlider(const cv::Rect &trilha, int maximo, int x)
    {
        return std::clamp(cvRound((x - trilha.x) * static_cast<double>(maximo) / trilha.width), 0, maximo);
    }

    int indiceSegmento(const cv::Rect &r, int n, int x)
    {
        return std::clamp((x - r.x) * n / r.width, 0, n - 1);
    }
}

// ---------------------------------------------------------------------------

Aplicativo::Aplicativo(std::filesystem::path pastaExecutavel, int camera)
    : pasta(std::move(pastaExecutavel)), indiceCamera(camera)
{
}

void Aplicativo::carregarStickers()
{
    for (const auto &nome : {"link.png", "princesa.png", "arvore.png", "arbusto.png", "ponte.png"})
    {
        cv::Mat sticker = lerImagem(pasta / "assets" / "stickers" / nome, cv::IMREAD_UNCHANGED);
        if (sticker.type() != CV_8UC4) throw std::runtime_error("Sticker sem canal alfa: " + std::string(nome));
        stickers.push_back(sticker);
    }
}

void Aplicativo::ajuda()
{
    std::cout << "\nGRAU B - Processamento de imagens e video\n"
        << "F: carregar foto (digite o caminho no console)\n"
        << "V: iniciar/parar webcam | O: carregar segunda imagem\n"
        << "Painel lateral: selecionar filtro, canal, parametro e operacao (mouse)\n"
        << "ENTER ou botao APLICAR: aplicar a selecao; descricoes aparecem antes no console\n"
        << "Canal R/G/B: a imagem principal passa a exibir aquele canal em cinza\n"
        << "S: salvar resultado/frame em output/images, ao lado do executavel\n"
        << "R: resetar filtro, operacao, canal, parametro e todos os stickers\n"
        << "1 Link | 2 Princesa | 3 Arvore | 4 Arbusto | 5 Ponte (ou clique na miniatura)\n"
        << "Selecione e clique na foto; 0 desmarca o sticker\n"
        << "H: repetir ajuda | ESC: sair | Clique na janela antes de usar o teclado\n"
        << "Filtros e operacoes substituem o efeito anterior, usando sempre a origem.\n";
    for (std::size_t i = 1; i < listaFiltros().size(); ++i)
        std::cout << i << ": " << listaFiltros()[i].nome << '\n';
}

std::filesystem::path Aplicativo::pedirCaminho(const std::string &rotulo)
{
    std::cout << '\n' << rotulo << " (Enter vazio cancela): " << std::flush;
    std::string caminho;
    if (!std::getline(std::cin, caminho))
    {
        std::cin.clear();
        return {};
    }
    // Aceita tambem o caminho entre aspas, como ao usar Copiar como caminho.
    const auto inicio = caminho.find_first_not_of(" \t\r\n");
    if (inicio == std::string::npos) return {};
    caminho = caminho.substr(inicio, caminho.find_last_not_of(" \t\r\n") - inicio + 1);
    if (caminho.size() >= 2 && caminho.front() == '"' && caminho.back() == '"')
        caminho = caminho.substr(1, caminho.size()-2);
    return std::filesystem::u8path(caminho);
}

void Aplicativo::carregarFoto(const std::filesystem::path &caminho)
{
    // So troca o modo depois que a nova imagem foi validada.
    cv::Mat nova = lerImagem(caminho, cv::IMREAD_COLOR);
    camera.release();
    modoVideo = false;
    original = nova;
    resetar();
    mensagem = "Foto carregada: " + std::to_string(original.cols) + " x " + std::to_string(original.rows);
    std::cout << mensagem << '\n';
}

void Aplicativo::alternarWebcam()
{
    if (camera.isOpened())
    {
        camera.release();
        mensagem = "Webcam parada. Ultimo frame preservado; V retoma, F carrega uma foto.";
        return;
    }
    cv::VideoCapture nova;
    bool abriu = false;
#ifdef _WIN32
    abriu = nova.open(indiceCamera, cv::CAP_DSHOW);
#endif
    if (!abriu) abriu = nova.open(indiceCamera, cv::CAP_ANY);
    cv::Mat primeiroFrame;
    if (!abriu || !nova.read(primeiroFrame) || primeiroFrame.empty())
        throw std::runtime_error("Webcam indisponivel. Verifique conexao, permissoes e indice --camera.");
    camera = std::move(nova);
    original = primeiroFrame.clone();
    modoVideo = true;
    resetar();
    mensagem = "Webcam iniciada. ENTER aplica o efeito a cada novo frame; V para.";
}

void Aplicativo::resetar()
{
    selecionado = Efeito{};
    aplicado = Efeito{};
    ultimoFiltro = ultimaOperacao = ultimoCanal = 0;
    ultimoParametro = 128;
    uiFiltro = uiOperacao = uiCanal = 0;
    uiParametro = 128;
    posicoes.clear();
    stickerSelecionado = -1;
    atualizar = true;
    mensagem = "Reset: imagem original, sem efeitos e sem stickers.";
}

void Aplicativo::lerSelecao()
{
    int filtro = uiFiltro;
    int operacao = uiOperacao;
    const int canal = uiCanal;
    const int parametro = uiParametro;
    if (filtro == ultimoFiltro && operacao == ultimaOperacao && canal == ultimoCanal && parametro == ultimoParametro)
        return;
    if (filtro != ultimoFiltro)
    {
        operacao = 0;
        uiOperacao = 0;
    }
    else if (operacao != ultimaOperacao && operacao != 0)
    {
        filtro = 0;
        uiFiltro = 0;
    }
    selecionado.filtro = ultimoFiltro = filtro;
    selecionado.operacao = static_cast<Operacao>(operacao);
    ultimaOperacao = operacao;
    selecionado.parametro = ultimoParametro = parametro;
    selecionado.canal = static_cast<Canal>(canal);
    // A selecao de canal deve aparecer de imediato na imagem principal.
    if (canal != ultimoCanal)
    {
        aplicado.canal = selecionado.canal;
        atualizar = true;
    }
    ultimoCanal = canal;
    descreverSelecao();
    mensagem = "Selecao: " + nomeEfeito(selecionado) + ". Leia a descricao no console e pressione ENTER.";
}

void Aplicativo::descreverSelecao() const
{
    std::cout << "\nSelecionado: " << nomeEfeito(selecionado)
        << " | Canal: " << nomeCanal(selecionado.canal) << " | P: " << selecionado.parametro << '\n';
    if (selecionado.operacao == Operacao::Nenhuma)
    {
        const auto &filtro = listaFiltros().at(selecionado.filtro);
        std::cout << filtro.descricao << '\n' << filtro.parametro << '\n';
    }
    else
    {
        switch (selecionado.operacao)
        {
            case Operacao::Adicao:
                std::cout << "Soma pixel a pixel: min(A + B, 255). Sem parametro.\n";
                break;
            case Operacao::Subtracao:
                std::cout << "Subtracao ponderada: max(A - peso*B, 0); peso = P/255.\n";
                break;
            case Operacao::Blending:
                std::cout << "Mistura: peso*A + (1-peso)*B; peso = P/255.\n";
                break;
            default: break;
        }
        std::cout << "B e redimensionada para A. Use O para carregar B.\n";
    }
    std::cout << "ENTER aplica sobre a origem.\n";
}

void Aplicativo::aplicarSelecao()
{
    if (original.empty()) throw std::runtime_error("Carregue uma foto (F) ou inicie a webcam (V).");
    if (selecionado.operacao != Operacao::Nenhuma && segundaImagem.empty())
        throw std::runtime_error("Use O para carregar a segunda imagem antes de aplicar a operacao.");
    aplicado = selecionado;
    atualizar = true;
    mensagem = "Aplicado: " + nomeEfeito(aplicado) + " | Canal " + nomeCanal(aplicado.canal)
        + " | P=" + std::to_string(aplicado.parametro);
    std::cout << mensagem << '\n';
}

void Aplicativo::salvar()
{
    if (original.empty()) throw std::runtime_error("Nao ha imagem para salvar.");
    // Recalcula aqui para incluir um clique/tecla recebido no mesmo ciclo.
    resultado = comporResultado(original, segundaImagem, aplicado, stickers, posicoes);
    const auto instante = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const auto arquivo = pasta / "output" / "images" / ("resultado_" + std::to_string(instante) + ".png");
    salvarPNG(arquivo, resultado);
    mensagem = "PNG salvo em output/images (resolucao original, sem os controles).";
    std::cout << "Salvo: " << arquivo.u8string() << '\n';
}

void Aplicativo::mouse_callback(int evento, int x, int y, int flags, void *dados)
{
    static_cast<Aplicativo *>(dados)->clicar(evento, x, y, flags);
}

void Aplicativo::clicar(int evento, int x, int y, int flags)
{
    const cv::Point p(x, y);

    if (evento == cv::EVENT_LBUTTONUP)
    {
        arrastando = -1;
        return;
    }
    if (evento == cv::EVENT_MOUSEMOVE)
    {
        if (arrastando >= 0 && (flags & cv::EVENT_FLAG_LBUTTON))
        {
            if (arrastando == 0) uiFiltro = valorSlider(TRILHA_FILTRO, 10, x);
            else uiParametro = valorSlider(TRILHA_PARAM, 255, x);
        }
        else arrastando = -1;
        return;
    }
    if (evento != cv::EVENT_LBUTTONDOWN) return;

    // Sliders (arrastaveis)
    if (ACERTO_FILTRO.contains(p))
    {
        arrastando = 0;
        uiFiltro = valorSlider(TRILHA_FILTRO, 10, x);
        return;
    }
    if (ACERTO_PARAM.contains(p))
    {
        arrastando = 1;
        uiParametro = valorSlider(TRILHA_PARAM, 255, x);
        return;
    }
    // Segmentos
    if (SEG_CANAL.contains(p)) { uiCanal = indiceSegmento(SEG_CANAL, 4, x); return; }
    if (SEG_OPERACAO.contains(p)) { uiOperacao = indiceSegmento(SEG_OPERACAO, 4, x); return; }

    // Botoes: a acao roda no loop principal (podem pedir caminho no console ou lancar excecao).
    for (const auto &b : BOTOES)
        if (b.area.contains(p)) { teclaPendente = b.tecla; return; }

    // Miniaturas dos stickers
    for (int i = 0; i < static_cast<int>(stickers.size()); ++i)
    {
        if (!cartaoSticker(i).contains(p)) continue;
        stickerSelecionado = (stickerSelecionado == i) ? -1 : i;
        mensagem = stickerSelecionado < 0 ? "Sticker desmarcado."
                                          : "Sticker selecionado: " + NOMES_STICKERS[stickerSelecionado];
        return;
    }

    // Posicionar sticker na foto
    if (modoVideo || original.empty() || stickerSelecionado < 0 || !areaFoto.contains(p)) return;
    // Mesmo conceito das Listas 2/3: converter coordenadas da viewport para o mundo.
    const cv::Point centro(
        std::clamp((x-areaFoto.x)*original.cols/areaFoto.width, 0, original.cols-1),
        std::clamp((y-areaFoto.y)*original.rows/areaFoto.height, 0, original.rows-1));
    const int largura = std::max(1, cvRound(80.0 * original.cols / areaFoto.width));
    posicoes.push_back({stickerSelecionado, centro, largura});
    atualizar = true;
    mensagem = "Sticker " + NOMES_STICKERS[stickerSelecionado] + " posicionado. R limpa tudo.";
}

void Aplicativo::desenhar()
{
    cv::Mat tela(ALTURA, LARGURA, CV_8UC3, FUNDO);

    // ---- Barra superior ----
    cv::rectangle(tela, cv::Rect(0, 0, LARGURA, 56), MARINHO, cv::FILLED);
    texto(tela, "FilterCam", cv::Point(24, 37), 0.95, BRANCO, 2);
    textoADireita(tela, "Aplicado: " + nomeEfeito(aplicado) + " | " + nomeCanal(aplicado.canal),
                  LARGURA - 24, 35, 0.5, CLARO);

    // ---- Cartao da imagem ----
    cartao(tela, CARTAO_FOTO, 14, BRANCO, BORDA);
    if (!original.empty())
    {
        if (atualizar)
        {
            resultado = comporResultado(original, segundaImagem, aplicado, stickers, posicoes);
            atualizar = false;
        }
        const double maxL = CARTAO_FOTO.width - 20.0, maxA = CARTAO_FOTO.height - 20.0;
        const double escala = std::min(maxL / resultado.cols, maxA / resultado.rows);
        const cv::Size tamanho(std::max(1, cvRound(resultado.cols*escala)), std::max(1, cvRound(resultado.rows*escala)));
        areaFoto = cv::Rect(CARTAO_FOTO.x + (CARTAO_FOTO.width-tamanho.width)/2,
                            CARTAO_FOTO.y + (CARTAO_FOTO.height-tamanho.height)/2,
                            tamanho.width, tamanho.height);
        cv::Mat previa;
        cv::resize(resultado, previa, tamanho, 0, 0, escala < 1 ? cv::INTER_AREA : cv::INTER_LINEAR);
        previa.copyTo(tela(areaFoto));
    }
    else
    {
        areaFoto = {};
        textoCentralizado(tela, "Nenhuma imagem carregada",
                          cv::Rect(CARTAO_FOTO.x, CARTAO_FOTO.y + 190, CARTAO_FOTO.width, 30), 0.8, MARINHO);
        textoCentralizado(tela, "Pressione F para abrir uma foto ou V para iniciar a webcam",
                          cv::Rect(CARTAO_FOTO.x, CARTAO_FOTO.y + 230, CARTAO_FOTO.width, 24), 0.5, CINZA);
    }

    // ---- Painel de controles ----
    cartao(tela, PAINEL, 14, BRANCO, BORDA);
    const int xi = 760, xd = 1060;

    texto(tela, "FILTRO", cv::Point(xi, 110), 0.45, CINZA);
    textoADireita(tela, listaFiltros().at(uiFiltro).nome, xd, 110, 0.5, MARINHO);
    desenharSlider(tela, TRILHA_FILTRO, uiFiltro, 10);

    texto(tela, "CANAL", cv::Point(xi, 176), 0.45, CINZA);
    desenharSegmentos(tela, SEG_CANAL, ROTULOS_CANAL, 4, uiCanal);

    texto(tela, "PARAMETRO P", cv::Point(xi, 252), 0.45, CINZA);
    textoADireita(tela, std::to_string(uiParametro), xd, 252, 0.5, MARINHO);
    desenharSlider(tela, TRILHA_PARAM, uiParametro, 255);

    texto(tela, "OPERACAO (2 IMAGENS)", cv::Point(xi, 316), 0.45, CINZA);
    desenharSegmentos(tela, SEG_OPERACAO, ROTULOS_OPERACAO, 4, uiOperacao);

    texto(tela, "Selecionado: " + nomeEfeito(selecionado), cv::Point(xi, 382), 0.48, MARINHO);
    texto(tela, "Descricao no console. ENTER aplica.", cv::Point(xi, 402), 0.4, CINZA);

    for (const auto &b : BOTOES) desenharBotao(tela, b);

    // ---- Rodape: stickers ----
    texto(tela, "STICKERS", cv::Point(20, 590), 0.45, CINZA);
    for (int i = 0; i < static_cast<int>(stickers.size()); ++i)
    {
        const cv::Rect r = cartaoSticker(i);
        const bool ativo = (i == stickerSelecionado);
        if (ativo) preencherArredondado(tela, cv::Rect(r.x - 2, r.y - 2, r.width + 4, r.height + 4), 12, AZUL);
        cartao(tela, r, 10, BRANCO, ativo ? AZUL : BORDA);
        // Largura limitada pela altura: stickers altos (Ponte) cabem no cartao.
        const double proporcao = static_cast<double>(stickers[i].cols) / std::max(1, stickers[i].rows);
        const int larguraMiniatura = std::max(1, cvRound(40.0 * std::min(1.0, proporcao)));
        sobreporSticker(tela, stickers[i], cv::Point(r.x + r.width / 2, r.y + r.height / 2 + 4), larguraMiniatura);
        texto(tela, std::to_string(i + 1), cv::Point(r.x + 7, r.y + 15), 0.4, ativo ? AZUL : CINZA);
    }
    texto(tela, std::string("Sticker: ") + (stickerSelecionado < 0 ? "nenhum" : NOMES_STICKERS[stickerSelecionado]),
          cv::Point(400, 624), 0.55, MARINHO);
    texto(tela, modoVideo ? "Disponivel apenas no modo foto" : "Clique na foto para posicionar | 0 desmarca",
          cv::Point(400, 648), 0.45, CINZA);

    // ---- Barra de status ----
    cv::rectangle(tela, cv::Rect(0, 668, LARGURA, 32), BRANCO, cv::FILLED);
    cv::line(tela, cv::Point(0, 668), cv::Point(LARGURA, 668), BORDA, 1);
    texto(tela, mensagem, cv::Point(20, 690), 0.5, AZUL);
    textoADireita(tela, "ESC sair", LARGURA - 20, 690, 0.45, CINZA);

    cv::imshow(JANELA, tela);
}

bool Aplicativo::tratarTecla(int tecla)
{
    if (tecla >= 'A' && tecla <= 'Z') tecla += 'a' - 'A';
    if (tecla >= '1' && tecla <= '5')
    {
        stickerSelecionado = tecla - '1';
        mensagem = "Sticker selecionado: " + NOMES_STICKERS[stickerSelecionado];
    }
    switch (tecla)
    {
        case 27: return true;
        case 'h': ajuda(); break;
        case '0': stickerSelecionado = -1; break;
        case 'r': resetar(); break;
        case 's': salvar(); break;
        case 10: case 13: aplicarSelecao(); break;
        case 'v': alternarWebcam(); break;
        case 'f':
        {
            const auto caminho = pedirCaminho("Caminho da foto");
            if (!caminho.empty()) carregarFoto(caminho);
            break;
        }
        case 'o':
        {
            const auto caminho = pedirCaminho("Caminho da segunda imagem");
            if (!caminho.empty())
            {
                segundaImagem = lerImagem(caminho, cv::IMREAD_COLOR);
                atualizar = true;
                mensagem = "Segunda imagem carregada. Selecione a operacao e pressione ENTER.";
            }
            break;
        }
        default: break;
    }
    return false;
}

int Aplicativo::executar(const std::filesystem::path &fotoInicial)
{
    carregarStickers();
    ajuda();
    cv::namedWindow(JANELA, cv::WINDOW_AUTOSIZE);
    cv::setMouseCallback(JANELA, mouse_callback, this);
    if (!fotoInicial.empty())
    {
        try { carregarFoto(fotoInicial); }
        catch (const std::exception &erro) { mensagem = erro.what(); std::cerr << mensagem << '\n'; }
    }
    bool sair = false;
    while (!sair)
    {
        try
        {
            if (camera.isOpened())
            {
                cv::Mat frame;
                if (camera.read(frame) && !frame.empty())
                {
                    original = frame.clone();
                    atualizar = true;
                }
                else
                {
                    camera.release();
                    mensagem = "A webcam parou de fornecer frames. V tenta reiniciar.";
                    std::cerr << mensagem << '\n';
                }
            }
            lerSelecao();
            desenhar();
            int tecla = cv::waitKeyEx(15);
            if (cv::getWindowProperty(JANELA, cv::WND_PROP_VISIBLE) < 1) break;
            // Cliques em botoes viram "teclas" aqui, fora do callback do mouse.
            if (tecla == -1 && teclaPendente != 0) tecla = teclaPendente;
            teclaPendente = 0;
            sair = tratarTecla(tecla);
        }
        catch (const std::exception &erro)
        {
            mensagem = erro.what();
            std::cerr << "Erro: " << mensagem << '\n';
        }
    }
    camera.release();
    cv::destroyAllWindows();
    return 0;
}
