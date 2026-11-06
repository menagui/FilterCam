#include "Aplicativo.h"

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <chrono>
#include <iostream>

static const std::vector<std::string> NOMES_STICKERS = {"Link", "Princesa", "Arvore", "Arbusto", "Ponte"};
static constexpr const char *FILTRO = "Filtro (0 original, 1-10)";
static constexpr const char *CANAL = "Canal (0 RGB, 1 R, 2 G, 3 B)";
static constexpr const char *PARAMETRO = "Parametro P (0-255)";
static constexpr const char *OPERACAO = "Operacao (0 nenhuma, 1 +, 2 -, 3 blend)";

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
        << "Barras: selecionar filtro, canal, parametro e operacao\n"
        << "ENTER: aplicar a selecao; descricoes aparecem antes no console\n"
        << "Canal R/G/B: a imagem principal passa a exibir aquele canal em cinza\n"
        << "S: salvar resultado/frame em output/images, ao lado do executavel\n"
        << "R: resetar filtro, operacao, canal, parametro e todos os stickers\n"
        << "1 Link | 2 Princesa | 3 Arvore | 4 Arbusto | 5 Ponte\n"
        << "Selecione com 1-5 e clique na foto; 0 desmarca o sticker\n"
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
    posicoes.clear();
    stickerSelecionado = -1;
    cv::setTrackbarPos(FILTRO, JANELA, 0);
    cv::setTrackbarPos(OPERACAO, JANELA, 0);
    cv::setTrackbarPos(CANAL, JANELA, 0);
    cv::setTrackbarPos(PARAMETRO, JANELA, 128);
    atualizar = true;
    mensagem = "Reset: imagem original, sem efeitos e sem stickers.";
}

void Aplicativo::lerSelecao()
{
    int filtro = cv::getTrackbarPos(FILTRO, JANELA);
    int operacao = cv::getTrackbarPos(OPERACAO, JANELA);
    const int canal = cv::getTrackbarPos(CANAL, JANELA);
    const int parametro = cv::getTrackbarPos(PARAMETRO, JANELA);
    if (filtro == ultimoFiltro && operacao == ultimaOperacao && canal == ultimoCanal && parametro == ultimoParametro)
        return;
    if (filtro != ultimoFiltro)
    {
        operacao = 0;
        cv::setTrackbarPos(OPERACAO, JANELA, 0);
    }
    else if (operacao != ultimaOperacao && operacao != 0)
    {
        filtro = 0;
        cv::setTrackbarPos(FILTRO, JANELA, 0);
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

void Aplicativo::mouse_callback(int evento, int x, int y, int, void *dados)
{
    static_cast<Aplicativo *>(dados)->clicar(evento, x, y);
}

void Aplicativo::clicar(int evento, int x, int y)
{
    if (evento != cv::EVENT_LBUTTONDOWN || modoVideo || original.empty() || stickerSelecionado < 0
        || !areaFoto.contains(cv::Point(x, y))) return;
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
    cv::Mat tela(CABECALHO + ALTURA_FOTO + RODAPE, LARGURA, CV_8UC3, cv::Scalar(28, 28, 28));
    const auto texto = [&](const std::string &s, int y, double escala = 0.48, cv::Scalar cor = cv::Scalar(225,225,225))
    {
        cv::putText(tela, s, cv::Point(16, y), cv::FONT_HERSHEY_SIMPLEX, escala, cor, 1, cv::LINE_AA);
    };
    texto(std::string("GRAU B | ") + (modoVideo ? (camera.isOpened() ? "VIDEO AO VIVO" : "VIDEO PARADO") : "FOTO")
        + " | Aplicado: " + nomeEfeito(aplicado) + " | " + nomeCanal(aplicado.canal), 24, 0.6);
    texto("Selecionado: " + nomeEfeito(selecionado) + " | P=" + std::to_string(selecionado.parametro)
        + " | ENTER aplica | Descricao no console", 50);
    if (!original.empty())
    {
        if (atualizar)
        {
            resultado = comporResultado(original, segundaImagem, aplicado, stickers, posicoes);
            atualizar = false;
        }
        const double escala = std::min(960.0 / resultado.cols, static_cast<double>(ALTURA_FOTO) / resultado.rows);
        const cv::Size tamanho(std::max(1, cvRound(resultado.cols*escala)), std::max(1, cvRound(resultado.rows*escala)));
        areaFoto = cv::Rect((LARGURA-tamanho.width)/2, CABECALHO+(ALTURA_FOTO-tamanho.height)/2, tamanho.width, tamanho.height);
        cv::Mat previa;
        cv::resize(resultado, previa, tamanho, 0, 0, escala < 1 ? cv::INTER_AREA : cv::INTER_NEAREST);
        previa.copyTo(tela(areaFoto));
    }
    else
    {
        areaFoto = {};
        texto("Pressione F para carregar uma foto ou V para iniciar a webcam.", CABECALHO+ALTURA_FOTO/2, 0.65);
    }
    const int base = CABECALHO + ALTURA_FOTO;
    texto("F foto | V webcam | O segunda imagem | ENTER aplicar | S salvar | R reset | H ajuda | ESC sair", base+24);
    texto("Stickers (foto): 1 Link | 2 Princesa | 3 Arvore | 4 Arbusto | 5 Ponte | 0 desmarca", base+48);
    texto("Sticker: " + (stickerSelecionado < 0 ? std::string("nenhum") : NOMES_STICKERS[stickerSelecionado])
        + (modoVideo ? " (disponivel apenas em foto)" : " | Clique na foto para posicionar"), base+72);
    texto(mensagem, base+100, 0.44, cv::Scalar(125,220,230));
    if (stickerSelecionado >= 0)
        sobreporSticker(tela, stickers[stickerSelecionado], cv::Point(958,base+68), 38);
    cv::imshow(JANELA, tela);
}

int Aplicativo::executar(const std::filesystem::path &fotoInicial)
{
    carregarStickers();
    ajuda();
    cv::namedWindow(JANELA, cv::WINDOW_AUTOSIZE);
    cv::createTrackbar(FILTRO, JANELA, nullptr, 10);
    cv::createTrackbar(CANAL, JANELA, nullptr, 3);
    cv::createTrackbar(PARAMETRO, JANELA, nullptr, 255);
    cv::createTrackbar(OPERACAO, JANELA, nullptr, 3);
    cv::setTrackbarPos(PARAMETRO, JANELA, 128);
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
            if (tecla >= 'A' && tecla <= 'Z') tecla += 'a' - 'A';
            if (tecla >= '1' && tecla <= '5')
            {
                stickerSelecionado = tecla - '1';
                mensagem = "Sticker selecionado: " + NOMES_STICKERS[stickerSelecionado];
            }
            switch (tecla)
            {
                case 27: sair = true; break;
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
